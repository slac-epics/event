/*=============================================================================

  Name: mpgPattern.c
           mpgPatternInit      - General Initialization
           mpgPatternPnetInit  - Pnet Pattern Processing Initialization
           mpgPatternPnet      - 360Hz Pnet Pattern Processing
           mpgPatternProcInit  - MPG Pattern Setup Initialization
           mpgPatternProc      - 360Hz MPG Pattern Setup
           mpgPatternState     - Pattern Processing State and Diagnostics

  Abs: This file contains all subroutine support for the MPG IOC processing
       of the incoming Pnet pattern and outgoing MPG pattern.

  Rem: All functions called by the subroutine record get passed one argument:

         psub                       Pointer to the subroutine record data.
          Use:  pointer
          Type: struct subRecord *
          Acc:  read/write
          Mech: reference

         All functions return a long integer.  0 = OK, -1 = ERROR.
         The subroutine record ignores the status returned by the Init
         routines.  For the calculation routines, the record status (STAT) is
         set to SOFT_ALARM (unless it is already set to LINK_ALARM due to
         severity maximization) and the severity (SEVR) is set to psub->brsv
         (BRSV - set by the user in the database though it is expected to
          be invalid).

  Auth: 04-dec-2006, S. Allison (saa), some logic taken from
        D. Kotturi's drvPnet.c
  Rev:  

-----------------------------------------------------------------------------*/

#include "copyright_SLAC.h"     /* SLAC copyright comments */

/*-----------------------------------------------------------------------------

  Mod:  (newest to oldest)

=============================================================================*/
#include "debugPrint.h"
#include "subRecord.h"        /* for struct subRecord       */
#include "sSubRecord.h"       /* for struct sSubRecord      */
#include "registryFunction.h" /* for epicsExport            */
#include "epicsExport.h"      /* for epicsRegisterFunction  */
#include "epicsTime.h"        /* for epicsTimeGetCurrent    */
#include "dbAccess.h"         /* DBADDR typedef and protos  */
#include "evrPattern.h"       /* for evrPattern* prototypes */
#include "evrTime.h"          /* for evrTime* prototypes    */
#include "pnetSeqCheck.h"     /* for pnetSeqCheck* protos   */
#ifdef __rtems__
#include "drvMrfEg.h"         /* for EvgDataBufferLoad proto*/
#endif

/* Modifier 1 Bit Masks */
#define PNET_MPG_IPLING         (0x00004000)  /* Set when SLC MPG is IPLing  */
#define PNET_MODULO720_RESYNC   (0x00008000)  /* Set to sync modulo 720      */

/* This number should rollover every 69 days at 360 Hz */
#define MPG_MAX_INT	       (2147483647)  /* 4 byte int - 1 bit for sign */

/* VAL values set by subroutines */
#define EVG_OK                0
#define EVG_SEQ_CHECK1_ERR    1
#define EVG_SEQ_CHECK2_ERR    2
#define EVG_SEQ_CHECK3_ERR    3
#define EVG_INVALID_WF        4
#define EVG_MPG_IPLING        5
#define EVG_PULSEID_NO_SYNC   6
#define EVG_MODULO720_NO_SYNC 7
#define EVG_INVALID_TIMESTAMP 8

#define EVG_DELTA_TIME 0.008333333333 /* 1/120sec for setting time 3 pulses ahead */

#ifdef DEBUG_PRINT
  /* ENUM with values: DP_FATAL, DP_ERROR, DP_WARN, DP_INFO, DP_DEBUG
     Anything less than or equal to mpgPatternFlag gets printed out. Can
     be set to 0 here and altered via prompt or in st.cmd file */
int mpgPatternFlag = 0;
#endif

static unsigned int msgCount         = 0; /* # waveforms processed since boot/reset */ 
static unsigned int msgRolloverCount = 0; /* # time msgCount reached MPG_MAX_INT    */ 
static unsigned int resyncCount      = 0; /* # of times 6min 2s rollover happened since boot/reset */
static unsigned int pulseIDSyncErrCount   = 0; /* # of pulse ID  sync errors */
static unsigned int modulo720SyncErrCount = 0; /* # of modulo720 sync errors */

/*=============================================================================

  Name: mpgPatternInit

  Abs:  General purpose initialization required since all subroutine records
   require a non-NULL init routine even if no initialization is required.
   Note that most subroutines in this file use this routine as an init
   routine.  If init logic is needed for a specific subroutine, create a
   new routine for it - don't modify this one.
  
==============================================================================*/ 
static int mpgPatternInit(subRecord *psub)
{
  return 0;
}

/*=============================================================================

  Name: mpgPatternPnetInit

  Abs:  Initialization for 360Hz Processing to Process the Pnet Pattern

  Args: Type                Name        Access     Description
        ------------------- ----------- ---------- ----------------------------
        subRecord *         psub        read       point to subroutine record

  Rem:  Initialization subroutine for EVG:IN20:EV01:PATTERNPNET

  Inputs:
       A - Pnet waveform, used to access waveform data
     
  Outputs:
       DPVT - pointer to waveform data
  
==============================================================================*/ 
static int mpgPatternPnetInit(subRecord *psub)
{
  DBADDR      *wfAddr = dbGetPdbAddrFromLink(&psub->inpa);
  /*
   * inpa must be a DB link and must be the proper type.
   */
  if (wfAddr && (wfAddr->field_type == DBF_LONG))
    psub->dpvt = (void *)wfAddr->pfield;
  else
    psub->dpvt = 0;
  if (psub->dpvt) return 0;
  else            return -1;
}

/*=============================================================================

  Name: mpgPatternPnet

  Abs:  360Hz Processing of the Pnet pattern

  Args: Type                Name        Access     Description
        ------------------- ----------- ---------- ----------------------------
        subRecord *         psub        read       point to subroutine record

  Rem:  Subroutine for EVG:IN20:EV01:PATTERNPNET

  Inputs:
       A - Pnet waveform, used to access waveform data
       B - Pnet waveform severity
       C - Pnet waveform number of elements
       H - Flag to stop sequence checking
       I - Previous Modulo 720 Count
       L - Previous Pulse ID
     
  Outputs:
       D - Pnet Modifier 1
       E - Pnet Modifier 2
       F - Pnet Modifier 3
       G - Pnet Modifier 4
       I - Modulo 720 Count
       J - Beam Code
       K - YY
       L - Pulse ID
       VAL = Error flag:
             0 = OK
             1 = Seq check 1 error
             2 = Seq check 2 error
             3 = Seq check 3 error
             4 = Invalid Waveform
             5 = MPG IPLing
             6 = Pulse ID   not synchronized with the SLC MPG
             7 = Modulo 720 not synchronized with the SLC MPG

  Side: None
  
  Ret:  0 = OK, -1 = ERROR

==============================================================================*/
static long mpgPatternPnet(subRecord *psub)
{
  DBADDR                *wfAddr = dbGetPdbAddrFromLink(&psub->inpa);
  evrPatternPnet_ts     *evrPatternPnet_ps = (evrPatternPnet_ts *)psub->dpvt;
  int                    pulsid, pulsid_resync, modulo720Count;
  int                    errFlag = EVG_OK;
  unsigned char          traveling_one;
  unsigned char          seq_chk;
  static unsigned int    pulseIDSync   = 0; /* EVG and SLC MPG pulse ID synced  */
  static unsigned int    modulo720Sync = 0; /* EVG and SLC MPG modulo720 synced */

  /* Keep a count of messages and reset before overflow */
  if (msgCount < MPG_MAX_INT) {
    msgCount++;
  } else {
    msgRolloverCount++;
    msgCount = 0;
  }
  
  /* if waveform is invalid or has invalid size or type            */
  /*   set record invalid and pulse id to 0 and set flag           */
  /*   evr timestamp/status to last good time with invalid status */

  if (psub->b ||
      (psub->c < (sizeof(evrPatternPnet_ts)/sizeof(epicsUInt32))) ||
      (!psub->dpvt) || (!wfAddr)) {
    psub->d       = 0.0;
    psub->e       = 0.0;
    psub->f       = 0.0;
    psub->g       = 0.0;
    psub->j       = 0.0;
    psub->i       = 0.0;
    psub->k       = 0.0;
    psub->l       = 0.0;
    psub->val     = EVG_INVALID_WF;
    pulseIDSync   = 0;
    modulo720Sync = 0;
    return -1;
  }		
  /* read from the waveform now */
  dbScanLock(wfAddr->precord);
  /* set outputs to the modifiers */
  psub->d = (double)(evrPatternPnet_ps->modifier1);
  psub->e = (double)(evrPatternPnet_ps->modifier2);
  psub->f = (double)(evrPatternPnet_ps->modifier3);
  psub->g = (double)(evrPatternPnet_ps->modifier4);
  /* beamcode decoded from modifier 1*/ 
  psub->j = (double)((evrPatternPnet_ps->modifier1 >> 8) & BEAMCODE_BIT_MASK);
  /* yy decoded from modifier 1 bits */
  psub->k = (double)(evrPatternPnet_ps->modifier1 & YY_BIT_MASK);
  
  /* Check if SLC MPG is rebooting */
  if (evrPatternPnet_ps->modifier1 & PNET_MPG_IPLING) {
    psub->i       = 0.0;
    psub->l       = 0.0;
    psub->val     = EVG_MPG_IPLING;
    pulseIDSync   = 0;
    modulo720Sync = 0;
    dbScanUnlock(wfAddr->precord);
    return -1;
  }
  
  /* Do sequence checking if requested */
  if (psub->h > 0.5) { 
    traveling_one = (unsigned char)(evrPatternPnet_ps->modifier2 & TIMESLOT_MASK);
    if (!pnetSeqCheckData1(traveling_one)) /* "traveling 1" check */
      errFlag = EVG_SEQ_CHECK1_ERR;

    seq_chk = (unsigned char)((evrPatternPnet_ps->modifier4 >> 29) & PNET_SEQ_CHECK);
    if (!pnetSeqCheckData2(seq_chk))  /* 0-5 looping counter check */
      errFlag = EVG_SEQ_CHECK2_ERR;

    if (!pnetSeqCheckData3(traveling_one, seq_chk)) {/* check that two checksum values stay in sync */ 
      if (errFlag == EVG_OK) errFlag = EVG_SEQ_CHECK2_ERR;
    }
  }

  /* Check if modulo-720 can be resynchronized on this pulse. */
  modulo720Count = psub->i + 1.5;
  if (evrPatternPnet_ps->modifier1 & PNET_MODULO720_RESYNC) {
    /* If we should be sync'ed but are not, update a counter */
    if (modulo720Sync && (modulo720Count != 720)) {
      modulo720SyncErrCount++;
      DEBUGPRINT(DP_INFO, mpgPatternFlag, ("mpgPatternPnet: Modulo 720 not synchronized, modulo720Count = %d", modulo720Count));
    }
    modulo720Count = 1;
    modulo720Sync  = 1;
  } else if (!modulo720Sync) {
    modulo720Count = 0;
    errFlag = EVG_MODULO720_NO_SYNC;
  }
  psub->i = (double)modulo720Count;

  /* Check for a non-zero value in the 4 bit pulsid_resync field of Pnet
     data.  If found, it resets upper 4 bits of the pulse ID (lower bit
     will be zero).  Otherwise, the pulse ID is set to 1 if the last value
     is the rollover value.  Otherwise, the pulse ID is simply incremented.*/
  pulsid        = psub->l + 1.5;
  pulsid_resync = (evrPatternPnet_ps->modifier1>>3) & PULSEID_RESYNC;
  if (pulsid_resync) {
    /* If we should be sync'ed but are not, update a counter */
    if ((pulseIDSync) && (pulsid != pulsid_resync)) {
      pulseIDSyncErrCount++;
      DEBUGPRINT(DP_INFO, mpgPatternFlag, ("mpgPatternPnet: Pulse IDs not synchronized, mpg_pulsid = %d, evg_pulsid = %d\n", pulsid_resync, pulsid));
    }
    pulseIDSync = 1;
    pulsid = pulsid_resync;
  }
  else if (!pulseIDSync) {
    pulsid  = 1;
    errFlag = EVG_PULSEID_NO_SYNC;
  } else if (pulsid > PULSEID_MAX) {    
    /* Aha. Time to rollover, so THIS pulse will be set back to zero */
    pulsid = 0;
    resyncCount++;
    DEBUGPRINT(DP_INFO, mpgPatternFlag, ("mpgPatternPnet: Rollover, pulsid+1 = %d", pulsid));
  }
  psub->l = (double)pulsid;
  
  dbScanUnlock(wfAddr->precord);
  psub->val = (double)errFlag;
  if (errFlag != EVG_OK) return -1;
  return 0;
}

/*=============================================================================

  Name: mpgPatternProcInit

  Abs:  Initialization for 360Hz Processing to Set Up the MPG Pattern

  Args: Type                Name        Access     Description
        ------------------- ----------- ---------- ----------------------------
        subRecord *         psub        read       point to subroutine record

  Rem:  Initialization subroutine for EVG:IN20:EV01:PATTERN

  Inputs:
       A - EVG Card Number
     
  Outputs:
    DPVT - Pointer to EVG Card Structure
  
==============================================================================*/ 
static int mpgPatternProcInit(subRecord *psub)
{

  /*
   * If a card number is provided, initialize the EVG data buffer size
   * and get the pointer to the card structure.
   */
#ifdef __rtems__
  if (psub->a > 0.5) {
    EvgDataBufferInit((int)psub->a, sizeof(evrPatternWaveform_ts));
    psub->dpvt = (EgCardStruct *)EgGetCardStruct((int)psub->a);
    if (psub->dpvt) return 0;
    else            return -1;
  }
#endif
  return 0;
}

/*=============================================================================

  Name: mpgPatternProc

  Abs:  360Hz Processing to Set Up the MPG Pattern

  Args: Type                Name        Access     Description
        ------------------- ----------- ---------- ----------------------------
        subRecord *         psub        read       point to subroutine record

  Rem:  Subroutine for EVG:IN20:EV01:PATTERN

  Inputs:
    DPVT - Pointer to EVG Card Structure
       A - EVG Card Number
       B - Pnet processing error flag
       C - Spare
       D - Pnet Modifier 1
       E - Pnet Modifier 2
       F - Pnet Modifier 3
       G - Pnet Modifier 4
       H - EVG Modifier  5 - Beam Synchronous Acquisition (BSA) Bit Mask
               One bit per BSA event definition (EDEF).
               20 EDEF total.
               0 = don't process this pulse, 1 = process this pulse.
       I - Bunch Charge
       J - Beam Code
       K - YY
       L - Pulse ID
     
  Outputs:
       L - Pulse ID (set invalid if error)
       VAL = Error flag:
             0 = OK
             1 = Seq check 1 error
             2 = Seq check 2 error
             3 = Seq check 3 error
             4 = Invalid Waveform
             5 = MPG IPLing
             6 = Pulse ID   not synchronized with the SLC MPG
             7 = Modulo 720 not synchronized with the SLC MPG
             8 = Invalid Timestamp

  Side: Pattern is sent out via the EVG.
  
  Ret:  0 = OK, -1 = ERROR

==============================================================================*/
static long mpgPatternProc(subRecord *psub)
{
  evrPatternWaveform_ts  evrPatternWF_s;
  epicsTimeStamp         prev_time;
  double                 delta_time;
  int                    status = 0;

  psub->val = psub->b;
  
  /* Do nothing until the pattern is valid and synchronized */
  if (psub->val >= EVG_INVALID_WF) {
    psub->d = psub->e = psub->f = psub->g = psub->h = psub->i = psub->j = psub->k = 0.0;
    status = -1;
  /* Initialize EVR timestamp to system time */
  } else if (epicsTimeGetCurrent(&evrPatternWF_s.time)) {
    psub->val = EVG_INVALID_TIMESTAMP;
    status = -1;
  /* The time is for 3 pulses in the future - add 1/120sec */
  } else {
    epicsTimeAddSeconds(&evrPatternWF_s.time, EVG_DELTA_TIME);
    /* Overlay the pulse ID into the lower 17 bits of the nsec field */
    evrTimePutPulseID(&evrPatternWF_s.time, (unsigned int)psub->l);

    /* The timestamp must ALWAYS be increasing.  Check if this time
       is less than the previous time (due to rollover) and adjust up
       slightly using bit 17 (the lower-most bit of the top 15 bits). */
    if (evrTimeGet(&prev_time, evrTimeNext3) == epicsTimeOK) {
      delta_time = epicsTimeDiffInSeconds(&evrPatternWF_s.time, &prev_time);
      if (delta_time < 0.0) {
        epicsTimeAddSeconds(&evrPatternWF_s.time, PULSEID_BIT17/NSEC_PER_SEC);
        evrTimePutPulseID(&evrPatternWF_s.time, (unsigned int)psub->l);
      }
    }
    /* write to evr timestamp table and error check */
    if (evrTimePut(&evrPatternWF_s.time, evrTimeOK)) {
      psub->val = EVG_INVALID_TIMESTAMP;
      status = -1;
    }
  }
  /* Timestamp done - now fill in the rest of the pattern */
  if (status) {
    psub->l = PULSEID_INVALID;
    evrTimePut (0, evrTimeInvalid);
  } else {
    evrPatternWF_s.header_s.type    = EVR_WF_PATTERN;
    evrPatternWF_s.header_s.version = EVR_WF_PATTERN_VERSION;
    evrPatternWF_s.pnet_s.modifier1 = psub->d;
    evrPatternWF_s.pnet_s.modifier2 = psub->e;
    evrPatternWF_s.pnet_s.modifier3 = psub->f;
    evrPatternWF_s.pnet_s.modifier4 = psub->g;
    evrPatternWF_s.modifier5        = psub->h;
    evrPatternWF_s.bunchcharge      = psub->i;
#ifdef __rtems__
    if (psub->dpvt) {
      EvgDataBufferUpdate((EgCardStruct *)psub->dpvt,
                          (epicsUInt32 *)&evrPatternWF_s,
                          sizeof(evrPatternWaveform_ts)/sizeof(epicsUInt32));
    }
#endif
  }
  return status;
}
/*=============================================================================

  Name: mpgPatternState

  Abs:  Access to Last MPG Pattern State and Pattern Diagnostics.
        This subroutine is for status only and should update at a low
        rate like 1Hz.

  Args: Type                Name        Access     Description
        ------------------- ----------- ---------- ----------------------------
        subRecord *         psub        read       point to subroutine record

  Rem:  Subroutine for EVG:IN20:EV01:PATTERNSTATE

  Inputs:
       A - Error Flag from mpgPatternProc
       B - Counter Reset Flag
       C - Spare
     
  Outputs:
       D - Number of pulse Id 6min 2sec cycles that have transpired since boot
       E - Number of waveforms processed by this subroutine
       F - Number of times E has rolled over
       G - Number of seq check 1 errors
       H - Number of seq check 2 errors
       I - Number of seq check 3 errors
       J - Number of unexpected unsynchronized pulse IDs
       K - Number of unexpected unsynchronized modulo720s
       L - Spare
       VAL = Last Error flag (see mpgPatternProc for values)

  Side: File-scope counters may be reset.
  
  Ret:  0 = OK

==============================================================================*/
static long mpgPatternState(subRecord *psub)
{
  psub->val = psub->a;
  if (psub->b > 0.5) {
    psub->b = 0.0;
    msgCount              = 0;
    msgRolloverCount      = 0;
    resyncCount           = 0;
    seqCheck1ErrCount     = 0;
    seqCheck2ErrCount     = 0;
    seqCheck3ErrCount     = 0;
    pulseIDSyncErrCount   = 0;
    modulo720SyncErrCount = 0;
  }
  psub->d = msgCount;          /* # waveforms processed since boot/reset */
  psub->e = msgRolloverCount;  /* # time msgCount reached MPG_MAX_INT    */
  psub->f = resyncCount;       /* # of times 6min 2s rollover happened since boot/reset */
  psub->g = seqCheck1ErrCount;
  psub->h = seqCheck2ErrCount;
  psub->i = seqCheck3ErrCount;
  psub->j = pulseIDSyncErrCount;
  psub->k = modulo720SyncErrCount;
  return 0;
}
/*=============================================================================

  Name: mpgPatternEdefEnables

  Abs:  Create Modifier5 from the 20 Patterncheck values
        360Hz Processing
		
  Args: Type	            Name        Access	   Description
        ------------------- -----------	---------- ----------------------------
        subRecord *         psub        read       point to subroutine record

  Rem:  Subroutine for pattern MODIFIER5

  Side:

  Sub Inputs/ Outputs:
   Inputs:
    A-U - EDEF PATTERNCHECK1-20
    
   Outputs:   
	VAL = MODIFIER5
  Ret:  0

==============================================================================*/
static long mpgPatternEdefEnables (sSubRecord *psub)
{ 
  unsigned long mod5 = 0;

  mod5 |= (unsigned long)psub->a;
  mod5 |= ( ((unsigned long)psub->b) << 1 );
  mod5 |= ( ((unsigned long)psub->c) << 2 );
  mod5 |= ( ((unsigned long)psub->d) << 3 );
  mod5 |= ( ((unsigned long)psub->e) << 4 );
  mod5 |= ( ((unsigned long)psub->f) << 5 );
  mod5 |= ( ((unsigned long)psub->g) << 6 );
  mod5 |= ( ((unsigned long)psub->h) << 7 );
  mod5 |= ( ((unsigned long)psub->i) << 8 );
  mod5 |= ( ((unsigned long)psub->j) << 9 );
  mod5 |= ( ((unsigned long)psub->k) << 10 );
  mod5 |= ( ((unsigned long)psub->l) << 11 );
  mod5 |= ( ((unsigned long)psub->m) << 12 );
  mod5 |= ( ((unsigned long)psub->n) << 13 );
  mod5 |= ( ((unsigned long)psub->o) << 14 );
  mod5 |= ( ((unsigned long)psub->p) << 15 );
  mod5 |= ( ((unsigned long)psub->q) << 16 );
  mod5 |= ( ((unsigned long)psub->r) << 17 );
  mod5 |= ( ((unsigned long)psub->s) << 18 );
  mod5 |= ( ((unsigned long)psub->u) << 19 );

  psub->val = (double)mod5;

  return 0;
}
epicsRegisterFunction(mpgPatternInit);
epicsRegisterFunction(mpgPatternPnetInit);
epicsRegisterFunction(mpgPatternPnet);
epicsRegisterFunction(mpgPatternProcInit);
epicsRegisterFunction(mpgPatternProc);
epicsRegisterFunction(mpgPatternState);
epicsRegisterFunction(mpgPatternEdefEnables);

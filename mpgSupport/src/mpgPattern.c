/*=============================================================================

  Name: mpgPattern.c
           mpgPatternInit      - General Initialization
           mpgPatternPnetInit  - Pnet Pattern Processing Initialization
           mpgPatternPnet      - 360Hz Pnet Pattern Processing
           mpgPatternProcInit  - MPG Pattern Setup Initialization
           mpgPatternProc      - 360Hz MPG Pattern Setup
           mpgPatternState     - Pattern Processing State and Diagnostics
	   mpgPatternCheck     - Check Pattern against Event Definitions

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
#include "errlog.h"           /* errlogPrintf               */
#include "evrQueue.h"         /* for EVR_QUEUE_PATTERN*     */
#include "evrTime.h"          /* for evrTime* prototypes    */
#include "evrPattern.h"       /* for definitions            */
#include "pnetSeqCheck.h"     /* for pnetSeqCheck* protos   */
#ifdef __rtems__
#include "drvMrfEg.h"         /* for EvgDataBufferLoad proto*/
#endif
void pnetSend(void *message);

/* Modifier 1 Bit Masks */
#define PNET_MPG_IPLING         (0x00004000)  /* Set when SLC MPG is IPLing  */
#define PNET_MODULO720_RESYNC   (0x00008000)  /* Set to sync modulo 720      */

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
static unsigned int msgRolloverCount = 0; /* # time msgCount reached EVR_MAX_INT    */ 
static unsigned int resyncCount      = 0; /* # of times 6min 2s rollover happened since boot/reset */
static unsigned int patternErrCount       = 0; /* # bad PNET waveforms       */
static unsigned int pulseIDSyncErrCount   = 0; /* # of pulse ID  sync errors */
static unsigned int modulo720SyncErrCount = 0; /* # of modulo720 sync errors */
static unsigned int seqCheck1ErrCount     = 0;
static unsigned int seqCheck2ErrCount     = 0;
static unsigned int seqCheck3ErrCount     = 0;

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

  Rem:  Initialization subroutine for IOC:LOCA:UNIT:PNET

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
  if (wfAddr && (wfAddr->field_type == DBF_ULONG))
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

  Rem:  Subroutine for IOC:LOCA:UNIT:PNET

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
       K - Spare
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
  epicsUInt32           *pnet_a = (epicsUInt32 *)psub->dpvt;
  int                    pulsid, pulsid_resync, modulo720Count;
  int                    errFlag = EVG_OK;
  unsigned char          traveling_one;
  unsigned char          seq_chk;
  static unsigned int    pulseIDSync   = 0; /* EVG and SLC MPG pulse ID  synced */
  static unsigned int    modulo720Sync = 0; /* EVG and SLC MPG modulo720 synced */
  static unsigned int    seqCheckSync  = 0; /* sequence checking         synced */

  /* Keep a count of messages and reset before overflow */
  if (msgCount < EVR_MAX_INT) {
    msgCount++;
  } else {
    msgRolloverCount++;
    msgCount = 0;
  }
  
  /* if waveform is invalid or has invalid size or type            */
  /*   set record invalid and pulse id to 0 and set flag           */
  /*   evr timestamp/status to last good time with invalid status  */

  if (psub->b || (psub->c != EVR_PNET_MODIFIER_MAX) ||
      (!psub->dpvt) || (!wfAddr)) {
    if (psub->b) patternErrCount++;
    psub->d = psub->e = psub->f = psub->g = psub->i = psub->j = psub->l = 0.0;
    psub->val     = EVG_INVALID_WF;
    pulseIDSync   = 0;
    modulo720Sync = 0;
    seqCheckSync  = 0;
    return -1;
  }		
  /* read from the waveform now */
  dbScanLock(wfAddr->precord);
  /* set outputs to the modifiers */
  psub->d = (double)(pnet_a[0]);
  psub->e = (double)(pnet_a[1]);
  psub->f = (double)(pnet_a[2]);
  psub->g = (double)(pnet_a[3]);
  /* beamcode decoded from modifier 1*/ 
  psub->j = (double)((pnet_a[0] >> 8) & BEAMCODE_BIT_MASK);
  
  /* Check if SLC MPG is rebooting */
  if (pnet_a[0] & PNET_MPG_IPLING) {
    psub->i       = 0.0;
    psub->l       = 0.0;
    psub->val     = EVG_MPG_IPLING;
    pulseIDSync   = 0;
    modulo720Sync = 0;
    seqCheckSync  = 0;
    dbScanUnlock(wfAddr->precord);
    return -1;
  }
  
  /* Do sequence checking if requested */
  if (psub->h > 0.5) { 
    traveling_one = (unsigned char)(pnet_a[1] & TIMESLOT_MASK);
    seq_chk       = (unsigned char)((pnet_a[3] >> 29) & PNET_SEQ_CHECK);
    if (pnetSeqCheckData1(traveling_one, seqCheckSync)) { /* "traveling 1" check */
      errFlag = EVG_SEQ_CHECK1_ERR;
      seqCheck1ErrCount++;
    } else if (pnetSeqCheckData2(seq_chk, seqCheckSync)) { /* 0-5 looping counter check */
    /* Need to do more here to really handle this condition. TEG says
       if MPG gets "off" chances are beam is hitting the sides. The
       reaction of choice is for the micro to start sending zero beam
     */
      errFlag = EVG_SEQ_CHECK2_ERR;
      seqCheck2ErrCount++;
    } else if (pnetSeqCheckData3(traveling_one, seq_chk)) {/* check that two checksum values stay in sync */ 
      errFlag = EVG_SEQ_CHECK3_ERR;
      seqCheck3ErrCount++;
    }
    /* Both pulse ID and modulo 720 will need to be resync'ed too */
    if (errFlag != EVG_OK) {
      pulseIDSync   = 0;
      modulo720Sync = 0;
    }
    seqCheckSync = 1;
  } else {
    seqCheckSync = 0;
  } 

  /* Check if modulo-720 can be resynchronized on this pulse. */
  modulo720Count = psub->i + 1.1;
  if (pnet_a[0] & PNET_MODULO720_RESYNC) {
    /* If we should be sync'ed but are not, update a counter */
    if (modulo720Sync && (modulo720Count != 720)) {
      modulo720SyncErrCount++;
      errlogPrintf("mpgPatternPnet: Modulo 720 not synchronized, modulo720Count = %d\n",
                   modulo720Count);
      /* Assume pulse ID is also out-of-sync */
      pulseIDSync = 0;
    }
    modulo720Count = 0;
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
  pulsid        = psub->l + 1.1;
  pulsid_resync = (pnet_a[0] >> 3) & PULSEID_RESYNC;
  if (pulsid_resync) {
    /* If we should be sync'ed but are not, update a counter */
    if ((pulseIDSync) && (pulsid != pulsid_resync)) {
      pulseIDSyncErrCount++;
      errlogPrintf("mpgPatternPnet: Pulse IDs not synchronized, MPG = %d, EVG = %d\n",
                   pulsid_resync, pulsid);
      /* Assume modulo 720 out-of-sync too unless the RESYNC bit is set */
      if (!(pnet_a[0] & PNET_MODULO720_RESYNC)) {
        modulo720Sync  = 0;
        errFlag = EVG_MODULO720_NO_SYNC;
      }
    }
    pulseIDSync = 1;
    pulsid = pulsid_resync;
  } else if (!pulseIDSync) {
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

  Rem:  Initialization subroutine for IOC:LOCA:UNIT:PATTERN

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
    EvgDataBufferInit((int)psub->a, sizeof(evrQueuePattern_ts));
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

  Rem:  Subroutine for IOC:LOCA:UNIT:PATTERN

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
       K - Spare
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
  evrQueuePattern_ts     evrPatternWF_s;
  epicsTimeStamp         prev_time;
  double                 delta_time;
  int                    status = 0;

  psub->val = psub->b;
  
  /* Do nothing until the pattern is valid and synchronized */
  if (psub->val > EVG_OK) {
    psub->d = psub->e = psub->f = psub->g = psub->h = psub->i = psub->j = 0.0;
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
    evrPatternWF_s.header_s.type    = EVR_QUEUE_PATTERN;
    evrPatternWF_s.header_s.version = EVR_QUEUE_PATTERN_VERSION;
    evrPatternWF_s.modifier_a[0]    = psub->d;
    evrPatternWF_s.modifier_a[1]    = psub->e;
    evrPatternWF_s.modifier_a[2]    = psub->f;
    evrPatternWF_s.modifier_a[3]    = psub->g;
    evrPatternWF_s.modifier5        = psub->h;
    evrPatternWF_s.bunchcharge      = psub->i;
#ifdef __rtems__
    if (psub->dpvt) {
      EvgDataBufferUpdate((EgCardStruct *)psub->dpvt,
                          (epicsUInt32 *)&evrPatternWF_s,
                          sizeof(evrQueuePattern_ts)/sizeof(epicsUInt32));
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
        sSubRecord *        psub        read       point to subroutine record

  Rem:  Subroutine for IOC:LOCA:UNIT:PATTERNSTATE

  Inputs:
       A - Error Flag from mpgPatternProc
       B - Counter Reset Flag
       C - Spare
     
  Outputs:
       D - Number of waveforms processed by mpgPatternPnet
       E - Number of times D has rolled over
       F - Number of bad waveforms
           The following G-I outputs are pop'ed by a call to proc. evrQueueCounts()
       G - Number of times ISR tried to send a message
       H - Number of times G has rolled over
       I - Number of times ISR failed to send a message
       J - Number of pulse Id 6min 2sec cycles that have transpired since reset
       K - Number of unexpected unsynchronized pulse IDs
       L - Number of unexpected unsynchronized modulo720s
       M - Number of seq check 1 errors
       N - Number of seq check 2 errors
       O - Number of seq check 3 errors
       VAL = Last Error flag (see mpgPatternProc for values)

  Side: File-scope counters may be reset.
  
  Ret:  0 = OK

==============================================================================*/
static long mpgPatternState(sSubRecord *psub)
{
  psub->val = psub->a;
  if (psub->b > 0.5) {
    psub->b = 0.0;
    msgCount              = 0;
    msgRolloverCount      = 0;
    patternErrCount       = 0;
    resyncCount           = 0;
    seqCheck1ErrCount     = 0;
    seqCheck2ErrCount     = 0;
    seqCheck3ErrCount     = 0;
    pulseIDSyncErrCount   = 0;
    modulo720SyncErrCount = 0;
    evrQueueCountReset(EVR_QUEUE_PNET);
  }
  psub->d = msgCount;          /* # waveforms processed since boot/reset */
  psub->e = msgRolloverCount;  /* # time msgCount reached EVR_MAX_INT    */
  psub->f = patternErrCount;
  evrQueueCounts (EVR_QUEUE_PNET,&psub->g,&psub->h,&psub->i); 
  psub->j = resyncCount;       /* # of times 6min 2s rollover happened since boot/reset */
  psub->k = pulseIDSyncErrCount;
  psub->l = modulo720SyncErrCount;
  psub->m = seqCheck1ErrCount;
  psub->n = seqCheck2ErrCount;
  psub->o = seqCheck3ErrCount;
  return 0;
}

/*=============================================================================

  Name: mpgPatternCheck

  Abs:  360Hz Processing
        Check to see if current beam pulse is to be used in any
		current event measurement definition.
		Keep running measurement count.
                Keep done flag.
		Keep pattern match flag (val).
		

  Args: Type	            Name        Access	   Description
        ------------------- -----------	---------- ----------------------------
        sSubRecord *        psub        read       point to sSub subroutine record

  Rem:   Subroutine for PATTERNCHECK

  Inputs
        INPA - Spare
        INPB - Beam Code
        INPC - Spare 
        INPD - Pnet Modifier 1
        INPE - Pnet Modifier 2
        INPF - Pnet Modifier 3 
        INPG - Pnet Modifier 4 
        INPH - Modifier 5 without EDEF bits       
        INPI - EDEF Beam Code
        INPJ - Spare (ie for future beam codes)
        INPK - Spare (ie for future beam codes)
        INPL - Spare (ie for future beam codes)
        INPM - EDEF INCLUSION1
        INPN - EDEF INCLUSION2
        INPO - EDEF INCLUSION3
        INPP - EDEF INCLUSION4
        INPQ - EDEF INCLUSION5
        INPR - EDEF EXCLUSION1
        INPS - EDEF EXCLUSION2
        INPT - EDEF EXCLUSION3
        INPU - EDEF EXCLUSION4
        INPV - EDEF EXCLUSION5
        INPW - Beam Code SEVR
        INPX - EDEF CNTMAX (avg cnt * meas cnt)
		
  Outputs
	   VAL = Pattern match = 1
     	 no match = 0; enable/disable for bsacMeasCount
           Y   = Measurement Count
	   Z   = Done =1; not done measuring = 0
  Ret:  none

==============================================================================*/
static long mpgPatternCheck(sSubRecord *psub)
{
  psub->val = 0;

  /* Allow for X = -1 = forever */
  if ((psub->y >= psub->x) && (psub->x > 0)) {
    /* we're done with measurement */
    psub->z=1;
  }
  
  /* are we done? - if so exit now */
  /* Note: the sequence disables this record upon DONE, so this shouldn't happen */
  if (psub->z) return 0;

  /* check for bad data - do nothing with this pulse and return bad status */
  if (psub->w) return(-1);
  
  /* if Pattern beam code == EDEF beamcode.. */
  if (psub->b==psub->i) {
	/* check inclusion mask */
	DEBUGPRINT(DP_DEBUG, mpgPatternFlag,
                   ("\t mpgPatternCheck: Beam Codes match \n"));	  
	if (    (((unsigned long)psub->d & (unsigned long)psub->m) ==
                 (unsigned long)psub->m) &&
                (((unsigned long)psub->e & (unsigned long)psub->n) ==
                 (unsigned long)psub->n) &&
                (((unsigned long)psub->f & (unsigned long)psub->o) ==
                 (unsigned long)psub->o) &&
                (((unsigned long)psub->g & (unsigned long)psub->p) ==
                 (unsigned long)psub->p) &&
                (((unsigned long)psub->h & (unsigned long)psub->q) ==
                 (unsigned long)psub->q)) {
          DEBUGPRINT(DP_DEBUG, mpgPatternFlag,
                     ("mpgPatternCheck: inclusion match\n"));	  
	  if (  (((unsigned long)psub->r & (unsigned long)psub->d) == 0) &&
                (((unsigned long)psub->s & (unsigned long)psub->e) == 0) &&
                (((unsigned long)psub->t & (unsigned long)psub->f) == 0) &&
                (((unsigned long)psub->u & (unsigned long)psub->g) == 0) &&
                (((unsigned long)psub->v & (unsigned long)psub->h) == 0)) {
		DEBUGPRINT(DP_DEBUG, mpgPatternFlag,
                           ("mpgPatternCheck: exclusion match\n"));
		psub->val = 1;  /* pattern match*/
		psub->y++;      /* inc. meas. count */
          }
        }
  }
  return 0;
}

/*=============================================================================

  Name: mpgPatternSim

  Abs:  Simulate PNET data stream.
		
  Args: Type	            Name        Access	   Description
        ------------------- -----------	---------- ----------------------------
        subRecord *         psub        read       point to subroutine record

  Rem:  Subroutine for IOC:LOCA:UNIT:PATTERNSIM

  Side: Sends a message to the PNET pattern queue.
  
  Sub Inputs/ Outputs:
    Simulated inputs:
    D - MODIFIER1
    E - MODIFIER2
    F - MODIFIER3
    G - MODIFIER4
    J - BEAMCODE
    K - YY
  Ret:  0

==============================================================================*/
static long mpgPatternSim(subRecord *psub)
{ 
  epicsUInt32            modifier_a[EVR_PNET_MODIFIER_MAX];

/*------------- parse input into sub outputs ----------------------------*/

    modifier_a[0]    = ((epicsUInt32)psub->d) & 0xFFFFE000;
    modifier_a[0]   |= ((((epicsUInt32)psub->j) << 8) & 0x1F00);
    modifier_a[0]   |= ( ((epicsUInt32)psub->k) & YY_BIT_MASK);
    modifier_a[1]    = psub->e;
    modifier_a[2]    = psub->f;
    modifier_a[3]    = psub->g;

    /* Send the pattern to the PNET pattern queue */
    pnetSend(modifier_a);
    return 0;
}
epicsRegisterFunction(mpgPatternInit);
epicsRegisterFunction(mpgPatternPnetInit);
epicsRegisterFunction(mpgPatternPnet);
epicsRegisterFunction(mpgPatternProcInit);
epicsRegisterFunction(mpgPatternProc);
epicsRegisterFunction(mpgPatternState);
epicsRegisterFunction(mpgPatternCheck);
epicsRegisterFunction(mpgPatternSim);

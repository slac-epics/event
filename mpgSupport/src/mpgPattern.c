/*=============================================================================

  Name: mpgPattern.c
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
#include "evrMessage.h"       /* for EVR_MESSAGE_PATTERN*   */
#include "evrTime.h"          /* for evrTime* prototypes    */
#include "evrPattern.h"       /* for PATTERN* defines       */
#include "pnetSeqCheck.h"     /* for pnetSeqCheck* protos   */
#include "mpgEvent.h"         /* for mpgEvent* protos       */
#include "drvMrfEg.h"         /* for EgDataBufferLoad proto */


#define EVG_DELTA_TIME 0.008333333333 /* 1/120sec for setting time 3 pulses ahead */

#ifdef DEBUG_PRINT
  /* ENUM with values: DP_FATAL, DP_ERROR, DP_WARN, DP_INFO, DP_DEBUG
     Anything less than or equal to mpgPatternFlag gets printed out. Can
     be set to 0 here and altered via prompt or in st.cmd file */
int mpgPatternFlag = DP_INFO;
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
#ifdef __rtems__
static unsigned int dataBufferSize = sizeof(evrMessagePattern_ts)/sizeof(epicsUInt32);
#endif

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
  DBADDR *wfAddr = dbGetPdbAddrFromLink(&psub->inpa);
  /*
   * inpa must be a DB link and must be the proper type.
   */
  if (wfAddr && (wfAddr->field_type == DBF_ULONG))
    psub->dpvt = (void *)wfAddr;
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
       L - Pulse ID (set invalid if error)
       VAL = Error flag:
             0 = OK
             Seq check 1 error
             Seq check 2 error
             Seq check 3 error
             Invalid Waveform
             MPG IPLing
             Pulse ID   not synchronized with the SLC MPG
             Modulo 720 not synchronized with the SLC MPG

  Side: None
  
  Ret:  0 = OK, -1 = ERROR

==============================================================================*/
static long mpgPatternPnet(subRecord *psub)
{
  DBADDR                *wfAddr = (DBADDR *)psub->dpvt;
  epicsUInt32           *pnet_a;
  epicsUInt32            modifier1;
  int                    pulsid         = psub->l + 1.1;
  int                    modulo720Count = psub->i + 1.1;
  int                    errFlag        = PATTERN_OK;
  int                    pulsid_resync;
  unsigned char          traveling_one;
  unsigned char          timeslot;
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

  if (psub->b || (!wfAddr) || (psub->c != EVR_PNET_MODIFIER_MAX) ||
      (!(pnet_a = (epicsUInt32 *)wfAddr->pfield))) {
    if (psub->b) patternErrCount++;
    psub->e = psub->f = psub->g = psub->j = 0.0;
    modifier1     = 0;
    errFlag       = PATTERN_INVALID_WF;
    pulseIDSync   = 0;
    modulo720Sync = 0;
    seqCheckSync  = 0;
  } else {
    /* read from the waveform now */
    dbScanLock(wfAddr->precord);
    /* set outputs to the modifiers */
    modifier1 = pnet_a[0] & (~0x80000000);
    psub->e = (double)(pnet_a[1]);
    psub->f = (double)(pnet_a[2]);
    psub->g = (double)(pnet_a[3]);
    /* beamcode decoded from modifier 1*/
    psub->j = (double)((modifier1 >> 8) & BEAMCODE_BIT_MASK);
  
    /* Check if SLC MPG is rebooting */
    if (modifier1 & MPG_IPLING) {
      errFlag       = PATTERN_MPG_IPLING;
      pulseIDSync   = 0;
      modulo720Sync = 0;
      seqCheckSync  = 0;
    /* Do sync checking if requested */
    } else if (psub->h > 0.5) {
      traveling_one = (unsigned char)(pnet_a[1] & TIMESLOT_MASK);
      timeslot      = (unsigned char)((pnet_a[3] >> 29) & TIMESLOT_VAL_MASK);
      if (pnetSeqCheckData1(traveling_one, seqCheckSync)) { /* "traveling 1" check */
        errFlag = PATTERN_SEQ_CHECK1_ERR;
        seqCheck1ErrCount++;
      } else if (pnetSeqCheckData2(timeslot, seqCheckSync)) { /* 0-5 looping counter check */
        errFlag = PATTERN_SEQ_CHECK2_ERR;
        seqCheck2ErrCount++;
      } else if (pnetSeqCheckData3(traveling_one, timeslot)) {/* check that two checksum values stay in sync */
        errFlag = PATTERN_SEQ_CHECK3_ERR;
        seqCheck3ErrCount++;
      }
      /* Both pulse ID and modulo 720 will need to be resync'ed too */
      if (errFlag) {
        pulseIDSync   = 0;
        modulo720Sync = 0;
      }
      seqCheckSync = 1;

      /* Check if modulo-720 can be resynchronized on this pulse. */
      if (modifier1 & MODULO720_MASK) {
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
        errFlag        = PATTERN_MODULO720_NO_SYNC;
      } else if (modulo720Count >= 720) {
        /* If we should be sync'ed but are not, update a counter */
        modulo720SyncErrCount++;
        errlogPrintf("mpgPatternPnet: Modulo 720 not synchronized, modulo720Count = %d\n",
                     modulo720Count);
        modulo720Count = 0;
        modulo720Sync  = 0;
        errFlag        = PATTERN_MODULO720_NO_SYNC;
        /* Assume pulse ID is also out-of-sync */
        pulseIDSync    = 0;
      }

      /* Check for a non-zero value in the 4 bit pulsid_resync field of Pnet
         data.  If found, it resets upper 4 bits of the pulse ID (lower bit
         will be zero).  Otherwise, the pulse ID is set to 1 if the last value
         is the rollover value.  Otherwise, the pulse ID is simply incremented.*/
      pulsid_resync = (modifier1 >> 3) & PULSEID_RESYNC;
      if (pulsid_resync) {
        /* If we should be sync'ed but are not, update a counter */
        if ((pulseIDSync) && (pulsid != pulsid_resync)) {
          pulseIDSyncErrCount++;
          errlogPrintf("mpgPatternPnet: Pulse IDs not synchronized, MPG = %d, EVG = %d\n",
                       pulsid_resync, pulsid);
          /* Assume modulo 720 out-of-sync too unless the RESYNC bit is set */
          if (!(modifier1 & MODULO720_MASK)) {
            modulo720Sync = 0;
            errFlag       = PATTERN_MODULO720_NO_SYNC;
          }
        }
        pulseIDSync = 1;
        pulsid = pulsid_resync;
      } else if (!pulseIDSync) {
        errFlag = PATTERN_PULSEID_NO_SYNC;
      } else if (pulsid > PULSEID_MAX) {
        /* Aha. Time to rollover, so THIS pulse will be set back to zero */
        DEBUGPRINT(DP_DEBUG, mpgPatternFlag, ("mpgPatternPnet: Rollover, pulsid+1 = %d\n",
                                              pulsid));
        pulsid = 0;
        resyncCount++;
      }
    } else {
      pulseIDSync   = 0;
      modulo720Sync = 0;
      seqCheckSync  = 0;
    }
    dbScanUnlock(wfAddr->precord);
  }
  /* Tell EVRs if data is out-of-sync */
  if (errFlag) modifier1 |= MPG_IPLING;
  if (pulsid > PULSEID_MAX) pulsid = 0;
  if (modulo720Count >= 720) {
    modifier1 |= MODULO720_MASK;
    modulo720Count = 0;
  }
  psub->d   = (double)modifier1;
  psub->i   = (double)modulo720Count;
  psub->l   = (double)pulsid;
  psub->val = (double)errFlag;
  if (errFlag) return -1;
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
        None.
     
  Outputs:
    DPVT - Pointer to EVG Card Structure
  
==============================================================================*/ 
static int mpgPatternProcInit(sSubRecord *psub)
{
  /*
   * For RTEMS, initialize the EVG data buffer size
   * and get the pointer to the card structure.
   */
#ifdef __rtems__
  EgDataBufferInit(0, dataBufferSize);
  psub->dpvt = (EgCardStruct *)EgGetCardStruct(0);
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
       A - Spare
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
       K - EDAVGDONEN-3
       L - Pulse ID
	   U - edef Init Mask
	   V - edef MeasSevr Minor Mask
       W - edef MeasSevr Major Mask
       Z - Modulo 720 Flag    
  Outputs:
       L - Pulse ID
       VAL = Error flag:
             0 = OK
             Seq check 1 error
             Seq check 2 error
             Seq check 3 error
             Invalid Waveform
             MPG IPLing
             Pulse ID   not synchronized with the SLC MPG
             Modulo 720 not synchronized with the SLC MPG
             Invalid Timestamp

  Side: Pattern is sent out via the EVG.
  
  Ret:  0 = OK, -1 = ERROR

==============================================================================*/
static long mpgPatternProc(sSubRecord *psub)
{
  evrMessagePattern_ts   evrPatternWF_s;
  epicsTimeStamp         prev_time;
  double                 delta_time;

  psub->val = psub->b;
  psub->z  = (double)(((epicsUInt32)psub->d) & MODULO720_MASK);
  
  /* Initialize EVR timestamp to system time */
  if (epicsTimeGetCurrent(&evrPatternWF_s.time)) {
    evrTimePut(0, epicsTimeERROR);
    psub->val = PATTERN_INVALID_TIMESTAMP;
    return -1;
  }
  /* The time is for 3 pulses in the future - add 1/120sec */
  epicsTimeAddSeconds(&evrPatternWF_s.time, EVG_DELTA_TIME);
  /* Overlay the pulse ID into the lower 17 bits of the nsec field */
  evrTimePutPulseID(&evrPatternWF_s.time, (unsigned int)psub->l);

  /* The timestamp must ALWAYS be increasing.  Check if this time
     is less than the previous time (due to rollover) and adjust up
     slightly using bit 17 (the lower-most bit of the top 15 bits). */
  if (!evrTimeGet(&prev_time, evrTimeNext3)) {
    delta_time = epicsTimeDiffInSeconds(&evrPatternWF_s.time, &prev_time);
    if (delta_time < 0.0) {
      epicsTimeAddSeconds(&evrPatternWF_s.time, PULSEID_BIT17/NSEC_PER_SEC);
      evrTimePutPulseID(&evrPatternWF_s.time, (unsigned int)psub->l);
    }
  }
  /* write to evr timestamp table and error check */
  if (evrTimePut(&evrPatternWF_s.time, epicsTimeOK)) {
    psub->val = PATTERN_INVALID_TIMESTAMP;
  }
  /* Timestamp done - now fill in the rest of the pattern */
  evrPatternWF_s.header_s.type        = EVR_MESSAGE_PATTERN;
  evrPatternWF_s.header_s.version     = EVR_MESSAGE_PATTERN_VERSION;
  evrPatternWF_s.pnet_s.modifier_a[0] = psub->d;
  evrPatternWF_s.pnet_s.modifier_a[1] = psub->e;
  evrPatternWF_s.pnet_s.modifier_a[2] = psub->f;
  evrPatternWF_s.pnet_s.modifier_a[3] = psub->g;
  evrPatternWF_s.modifier5            = psub->h;
  evrPatternWF_s.bunchcharge          = psub->i;
  evrPatternWF_s.edefInitMask         = psub->u;
  evrPatternWF_s.edefAvgDoneMask      = psub->k;
  evrPatternWF_s.edefMinorMask        = psub->v; 
  evrPatternWF_s.edefMajorMask        = psub->w;
#ifdef __rtems__
  if (psub->dpvt) {
    EgDataBufferLoad(((EgCardStruct *)psub->dpvt)->pEg,
                     (epicsUInt32 *)&evrPatternWF_s, dataBufferSize);
  }
#endif
  if (psub->val) return -1;
  return 0;
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
           The following G-J outputs are pop'ed by a call to evrMessageCounts()
       G - Number of times ISR wrote a message
       H - Number of times G has rolled over
       I - Number of times ISR overwrote a message
       J - Number of times ISR failed to get a mutex lock
       K - Number of unexpected unsynchronized pulse IDs
       L - Number of unexpected unsynchronized modulo720s
       M - Number of seq check 1 errors
       N - Number of seq check 2 errors
       O - Number of seq check 3 errors
       P - Number of pulse Id 6min 2sec cycles that have transpired since reset
       Q - Number of lock errors in event processing
       R - Number of times a seq ram was still busy
       S - Number of times a seq ram was not in single mode
       T - Number of times a seq ram was still active
       U - Number of bad patterns in event processing
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
    evrMessageCountReset(EVR_MESSAGE_PNET);
    mpgEventCountReset();
  }
  psub->d = msgCount;          /* # waveforms processed since boot/reset */
  psub->e = msgRolloverCount;  /* # time msgCount reached EVR_MAX_INT    */
  psub->f = patternErrCount;
  evrMessageCounts(EVR_MESSAGE_PNET,&psub->g,&psub->h,&psub->i,&psub->j); 
  psub->k = pulseIDSyncErrCount;
  psub->l = modulo720SyncErrCount;
  psub->m = seqCheck1ErrCount;
  psub->n = seqCheck2ErrCount;
  psub->o = seqCheck3ErrCount;
  psub->p = resyncCount;       /* # of times 6min 2s rollover happened since boot/reset */
  mpgEventCounts(&psub->q,&psub->r,&psub->s,&psub->t,&psub->u);
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
        INPB - Beam Code
        INPC - EDEF AVGCNT 
        INPD - Pnet Modifier 1
        INPE - Pnet Modifier 2
        INPF - Pnet Modifier 3 
        INPG - Pnet Modifier 4 
        INPH - Modifier 5 without EDEF bits       
        INPI - EDEF Beam Code
        INPJ - Spare (ie for future beam codes)
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
        INPX - EDEF MEASCNT (= -1 means forever)
		
  Outputs
	   VAL = Pattern match = 1
     	 no match = 0; enable/disable for bsacMeasCount
           A   = Counter before CTRL is turned off
		   K   = Done averaging = 1; not done averaging = 0
		   L   = Average Count
           Y   = Measurement Count (= -1 means forever)
		   Z   = Done =1; not done measuring = 0
  Ret:  none

==============================================================================*/
static long mpgPatternCheck(sSubRecord *psub)
{
  psub->val = 0;
  /* Allow for X = -1 = forever */
  if ((psub->y >= psub->x) && (psub->x > 0.5)) {
    /* we're done with measurement */
    psub->a++;
    /* Check is done for the N-3 pulse - don't turn CTRL OFF
       until 3 pulse from now */
    if (psub->a > 3) psub->z = 1;
  }
  
  /* are we done? - if so exit now */
  if (psub->a) return 0;

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
		if (psub->x > 0.5) psub->y++;      /* inc. meas. count */
		else               psub->y = -1;
		psub->l++;                         /* inc. avg count */
		if (psub->l >=psub->c) {
		  psub->k = 1;                     /* averaging done */
		  psub->l = 0;                     /* reset avg count */
		}
		else               psub->k   = 0;  /* not done averaging */
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
    L - PULSE ID (not used)
  Ret:  Return from evrMessageWrite.

==============================================================================*/
static long mpgPatternSim(subRecord *psub)
{ 
  evrMessage_tu evrMessage_u;

/*------------- parse input into sub outputs ----------------------------*/

  evrMessage_u.pnet_s.modifier_a[0]  = ((epicsUInt32)psub->d) & 0xFFFF8000;
  evrMessage_u.pnet_s.modifier_a[0] |= ((((epicsUInt32)psub->j) << 8) & 0x1F00);
  evrMessage_u.pnet_s.modifier_a[0] |= ( ((epicsUInt32)psub->k) & YY_BIT_MASK);
  evrMessage_u.pnet_s.modifier_a[1]  = psub->e;
  evrMessage_u.pnet_s.modifier_a[2]  = psub->f;
  evrMessage_u.pnet_s.modifier_a[3]  = psub->g;

  /* Send the pattern to the PNET pattern queue */
  return(evrMessageWrite(EVR_MESSAGE_PNET, &evrMessage_u));
}
epicsRegisterFunction(mpgPatternPnetInit);
epicsRegisterFunction(mpgPatternPnet);
epicsRegisterFunction(mpgPatternProcInit);
epicsRegisterFunction(mpgPatternProc);
epicsRegisterFunction(mpgPatternState);
epicsRegisterFunction(mpgPatternCheck);
epicsRegisterFunction(mpgPatternSim);

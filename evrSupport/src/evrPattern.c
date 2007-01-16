/*=============================================================================
 
  Name: evrPattern.c
           evrPatternInit      - General Initialization
           evrPatternProcInit  - Pattern Setup Initialization
           evrPatternProc      - 360Hz Pattern Setup
           evrPatternState     - Pattern Processing State and Diagnostics

  Abs: This file contains all subroutine support for evr Pattern processing
       records.
       
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

  Auth:  
  Rev:  
-----------------------------------------------------------------------------*/

#include "copyright_SLAC.h"	/* SLAC copyright comments */
 
/*-----------------------------------------------------------------------------
 
  Mod:  (newest to oldest)  
 
=============================================================================*/
#include "debugPrint.h"
#ifdef  DEBUG_PRINT
#include <stdio.h>
  int evrSubFlag = DP_DEBUG;
#endif

/* c includes */

#include "subRecord.h"        /* for struct subRecord      */
#include "registryFunction.h" /* for epicsExport           */
#include "epicsExport.h"      /* for epicsRegisterFunction */
#include "dbAccess.h"         /* dbGetPdbAddrFromLink      */

#include "evrMessage.h"       /* for EVR_MESSAGE_PATTERN*  */
#include "evrTime.h"          /* evrTime* prototypes       */
#include "evrPattern.h"       /* for PATTERN* defines      */

static unsigned int msgCount         = 0; /* # waveforms processed since boot/reset */ 
static unsigned int msgRolloverCount = 0; /* # time msgCount reached EVR_MAX_INT    */ 
static unsigned int patternErrCount  = 0; /* # bad PATTERN waveforms */
static unsigned int syncErrCount     = 0; /* # out-of-sync patterns  */

/*=============================================================================

  Name: evrPatternInit

  Abs:  General purpose initialization required since all subroutine records
   require a non-NULL init routine even if no initialization is required.
   Note that most subroutines in this file use this routine as an init
   routine.  If init logic is needed for a specific subroutine, create a
   new routine for it - don't modify this one.
  
==============================================================================*/ 
static int evrPatternInit(subRecord *psub)
{
  return 0;
}

/*=============================================================================

  Name: evrPatternProcInit

  Abs:  Initialization for 360Hz pattern processing.  Initialize pointer
        to waveform data.
		
  Args: Type	            Name        Access	   Description
        ------------------- -----------	---------- ----------------------------
        subRecord *         psub        read       point to subroutine record

  Rem:  Initialization subroutine for IOC:LOCA:UNIT:PATTERN

  Side: None.

  Sub Inputs/ Outputs:
   Inputs:
    A - PATTERNDATA.VAL
   Outputs:
    DPVT - pointer to waveform data

  Ret:  0 = OK, -1 = Error.
  
==============================================================================*/ 
static int evrPatternProcInit(subRecord *psub)
{
  DBADDR      *wfAddr = dbGetPdbAddrFromLink(&psub->inpa);

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

  Name: evrPatternProc

  Abs:  360Hz Processing, Grab 7 pattern longs from PATTERNDATA waveform and
        parse into MODIFIER1-5 longins and two longin evr timestamps.

        Then parse out BEAMCODE from MODIFIER 1 and
	PULSEID from the lower 17 bits of timestamp nsec integer 
		
  Args: Type	            Name        Access	   Description
        ------------------- -----------	---------- ----------------------------
        subRecord *         psub        read       point to subroutine record

  Rem:  Subroutine for IOC:LOCA:UNIT:PATTERN

  Side: Waveform order:
        4 PNET 32 bit unsigned integers (MODIFIER 1-4)
		1 EVR 32 bit unsigned integer (MODIFIER 5)
		2 EVR timestamp 32 bit unsigned integers 
		1 bunchcharge 32 bit integer (picoCoulombs)
  Error checking:
Waveform record is invalid or has invalid size or type set record invalid, pulse ID to 0, 
and EVR timestamp/status to last-good-time/invalid
Error writing EVR timestamp set record invalid and pulse ID to 0

  Sub Inputs/ Outputs:
   Inputs:
    A - PATTERNDATA.VAL
    B - PATTERNDATA.SEVR
    C - PATTERNDATA.NORD
   Outputs:
	D - MODIFIER1N-3 (PNET)
	E - MODIFIER2N-3 (PNET)
	F - MODIFIER3N-3 (PNET)
	G - MODIFIER4N-3 (PNET)
	H - MODIFIER5N-3 (evr)
	I - BUNCHARGEN-3 (evr)
	J - BEAMCODEN-3 (decoded from PNET bits 8-12, MOD1 8-12)
        K - Modulo 720 Flag
	L - PULSEIDN-3  (decoded from PNET bits, 17)   
        VAL = Error flag:
             OK
             Invalid Waveform
             Invalid Waveform Header
             Invalid Timestamp
             MPG IPLing
   Output to evr timestamp table
  Ret:  0

==============================================================================*/
static long evrPatternProc(subRecord *psub)
{
  DBADDR                *wfAddr = (DBADDR *)psub->dpvt;
  evrMessagePattern_ts  *evrPatternWF_ps;
  int                    modifier1;
  int                    errFlag = PATTERN_OK;

  /* Keep a count of messages and reset before overflow */
  if (msgCount < EVR_MAX_INT) {
    msgCount++;
  } else {
    msgRolloverCount++;
    msgCount = 0;
  }
  /* if waveform is invalid or has invalid size or type            */
  /*   set record invalid; pulse id =0, and                        */
  /*   evr timestamp/status to last good time with invalid status */
  if (psub->b || (!wfAddr) ||
      (psub->c < (sizeof(evrMessagePattern_ts)/sizeof(epicsUInt32))) ||
      (!(evrPatternWF_ps = (evrMessagePattern_ts *)wfAddr->pfield))) {
    if (psub->b) patternErrCount++;
    psub->e = psub->f = psub->g = psub->h = psub->i = psub->j = 0.0;
    if (psub->val) psub->k = 0;
    else           psub->k = 1;
    psub->l   = PULSEID_INVALID;
    errFlag   = PATTERN_INVALID_WF;
    modifier1 = MPG_IPLING;
    evrTimePut(0, epicsTimeERROR);
  } else {
    dbScanLock(wfAddr->precord);
    /* error if the waveform has an invalid type or version */
    if ((evrPatternWF_ps->header_s.type    != EVR_MESSAGE_PATTERN  ) ||
        (evrPatternWF_ps->header_s.version != EVR_MESSAGE_PATTERN_VERSION)) {
      patternErrCount++;
      psub->e = psub->f = psub->g = psub->h = psub->i = psub->j = 0.0;
      if (psub->val) psub->k = 0;
      else           psub->k = 1;
      psub->l   = PULSEID_INVALID;
      errFlag   = PATTERN_INVALID_WF_HDR;
      modifier1 = MPG_IPLING;
      evrTimePut(0, epicsTimeERROR);
    } else {
      
      /* set outputs to the modifiers */
      modifier1 =        evrPatternWF_ps->pnet_s.modifier_a[0];
      psub->e = (double)(evrPatternWF_ps->pnet_s.modifier_a[1]);
      psub->f = (double)(evrPatternWF_ps->pnet_s.modifier_a[2]);
      psub->g = (double)(evrPatternWF_ps->pnet_s.modifier_a[3]);
      psub->h = (double)(evrPatternWF_ps->modifier5);
      psub->i = (double)(evrPatternWF_ps->bunchcharge);
      /* beamcode decoded from modifier 1*/
      psub->j = (double)((modifier1 >> 8) & BEAMCODE_BIT_MASK);
      /* modulo720 decoded from modifier 1*/
      psub->k = (double)(modifier1 & MODULO720_MASK);
      /* decode pulseid field to output j (keep lower 17 bits) */
      psub->l = (double)(evrPatternWF_ps->time.nsec & LOWER_17_BIT_MASK);
          /* write to evr timestamp table and error check */
      if (evrTimePut (&evrPatternWF_ps->time, epicsTimeOK)) {
        errFlag = PATTERN_INVALID_TIMESTAMP;
      /* Check if EVG reporting a problem  */
      } else if (modifier1 & MPG_IPLING) {
        errFlag = PATTERN_MPG_IPLING;
        syncErrCount++;
      }
    }
    dbScanUnlock(wfAddr->precord);
  }
  psub->d   = (double)modifier1;
  psub->val = (double)errFlag;
  if (errFlag) return -1;
  return 0;
}

/*=============================================================================

  Name: evrPatternState

  Abs:  Access to Last Pattern State and Pattern Diagnostics.
        This subroutine is for status only and should update at a low
        rate like 1Hz.

  Args: Type                Name        Access     Description
        ------------------- ----------- ---------- ----------------------------
        subRecord *         psub        read       point to subroutine record

  Rem:  Subroutine for IOC:LOCA:UNIT:PATTERNDIAG

  Inputs:
       A - Error Flag from evrPatternProc
       B - Counter Reset Flag
       C - Spare
     
  Outputs:
       D - Number of waveforms processed by this subroutine
       E - Number of times D has rolled over
       F - Number of bad waveforms
           The following G-J outputs are pop'ed by a call to evrMessageCounts()
       G - Number of times ISR wrote a message
       H - Number of times G has rolled over
       I - Number of times ISR overwrote a message
       J - Number of times ISR had a mutex lock error
       K - Number of unsynchronized patterns
       VAL = Last Error flag (see evrPatternProc for values)

  Side: File-scope counters may be reset.
  
  Ret:  0 = OK

==============================================================================*/
static long evrPatternState(subRecord *psub)
{
  psub->val = psub->a;
  if (psub->b > 0.5) {
    psub->b = 0.0;
    msgCount              = 0;
    msgRolloverCount      = 0;
    patternErrCount       = 0;
    syncErrCount          = 0;
    evrMessageCountReset(EVR_MESSAGE_PATTERN);
  }
  psub->d = msgCount;          /* # waveforms processed since boot/reset */
  psub->e = msgRolloverCount;  /* # time msgCount reached EVR_MAX_INT    */
  psub->f = patternErrCount;
  evrMessageCounts(EVR_MESSAGE_PATTERN,&psub->g,&psub->h,&psub->i,&psub->j); 
  psub->k = syncErrCount;
  return 0;
}

/*=============================================================================

  Name: evrPatternSim

  Abs:  Simulate EVR data stream. Used for simulation/debugging only.
		
  Args: Type	            Name        Access	   Description
        ------------------- -----------	---------- ----------------------------
        subRecord *         psub        read       point to subroutine record

  Rem:  Subroutine for IOC:LOCA:UNIT:PATTERNSIM

  Side: Sends a message to the EVR pattern queue.
  
  Sub Inputs/ Outputs:
    Simulated inputs:
    D - MODIFIER1
    E - MODIFIER2
    F - MODIFIER3
    G - MODIFIER4
    H - MODIFIER5  
    I - BUNCHARGE  
    J - BEAMCODE
    K - YY
  Ret:  Return from evrMessageWrite.

==============================================================================*/
static long evrPatternSim(subRecord *psub)
{ 
  evrMessage_tu          evrMessage_u;
  epicsTimeStamp         prev_time;
  double                 delta_time;

/*------------- parse input into sub outputs ----------------------------*/

  if (epicsTimeGetCurrent(&evrMessage_u.pattern_s.time)) return -1;
  /* The time is for 3 pulses in the future - add 1/120sec */
  epicsTimeAddSeconds(&evrMessage_u.pattern_s.time, 1/120);
  /* Overlay the pulse ID into the lower 17 bits of the nsec field */
  evrTimePutPulseID(&evrMessage_u.pattern_s.time, (unsigned int)psub->l);

  /* The timestamp must ALWAYS be increasing.  Check if this time
     is less than the previous time (due to rollover) and adjust up
     slightly using bit 17 (the lower-most bit of the top 15 bits). */
  if (!evrTimeGet(&prev_time, evrTimeNext3)) {
    delta_time = epicsTimeDiffInSeconds(&evrMessage_u.pattern_s.time,
                                        &prev_time);
    if (delta_time < 0.0) {
      epicsTimeAddSeconds(&evrMessage_u.pattern_s.time,
                          PULSEID_BIT17/NSEC_PER_SEC);
      evrTimePutPulseID(&evrMessage_u.pattern_s.time, (unsigned int)psub->l);
    }
  }
  /* Timestamp done - now fill in the rest of the pattern */
  evrMessage_u.pattern_s.header_s.type         = EVR_MESSAGE_PATTERN;
  evrMessage_u.pattern_s.header_s.version      = EVR_MESSAGE_PATTERN_VERSION;
  evrMessage_u.pattern_s.pnet_s.modifier_a[0]  = ((epicsUInt32)psub->d) & 0xFFFFE000;
  evrMessage_u.pattern_s.pnet_s.modifier_a[0] |= ((((epicsUInt32)psub->j) << 8) & 0x1F00);
  evrMessage_u.pattern_s.pnet_s.modifier_a[0] |= ( ((epicsUInt32)psub->k) & YY_BIT_MASK);
  evrMessage_u.pattern_s.pnet_s.modifier_a[1]  = psub->e;
  evrMessage_u.pattern_s.pnet_s.modifier_a[2]  = psub->f;
  evrMessage_u.pattern_s.pnet_s.modifier_a[3]  = psub->g;
  evrMessage_u.pattern_s.modifier5             = psub->h;
  evrMessage_u.pattern_s.bunchcharge           = psub->i;

  /* Send the pattern to the EVR pattern queue */
  return(evrMessageWrite(EVR_MESSAGE_PATTERN, &evrMessage_u));
}
epicsRegisterFunction(evrPatternInit);
epicsRegisterFunction(evrPatternProcInit);
epicsRegisterFunction(evrPatternProc);
epicsRegisterFunction(evrPatternState);
epicsRegisterFunction(evrPatternSim);

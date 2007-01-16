/*=============================================================================
 
  Name: evrTime.c

  Abs: This file contains all subroutine support for evr time processing
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
  int evrTimeFlag = DP_DEBUG;
#endif


/* c includes */

#include <subRecord.h>        /* for struct subRecord      */
#include <registryFunction.h> /* for epicsExport           */
#include <epicsExport.h>      /* for epicsRegisterFunction */
#include <epicsTime.h>        /* epicsTimeStamp and protos */
#include <epicsGeneralTime.h> /* generalTimeTpRegister     */
#include <epicsMutex.h>       /* epicsMutexId and protos   */
#include <alarm.h>            /* INVALID_ALARM             */

#include "evrTime.h"       

#define  EVR_TIME_OK 0
#define  EVR_TIME_INVALID 1

/* EVR Timestamp table */
typedef struct {

  epicsTimeStamp      time;   /* epics timestamp:                        */
                              /* 1st 32 bits = # of seconds since 1990   */
                              /* 2nd 32 bits = # of nsecs since last sec */
                              /*           except lower 17 bits = pulsid */
  int                 status; /* 0=OK; -1=invalid                        */
}evrTime_ts;
evrTime_ts evrTime_as[MAX_EVR_TIME];

/* EVR Time Timestamp RWMutex */
epicsMutexId evrTimeRWMutex_ps;

/*=============================================================================

  Name: evrTimeGetSystem

  Abs:  Returns system time and status of call
  
==============================================================================*/

static int evrTimeGetSystem (epicsTimeStamp  *epicsTime_ps, evrTimeId_te id)
{
  if ( epicsTimeGetCurrent (epicsTime_ps) ) {
	DEBUGPRINT(DP_ERROR, evrTimeFlag, 
			   ("evrTimeGetSystem: Unable to epicsTimeGetCurrent!\n"));	
	return epicsTimeERROR;
  }
  /* Set pulse ID to invalid */
  evrTimePutPulseID(epicsTime_ps, PULSEID_INVALID);

  return epicsTimeOK;
}

/*=============================================================================

  Name: evrTimeGet

  Abs:  Get the evr epics timestamp, defined as:
        1st integer = number of seconds since 1990  
        2nd integer = number of nsecs since last sec, except lower 17 bits=pulsid
        3rd integer = status, where 0=OK; -1=invalid   
		
  Args: Type     Name           Access	   Description
        -------  -------	---------- ----------------------------
  epicsTimeStamp * epicsTime_ps write  ptr to epics timestamp to be returned
  evrTimeId_te   id             read   timestamp pipeline index;
	                                  0=time associated w this pulse
					  1=time associated w next pulse
                                          2=time assoc'ed w pulse 2 ahead
                                          3=time assoc'ed w pulse 3 ahead

  Rem:  Routine to get the epics timestamp from the evr timestamp table
        that is populated from incoming broadcast from EVG

  Side: 

  Return:   -1=Failed; 0 = Success
==============================================================================*/

int evrTimeGet (epicsTimeStamp  *epicsTime_ps, evrTimeId_te id)
{
  int status;
  
  if ((id < 0) || (id > MAX_EVR_TIME) || (!evrTimeRWMutex_ps)) {
    DEBUGPRINT(DP_ERROR,evrTimeFlag,("evrTimeGet: Bad event! %d\n", id));
    return epicsTimeERROR;
  /* if the r/w mutex is valid, and we can lock with it, read requested time index */
  }
  if (epicsMutexLock(evrTimeRWMutex_ps)) return epicsTimeERROR;
  *epicsTime_ps = evrTime_as[id].time;
  status = evrTime_as[id].status;
  epicsMutexUnlock(evrTimeRWMutex_ps);
  
  return status; 
}

/*=============================================================================

  Name: evrTimePut

  Abs:  Set the evr timestamp, defined as:
        1st int = number of seconds since 1990  
        2nd int = number of nsecs since last sec, except lower 17 bits=pulsid
        3rd int = status, where 0=OK; -1=invalid   
		
  Args: Type     Name           Access	   Description
        -------  -------	---------- ----------------------------
  epicsTimeStamp *  epicsTime_ps   read    ptr to epics timestamp
        int      status            read    status

  Rem:  Routine to update the epics timestamp in the evr timestamp table
        that is populated from incoming broadcast from EVG

  Side: 

  Return:  -1=Failed; 0 = Success
==============================================================================*/

int evrTimePut (epicsTimeStamp  *epicsTime_ps, int status)
{
  if (evrTimeRWMutex_ps && (!epicsMutexLock(evrTimeRWMutex_ps))) {
    evrTime_as[evrTimeNext3].status = status;
    if (epicsTime_ps) evrTime_as[evrTimeNext3].time = *epicsTime_ps;
    epicsMutexUnlock(evrTimeRWMutex_ps);
  }
  /* invalid mutex id or error - must set status to invalid for the caller */
  else {
    evrTime_as[evrTimeNext3].status = epicsTimeERROR;
    DEBUGPRINT(DP_ERROR,evrTimeFlag,("evrTimePut: Bad TimeRWMutex! \n"));
    return epicsTimeERROR;
  }
  return epicsTimeOK;
}

/*=============================================================================

  Name: evrTimePutPulseID

  Abs:  Puts pulse ID in the lower 17 bits of the nsec field
        of the epics timestamp.
  
==============================================================================*/ 
int evrTimePutPulseID (epicsTimeStamp  *epicsTime_ps, unsigned int pulseID)
{
  epicsTime_ps->nsec &= UPPER_15_BIT_MASK;
  epicsTime_ps->nsec |= pulseID;
  if (epicsTime_ps->nsec >= NSEC_PER_SEC) {
    epicsTime_ps->secPastEpoch++;
    epicsTime_ps->nsec -= NSEC_PER_SEC;
    epicsTime_ps->nsec &= UPPER_15_BIT_MASK;
    epicsTime_ps->nsec |= pulseID;
  }
  return epicsTimeOK;
}

/*=============================================================================

  Name: evrTimeDiagInit

  Abs:  General purpose initialization required since all subroutine records
   require a non-NULL init routine even if no initialization is required.
   Note that most subroutines in this file use this routine as an init
   routine.  If init logic is needed for a specific subroutine, create a
   new routine for it - don't modify this one.
  
==============================================================================*/ 
static int evrTimeDiagInit(subRecord *psub)
{
  return epicsTimeOK;
}

/*=============================================================================

  Name: evrTimeDiag

  Abs:  Expose time from the table to pvs
  
  Outputs:
  A  secPastEpoch for timestamp 0, n (evrTime_as[0])
  B  nsec for timestamp 0, n
  C  status "
  D  secPastEpoch for timestamp 1, n-1 (evrTime_as[1])
  E  nsec for timestamp 1, n-1 (evrTime_as[1])
  F  status "
  G  secPastEpoch for timestamp 2, n-2 (evrTime_as[2])
  H  nsec for timestamp 2, n-2 (evrTime_as[2])
  I  status "
  J  secPastEpoch for timestamp 3, n-3 (evrTime_as[3])
  K  nsec for timestamp 3, n-3 (evrTime_as[3])
  L  status "
==============================================================================*/ 

static long evrTimeDiag (subRecord *psub)
{
  unsigned short n;
  double  *output_p = &psub->a;

  /* read evr timestamps in the pipeline*/
  if ((!evrTimeRWMutex_ps) || (epicsMutexLock(evrTimeRWMutex_ps)))
    return epicsTimeERROR;
  for (n=0;n<MAX_EVR_TIME;n++) {
    *output_p = (double)evrTime_as[n].time.secPastEpoch;
    output_p++;
    *output_p = (double)evrTime_as[n].time.nsec;
    output_p++;
    *output_p = (double)evrTime_as[n].status;
    output_p++;
  }
  epicsMutexUnlock(evrTimeRWMutex_ps);
  return epicsTimeOK;
}

/*=============================================================================

  Name: evrTimeInit

  Abs:  Creates the evrTimeRWMutex_ps read/write mutex 
        and initializes the evrTime_as array.
		The evr timestamp table is initialized to system time
		and invalid status. During processing, if a timestamp status goes
		invalid, the time is overwritten to the last good evr timestamp.

  Side: EVR Time Timestamp table
  pulse pipeline n  , status - 0=OK; -1=invalid
  0 = current pulse (Pn), status
  1 = next (upcoming) pulse (Pn-1), status
  2 = two pulses in the future (Pn-2), status
  3 = three pulses in the future (Pn-3), status

  Status is invalid when
  1) Bootup - System time is entered (as opposed to evr timestamp).
  2) the PULSEID of most recent index is is the same as the previous index.
  3) pattern waveform record invalid - timestamp is last good time 
==============================================================================*/ 
static int evrTimeInit(subRecord *psub)
{
  epicsTimeStamp sys_time;
  unsigned short i;

  /* create read/write mutex around evr timestamp table array */
  if (!evrTimeRWMutex_ps) {
	/* begin to init times with system time */
	if ( evrTimeGetSystem (&sys_time, evrTimeCurrent) ) {
	  return epicsTimeERROR;
	}
	else {
	  /* init structure to invalid status & system time*/
	  for (i=0;i<MAX_EVR_TIME;i++) {
		evrTime_as[i].time   = sys_time;
		evrTime_as[i].status = epicsTimeERROR;
	  }
	}
	evrTimeRWMutex_ps = epicsMutexCreate();
	if (!evrTimeRWMutex_ps) {
	  DEBUGPRINT(DP_ERROR, evrTimeFlag,
                     ("evrTimeInit: Unable to create TimeRWMutex!\n"));
	  return epicsTimeERROR;
	}
	DEBUGPRINT(DP_DEBUG, evrTimeFlag,
                   ("evrTimeInit: system time sec= %d; nsec = %d!\n",
                    sys_time.secPastEpoch, sys_time.nsec));	
  }
  /* For IOCs that support iocClock (RTEMS and vxWorks), register
     evrTimeGet with generalTime so it is used by epicsTimeGetEvent */
#ifdef __rtems__
  if (generalTimeTpRegister("evrTimeGet", 1000, 0, 0, 1,
                            (pepicsTimeGetEvent)evrTimeGet))
    return epicsTimeERROR;
  if (generalTimeTpRegister("evrTimeGetSystem", 2000, 0, 0, 2,
                            (pepicsTimeGetEvent)evrTimeGetSystem))
    return epicsTimeERROR;
#endif
  return epicsTimeOK;
}
/* For IOCs that don't support iocClock (linux), supply a dummy
   iocClockRegister to keep the linker happy. */
#ifdef linux
void iocClockRegister(pepicsTimeGetCurrent getCurrent,
                      pepicsTimeGetEvent   getEvent) 
{
}
#endif

/*=============================================================================

  Name: evrTimeProc

  Abs:  Processes every time the fiducial event code is received @ 360 Hz.
        Performs evr timestamp error checking, and then advances 
        the timestamp table in the evrTime_as array.

 
  Error Checking:
  Note, latest, n-3 timestamp, upon pattern waveform error, could have:
    pulse ID=0, EVR timestamp/status to last-good-time/invalid from evrPatternProc

  Pulse ID error (any PULSEID of 0 or non-consecutive PULSEIDs) - 
    Set appropriate counters. Set error flag.
  Set EVR timestamp status to invalid if the PULSEID of that index is 
    is the same as the previous index.
  Error advancing EVR timestamps - set error flag used later to disable 
    triggers (EVR) or event codes (EVG). 

Timestamp table evrTime_as after advancement:
pulse pipeline n  , status - 0=OK; 1=last good; 2=invalid
 0 = current pulse (P0), status
 1 = next (upcoming) pulse (Pn-1), status
 2 = two pulses in the future (Pn-2), status
 3 = three pulses in the future (Pn-3), status

		
  Args: Type	            Name        Access	   Description
        ------------------- -----------	---------- ----------------------------
        subRecord *         psub        read       point to subroutine record

  Rem:  Subroutine for IOC:LOCA:UNIT:FIDUCIAL

  Side: Upon entry, pattern is:
        n P0   don't care about n, this was acted upon for beam occuring last 
	       fiducial
        n-1 P1 this is the pattern for this fiducial w B1
        n-2 P2
        n-3 P3
	Algorithm: 
	During proper operation, every pulse in the pattern should be consecutive;
	differing by "1". Subtract oldest pulseid (N) from newest pulseid (N-3)
	to see if the difference is "3". Error until all four pulseids are consecutive.
	Upon error, set error flag that will disable events/triggers, whichever we decide.
	    
  Sub Inputs/ Outputs:
   Inputs:
   A - PULSEIDN-3
   B - PULSEIDN-2
   C - PULSEIDN-1
   D - PULSEID
   E - PULSEIDN-3 severity
   F - PULSEIDN-2 severity
   G - PULSEIDN-1 severity
   
   Input/Outputs:
   H   Spare
   I   Counter of same pulses in a row
   J   SAMEPULSECNT
   K   SKIPPULSECNT
   L   Spare
   VAL = Error Flag
   Ret:  0
   
==============================================================================*/
static int evrTimeProc (subRecord *psub)
{
  int errFlag = EVR_TIME_OK;
  int n;
  int diff;

  /* The 3 next pulses must be valid before all is considered OK to go */
  if ((psub->e == INVALID_ALARM) || (psub->f == INVALID_ALARM) ||
      (psub->g == INVALID_ALARM)) {
    errFlag = EVR_TIME_INVALID;
    
  /* Diff between first and second and second and third must be 1 */
  /* pulse ID may have rolled over */
  } else {
    diff = psub->a - psub->b;
    if ((diff != 1) && (diff != -PULSEID_MAX)) {
      errFlag = EVR_TIME_INVALID;
      if (diff==0) ++psub->j; /* same pulse coming into the pipeline */
      else         ++psub->k; /* skipped pulse (non-consecutive) in the pipeline */
    } else {
      diff = psub->b - psub->c;
      if ((diff != 1) && (diff != -PULSEID_MAX)) {
        errFlag = EVR_TIME_INVALID;
      }
    }
  }
  /* advance the evr timestamps in the pipeline */
  if (!epicsMutexLock(evrTimeRWMutex_ps)) {
    for (n=0;n<evrTimeNext3;n++) {
      evrTime_as[n] = evrTime_as[n+1];
    }
    /* determine if next is the same as last pulse */
    /* Allow 3 same pulses in a row before setting the next time to invalid */
    /* Same pulses means the EVG is not sending timestamps and this forces
       record timestamps to revert to system time */
    if (psub->d==psub->c) {
      if (psub->i > evrTimeNext3) {
        evrTime_as[evrTimeCurrent].status = epicsTimeERROR;
      } else {
        ++psub->i;
      }
    } else {
      psub->i = 0;
    }
    epicsMutexUnlock(evrTimeRWMutex_ps);
  /* If we cannot lock - bad problem somewhere. Set everything to bad status */
  } else {
    ++psub->i;
    errFlag = EVR_TIME_INVALID;
    for (n=0;n<evrTimeNext3;n++) {
      evrTime_as[evrTimeCurrent].status = epicsTimeERROR;
    }
  }
  psub->val = (double) errFlag;
  if (errFlag) return epicsTimeERROR;
  return epicsTimeOK;
}
epicsRegisterFunction(evrTimeInit);
epicsRegisterFunction(evrTimeProc);
epicsRegisterFunction(evrTimeDiagInit);
epicsRegisterFunction(evrTimeDiag);

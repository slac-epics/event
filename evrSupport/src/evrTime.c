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

#include "evrTime.h"       

#define  EVR_TIME_OK 0
#define  EVR_TIME_INVALID 1

/* EVR Timestamp table */
typedef struct {

  epicsTimeStamp      time;   /* epics timestamp:                        */
                              /* 1st 32 bits = # of seconds since 1990   */
                              /* 2nd 32 bits = # of nsecs since last sec */
                              /*           except lower 17 bits = pulsid */
  evrTimeStatus_te    status; /* 0=OK; 1=invalid                         */
}evrTime_ts;
evrTime_ts evrTime_as[MAX_EVR_TIME];

/* last good timestamp */
evrTime_ts evrTimeLastGood_s;

/* EVR Time Timestamp RWMutex */
epicsMutexId evrTimeRWMutex_ps;
/*=============================================================================

  Name: evrTimeGet

  Abs:  Get the evr epics timestamp, defined as:
        1st integer = number of seconds since 1990  
        2nd integer = number of nsecs since last sec, except lower 17 bits=pulsid
        3rd integer = status, where 0=OK; 1=invalid   
		
  Args: Type     Name           Access	   Description
        -------  -------	---------- ----------------------------
  epicsTimeStamp * epicsTime_ps write  ptr to epics timestamp to be returned
        evrTimeId_te  id        read   timestamp pipeline index;
	                                  1=time associated w this pulse
					  2=time associated w next pulse
                                          3=time assoc'ed w pulse 2 ahead
                                          4=time assoc'ed w pulse 3 ahead

  Rem:  Routine to get the epics timestamp from the evr timestamp table
        that is populated from incoming broadcast from EVG

  Side: 

  Return:   -1=Failed; 0 = Success
==============================================================================*/

int evrTimeGet (epicsTimeStamp  *epicsTime_ps, evrTimeId_te id)
{
  int index = id-1;
  int status;
  
  /* if the r/w mutex is valid, and we can lock with it, read requested time index */
  if ( evrTimeRWMutex_ps){
    epicsMutexMustLock( evrTimeRWMutex_ps);
    *epicsTime_ps = evrTime_as[index].time;
    if (evrTime_as[index].status == evrTimeInvalid) status = epicsTimeERROR;
    else                                            status = epicsTimeOK;
    epicsMutexUnlock(evrTimeRWMutex_ps);
  }
  /* invalid mutex id */
  else {
    DEBUGPRINT(DP_ERROR,evrTimeFlag,("evrTimeGet: Bad TimeRWMutex! \n"));
    status = epicsTimeERROR;
  }
  return status; 
}
/*=============================================================================

  Name: evrTimePut

  Abs:  Set the evr timestamp, defined as:
        1st int = number of seconds since 1990  
        2nd int = number of nsecs since last sec, except lower 17 bits=pulsid
        3rd int = status, where 0=OK; 1=invalid   
		
  Args: Type     Name           Access	   Description
        -------  -------	---------- ----------------------------
  epicsTimeStamp *  epicsTime_ps   read    ptr to epics timestamp
  evrTimeStatus  status            read    status

  Rem:  Routine to update the epics timestamp in the evr timestamp table
        that is populated from incoming broadcast from EVG

  Side: 

  Return:  -1=Failed; 0 = Success
==============================================================================*/

int evrTimePut (epicsTimeStamp  *epicsTime_ps, evrTimeStatus_te status)
{
  int index = evrTimeNext3-1;

  if (evrTimeRWMutex_ps){
    epicsMutexMustLock( evrTimeRWMutex_ps);
    evrTime_as[index].status = status;
    if (status != evrTimeOK) {
      evrTime_as[index].time = evrTimeLastGood_s.time;
    }
    else {
      evrTime_as[index].time = *epicsTime_ps;
      evrTimeLastGood_s.time = *epicsTime_ps;
    }
    epicsMutexUnlock(evrTimeRWMutex_ps);
  }
  /* invalid mutex id */
  else {
    DEBUGPRINT(DP_ERROR,evrTimeFlag,("evrTimePutTime: Bad TimeRWMutex! \n"));
    return epicsTimeERROR;
  }		
  return epicsTimeOK;
}
/*=============================================================================

  Name: evrTimePutStatus

  Abs:  Set the evr timestamp status, defined as:
        1st int = number of seconds since 1990  
        2nd int = number of nsecs since last sec, except lower 17 bits=pulsid
        3rd int = status, where 0=OK; 1=invalid   
		
  Args: Type     Name           Access	   Description
        -------  -------	---------- ----------------------------
   evrTime_te evrTimeIndex   read   timestamp pipeline index;
	                                  1=time associated w this pulse
					  2=time associated w next pulse
                                          3=time assoc'ed w pulse 2 ahead
                                          4=time assoc'ed w pulse 3 ahead
  evrTimeStatus  status     read  status

  Rem:  Routine to update the timestamp status in the evr timestamp table
        that is populated from incoming broadcast from EVG

  Side: 

  Return:  -1=Failed; 0 = Success
==============================================================================*/

static int evrTimePutStatus (evrTimeId_te id, evrTimeStatus_te status)
{
  int index = id-1;

  if ( evrTimeRWMutex_ps){
	epicsMutexMustLock( evrTimeRWMutex_ps);
	if (status > evrTime_as[index].status)
          evrTime_as[index].status = status;
	epicsMutexUnlock(evrTimeRWMutex_ps);
  }
  /* invalid mutex id */
  else {
	DEBUGPRINT(DP_ERROR,evrTimeFlag,("evrTimePutStatus: Bad TimeRWMutex! \n"));
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

  Name: evrTimeGetSystem

  Abs:  Returns system time and status of call
  
==============================================================================*/ 
int evrTimeGetSystem (epicsTimeStamp  *epicsTime_ps, evrTimeId_te id)
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

  Name: evrTimeDiagInit

  Abs:  General purpose initialization required since all subroutine records
   require a non-NULL init routine even if no initialization is required.
   Note that most subroutines in this file use this routine as an init
   routine.  If init logic is needed for a specific subroutine, create a
   new routine for it - don't modify this one.
  
==============================================================================*/ 
static int evrTimeDiagInit(subRecord *psub)
{

  return 0;
}
/*=============================================================================

  Name: evrTimeDiag

  Abs:  Expose time from the table to pvs
  
  Outputs:
  A  secPastEpoch for timestamp 1, n (diagTime_as[0])
  B  nsec for timestamp 1, n
  C  status "
  D  secPastEpoch for timestamp 2, n-1 (diagTime_as[1])
  E  nsec for timestamp 2, n-1 (diagTime_as[1])
  F  status "
  G  secPastEpoch for timestamp 3, n-2 (diagTime_as[2])
  H  nsec for timestamp 3, n-2 (diagTime_as[2])
  I  status "
  J  secPastEpoch for timestamp 4, n-3 (diagTime_as[3])
  K  nsec for timestamp 4, n-3 (diagTime_as[3])
  L  status "
==============================================================================*/ 
static long evrTimeDiag (subRecord *psub)
{
  unsigned short n;
  double  *output_p = &psub->a;

  /* read evr timestamps in the pipeline*/
  if ( evrTimeRWMutex_ps){
	epicsMutexMustLock( evrTimeRWMutex_ps);
	for (n=0;n<MAX_EVR_TIME;n++) {
	  *output_p = (double)evrTime_as[n].time.secPastEpoch;
          output_p++;
	  *output_p = (double)evrTime_as[n].time.nsec;
          output_p++;
	  *output_p = (double)evrTime_as[n].status;
          output_p++;
	}	
	epicsMutexUnlock(evrTimeRWMutex_ps);
  }
  /* invalid mutex id */
  else {
	DEBUGPRINT(DP_ERROR, evrTimeFlag, 
			   ("evrTimeDiag: Invalid evrTimeRWMutex_ps!\n"));	
	return -1;
  }
  return 0;
}

/*=============================================================================

  Name: evrTimeInit

  Abs:  Creates the evrTimeRWMutex_ps read/write mutex 
        and initializes the evrTime_as array.
		The evr timestamp table is initialized to system time
		and invalid status. During processing, if a timestamp status goes
		invalid, the time is overwritten to the last good evr timestamp.
		All records that want to use one of these timestamps must set the TSE 
		field to the proper event so epicsTimeGetEvent.  For most (probably all)
		records, TSE is 1.  

  Side: EVR Time Timestamp table
  TSE  pulse pipeline n  , status - 0=OK; 1=invalid
  1 = current pulse (Pn), status
  2 = next (upcoming) pulse (Pn-1), status
  3 = two pulses in the future (Pn-2), status
  4 = three pulses in the future (Pn-3), status

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
	  return -1;
	}
	else {
	  /* init structure to invalid status & system time*/
	  for (i=0;i<MAX_EVR_TIME;i++) {
		evrTime_as[i].time = sys_time;
		evrTime_as[i].status = evrTimeInvalid;      /* invalid=2*/
	  }
	}
	evrTimeLastGood_s.time = sys_time;
	evrTimeRWMutex_ps = epicsMutexCreate();
	if (!evrTimeRWMutex_ps) {
	  DEBUGPRINT(DP_ERROR, evrTimeFlag, ("evrTimeInit: Unable to create TimeRWMutex!\n"));
	  return -1;
	}
	DEBUGPRINT(DP_DEBUG, evrTimeFlag, ("evrTimeInit: system time sec= %d; nsec = %d!\n",
									   sys_time.secPastEpoch, sys_time.nsec));	
  }
  /* For IOCs that support iocClock (RTEMS and vxWorks), register
     evrTimeGet with generalTime so it is used by epicsTimeGetEvent
     for records with the TSE field set to 1 to 4 */
#ifdef __rtems__
  if (generalTimeTpRegister("evrTimeGet", 1000, 0, 0, 1,
                            (pepicsTimeGetEvent)evrTimeGet      )) return -1;
  if (generalTimeTpRegister("evrTimeGetSystem", 2000, 0, 0, 2,
                            (pepicsTimeGetEvent)evrTimeGetSystem)) return -1;
#endif
  return 0;
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
TSE  pulse pipeline n  , status - 0=OK; 1=invalid
 1 = current pulse (P0), status
 2 = next (upcoming) pulse (Pn-1), status
 3 = two pulses in the future (Pn-2), status
 4 = three pulses in the future (Pn-3), status

		
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
   
   I - Reset Counters
   
   Input/Outputs:
   J   SAMEPULSECNT
   K   SKIPPULSECNT
   VAL = Error Flag
   Ret:  0
   
==============================================================================*/
static int evrTimeProc (subRecord *psub)
{
  int errFlag = EVR_TIME_OK;
  int n;

  if (psub->i) {
   	psub->j = 0;
	psub->k = 0;
	psub->i = 0;
	DEBUGPRINT(DP_DEBUG,evrTimeFlag,("evrTimeProc Reset counters \n"));
  }
  /* error check the pulseids of n-1,n-2,n-3 */
  /*   if n-3 and n-2 are not consecutive, set error flag */

  if ( (psub->a - psub->d)!= 3) {
	errFlag = EVR_TIME_INVALID;
	++psub->k;  /* skipped pulse (non-consecutive) in the pipeline somewhere */
	/* determine if newest is the same as last pulse? */
	if (psub->a==psub->b) {
	  ++psub->j;	  
	  /* Set EVR timestamp status to invalid if the PULSEID of that index is 
		 is the same as the previous index. */
	  evrTimePutStatus (evrTimeNext3, evrTimeLastGood);
	}
  }

  /* advance the evr timestamps in the pipeline*/
  if ( evrTimeRWMutex_ps){
	epicsMutexMustLock( evrTimeRWMutex_ps);
	for (n=0;n<MAX_EVR_TIME-1;n++) {
	  evrTime_as[n] = evrTime_as[n+1];
	}	
	epicsMutexUnlock(evrTimeRWMutex_ps);
  }
  /* invalid mutex id */
  else {
	errFlag = EVR_TIME_INVALID;
  }
  psub->val = (double) errFlag;
  if (errFlag){
	return -1;
  }
  return 0;
}

epicsRegisterFunction(evrTimeInit);
epicsRegisterFunction(evrTimeProc);
epicsRegisterFunction(evrTimeDiagInit);
epicsRegisterFunction(evrTimeDiag);

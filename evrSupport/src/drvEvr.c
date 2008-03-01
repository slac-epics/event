/*=============================================================================
 
  Name: drvEvr.c
        evrInitialise  - EVR data queue initialisation
        evrReport      - driver report
        evrSend        - Send to EVR data queue

  Abs:  Driver data support for EVR Receiver module or EVR simulator.   

  Auth: 22-dec-2006, S. Allison:
  Rev:  

-----------------------------------------------------------------------------*/

#include "copyright_SLAC.h"	/* SLAC copyright comments */
 
/*-----------------------------------------------------------------------------
 
  Mod:  (newest to oldest)  
 
=============================================================================*/

#include <drvSup.h> 		/* for DRVSUPFN           */
#include <errlog.h>		/* for errlogPrintf       */
#include <epicsExport.h> 	/* for epicsExportAddress */
#include <epicsEvent.h> 	/* for epicsEvent*        */
#include <epicsThread.h> 	/* for epicsThreadCreate  */
#include <evrMessage.h>		/* for evrMessageCreate   */
#include <drvMrfEr.h>		/* for ErRegisterDevDBuffHandler */
#include <devMrfEr.h>		/* for ErRegisterEventHandler    */

#define EVR_TIMEOUT     (0.06)  /* Timeout in sec waiting for 360hz input. */

static int evrReport();
static int evrInitialise();
struct drvet drvEvr = {
  2,
  (DRVSUPFUN) evrReport, 	/* subroutine defined in this file */
  (DRVSUPFUN) evrInitialise 	/* subroutine defined in this file */
};
epicsExportAddress(drvet, drvEvr);

static ErCardStruct    *pCard             = NULL;  /* EVR card pointer    */
static epicsEventId     evrTaskEventSem   = NULL;  /* evr task semaphore  */
static volatile int     patternAvailable  = 0; /* pattern  available flag */

/*=============================================================================

  Name: evrReport

  Abs: Driver support registered function for reporting system info

=============================================================================*/
static int evrReport( int interest )
{
  if (interest > 0) {
    if (pCard) 
      printf("Pattern data from %s card %d\n",
             pCard->FormFactor?"PMC":"VME", pCard->Cardno);
    evrMessageReport(EVR_MESSAGE_FIDUCIAL, EVR_MESSAGE_FIDUCIAL_NAME,
                     interest);
    evrMessageReport(EVR_MESSAGE_PATTERN,  EVR_MESSAGE_PATTERN_NAME ,
                     interest);
  }
  return interest;
}

/*=============================================================================
 
  Name: evrSend

  Abs: Called by either ErIrqHandler to put each buffer of EVR data
       into the proper message area or by a subroutine record for
       the EVR simulator.

  Rem: Keep this routine to a minimum, so that CPU not blocked 
       too long processing each interrupt.
       
=============================================================================*/
void evrSend(void *pCard, epicsInt16 messageSize, void *message)
{
  unsigned int messageType = ((evrMessageHeader_ts *)message)->type;

  if (pCard && ((ErCardStruct *)pCard)->DBuffError) {
    evrMessageCheckSumError(EVR_MESSAGE_PATTERN);
  } else {
    evrMessageStart(messageType);
    evrMessageWrite(messageType, (evrMessage_tu *)message);
    patternAvailable = 1;
  }
}

/*=============================================================================
 
  Name: evrEvent

  Abs: Called by the ErIrqHandler to handle event code 1 (fiducial) processing.

  Rem: Keep this routine to a minimum, so that CPU not blocked 
       too long processing each interrupt.
       
=============================================================================*/
void evrEvent(void *pCard, epicsInt16 eventNum, epicsUInt32 timeNum)
{
  /* Skip this fiducial if the newest pattern hasn't yet been processed. */
  if (eventNum == EVENT_FIDUCIAL) {
    evrMessageStart(EVR_MESSAGE_FIDUCIAL);
    epicsEventSignal(evrTaskEventSem);
  }
}

/*=============================================================================
                                                                                
  Name: evrTask
                                                                                
  Abs:  This task performs record processing and monitors the EVR module.
  
  Rem:  It's started by evrInitialise after the EVR module is configured. 
    
=============================================================================*/
static int evrTask()
{  
  epicsEventWaitStatus status;
  for (;;)
  {
    status = epicsEventWaitWithTimeout(evrTaskEventSem, EVR_TIMEOUT);
    if (status == epicsEventWaitOK) {
      if (patternAvailable) {
        patternAvailable = 0;
        evrMessageProcess(EVR_MESSAGE_PATTERN);
        evrMessageEnd(EVR_MESSAGE_PATTERN);
      }
      evrMessageProcess(EVR_MESSAGE_FIDUCIAL);
      evrMessageEnd(EVR_MESSAGE_FIDUCIAL);
    /* If timeout or other error, process the data which will result in bad
       status since there is nothing to do.  Then advance the pipeline so
       that the bad status makes it from N-3 to N-2 then to N-2 and
       then to N. */
    } else {
      evrMessageProcess(EVR_MESSAGE_PATTERN);   /* N-3 */
      evrMessageProcess(EVR_MESSAGE_FIDUCIAL);  /* N-2 */
      evrMessageProcess(EVR_MESSAGE_FIDUCIAL);  /* N-1 */
      evrMessageProcess(EVR_MESSAGE_FIDUCIAL);  /* N   */
      if (status != epicsEventWaitTimeout) {
        errlogPrintf("evrTask: Exit due to bad status from epicsEventWaitWithTimeout\n");
        return -1;
      }
    }
  }
  return 0;
}

static int evrInitialise()
{

  /* Create space for the pattern + diagnostics */
  if (evrMessageCreate(EVR_MESSAGE_PATTERN_NAME,
                       sizeof(evrMessagePattern_ts)) !=
      EVR_MESSAGE_PATTERN) return -1;
  
  /* Create space for the fiducial + diagnostics */
  if (evrMessageCreate(EVR_MESSAGE_FIDUCIAL_NAME, 0) !=
      EVR_MESSAGE_FIDUCIAL) return -1;
  
  /* Create the semaphore used by the ISR to wake up the evr task */
  evrTaskEventSem = epicsEventMustCreate(epicsEventEmpty);
  if (!evrTaskEventSem) {
    errlogPrintf("evrInitialise: unable to create the EVR task semaphore\n");
    return -1;
  }
  
  /* Create the task to process records */
  if (!epicsThreadCreate("evrTask", epicsThreadPriorityHigh+1,
                         epicsThreadGetStackSize(epicsThreadStackMedium),
                         (EPICSTHREADFUNC)evrTask, 0)) {
    errlogPrintf("evrInitialise: unable to create the EVR task\n");
    return -1;
  }
  
#ifdef __rtems__
  /* Get first EVR in the list */
  pCard = ErGetCardStruct(0);
  if (!pCard) {
    errlogPrintf("evrInitialise: cannot find an EVR module\n");
  /* Register the ISR functions in this file with the EVR */
  } else {
    ErRegisterDevDBuffHandler(pCard, (DEV_DBUFF_FUNC)evrSend);
    ErEnableDBuff            (pCard, 1);
    ErDBuffIrq               (pCard, 1);
    ErRegisterEventHandler   (0,    (USER_EVENT_FUNC)evrEvent);
  }
#endif
  
  return 0;
}

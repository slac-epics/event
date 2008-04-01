/*=============================================================================
 
  Name: drvEvr.c
        evrInitialise  - EVR Data and Event Initialisation
        evrReport      - Driver Report
        evrSend        - Send EVR data to Message Queue
        evrEvent       - Process Event Codes
        evrTask        - High Priority task to process 360Hz Fiducial and Data

  Abs:  Driver data and event support for EVR Receiver module or EVR simulator.   

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
#include <evrTime.h>		/* for evrTimeCount       */
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
static int readyForFiducial = 1;        /* evrTask ready for new fiducial */

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

  /* Look for error from the driver or the wrong message size */
  if ((pCard && ((ErCardStruct *)pCard)->DBuffError) ||
      (messageSize != sizeof(evrMessagePattern_ts))) {
    evrMessageCheckSumError(EVR_MESSAGE_PATTERN);
  } else {
    evrMessageStart(messageType);
    if (evrMessageWrite(messageType, (evrMessage_tu *)message))
      evrMessageCheckSumError(EVR_MESSAGE_PATTERN);
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
  if (eventNum == EVENT_FIDUCIAL) {
    if (readyForFiducial) {
      readyForFiducial = 0;
      evrMessageStart(EVR_MESSAGE_FIDUCIAL);
      epicsEventSignal(evrTaskEventSem);
    } else {
      evrMessageNoDataError(EVR_MESSAGE_FIDUCIAL);
    }
  }
  evrTimeCount((unsigned long)eventNum);
}

/*=============================================================================
                                                         
  Name: evrTask

  Abs:  This task performs record processing for the fiducial and data.
  
  Rem:  It's started by evrInitialise after the EVR module is configured. 
    
=============================================================================*/
static int evrTask()
{  
  epicsEventWaitStatus status;

  for (;;)
  {
    readyForFiducial = 1;
    status = epicsEventWaitWithTimeout(evrTaskEventSem, EVR_TIMEOUT);
    if (status == epicsEventWaitOK) {
      evrMessageProcess(EVR_MESSAGE_PATTERN);
      evrMessageEnd(EVR_MESSAGE_PATTERN);
      evrMessageProcess(EVR_MESSAGE_FIDUCIAL);
      evrMessageEnd(EVR_MESSAGE_FIDUCIAL);
    /* If timeout or other error, process the data which will result in bad
       status since there is nothing to do.  Then advance the pipeline so
       that the bad status makes it from N-3 to N-2 then to N-2 and
       then to N. */
    } else {
      readyForFiducial = 0;
      evrMessageTimeout(EVR_MESSAGE_PATTERN);
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

/*=============================================================================

  Name: evrQueue.c
           evrQueueCreate    - Create a Message Queue and Init Scan I/O
           evrQueueRegister  - Register Receiving Client of Message Queue
           evrQueueSend      - Send a Message to a Message Queue
           evrQueueReceive   - Receive a Message from a Message Queue
           evrQueueReport    - Report Information about a Message Queue
           evrQueueCounts    - Get Message Queue Diagnostic Counts
           evrQueueCountReset- Reset Message Queue Diagnostic Counts
           evrQueueIndex     - Get Index of a Message Queue

  Abs:  EVR and PNET message queue creation and registration.

  Auth: 21-Dec-2006, S. Allison
  Rev:
-----------------------------------------------------------------------------*/

#include "copyright_SLAC.h"     /* SLAC copyright comments */

/*-----------------------------------------------------------------------------

  Mod:  (newest to oldest)

=============================================================================*/

#include <stddef.h>             /* size_t                      */
#include <string.h>             /* strcmp                      */
#include <stdio.h>              /* printf                      */

#include "dbScan.h"             /* IOSCANPVT and scanIo*       */
#include "epicsMessageQueue.h"  /* for epicsMessageQueue*      */
#include "epicsTime.h"          /* epicsTimeStamp              */
#include "errlog.h"             /* errlogPrintf                */
#include "evrQueue.h"           /* prototypes in this file     */

/* Maintain 3 queues - PNET, PATTERN, and DATA */
static evrQueue_ts evrQueue_as[EVR_QUEUE_MAX] = {{0}, {0}, {0}};

/*=============================================================================

  Name: evrQueueIndex

  Abs:  Get the Queue Index

  Args: Type     Name           Access     Description
        -------  -------        ---------- ----------------------------
        char *   queueName_a    Read       Queue Name ("PNET", "PATTERN", "DATA")

  Rem:  The queue name is checked for validity and the index of the queue
        is returned. 

  Side: None

  Return: index (-1 = Failed)
==============================================================================*/

static int evrQueueIndex(char *queueName_a)
{
  if (!strcmp(queueName_a, EVR_QUEUE_PNET_NAME   )) return EVR_QUEUE_PNET;
  if (!strcmp(queueName_a, EVR_QUEUE_PATTERN_NAME)) return EVR_QUEUE_PATTERN;
  if (!strcmp(queueName_a, EVR_QUEUE_DATA_NAME   )) return EVR_QUEUE_DATA;
  errlogPrintf("evrQueue - Invalid queue name %s\n", queueName_a);
  return -1;
}

/*=============================================================================

  Name: evrQueueCreate

  Abs:  Create a Message Queue and Init Scan I/O

  Args: Type     Name           Access     Description
        -------  -------        ---------- ----------------------------
        char *   queueName_a    Read       Queue Name ("PNET", "PATTERN", "DATA")
        size_t   messageSize    Read       Size of one message in the queue

  Rem:  The queue index is found and then a check is done that
        the queue hasn't already been created.  An epics message queue is then
        created and scan I/O for this queue is initialized.  The queue ID is
        returned if all is OK.

  Side: EPICS message queue is created and scan I/O is initialized.

  Return: evrQueueId (0 = Failed)
==============================================================================*/

evrQueueId evrQueueCreate(char *queueName_a, size_t messageSize)
{
  int queueIdx = evrQueueIndex(queueName_a);
  
  if (queueIdx < 0) return 0;

  if (evrQueue_as[queueIdx].ioscanpvt) {
    errlogPrintf("evrQueueCreate - Queue %s already created\n", queueName_a);
    return(0);
  }
  /* Initialize scan I/O */
  scanIoInit(&evrQueue_as[queueIdx].ioscanpvt);
  if (!evrQueue_as[queueIdx].ioscanpvt) {
    errlogPrintf("evrQueueCreate - Queue %s scanIoInit error\n", queueName_a);
    return(0);
  }
  /* set up the epics message queue */
  evrQueue_as[queueIdx].messageQueueId = epicsMessageQueueCreate(1, messageSize);
  if (!evrQueue_as[queueIdx].messageQueueId) {
    errlogPrintf("evrQueueCreate - Queue %s creation error\n", queueName_a);
    return(0);
  }
  evrQueue_as[queueIdx].messageSize          = messageSize;
  evrQueue_as[queueIdx].receiverRegistered   = 0;
  evrQueue_as[queueIdx].trySendCount         = 0;
  evrQueue_as[queueIdx].trySendCountRollover = 0;
  evrQueue_as[queueIdx].noSendCount          = 0;
  /* Save time at creation for reporting purposes */
  epicsTimeGetCurrent(&evrQueue_as[queueIdx].createTime_s);
  
  return (&evrQueue_as[queueIdx]);
}

/*=============================================================================

  Name: evrQueueRegister

  Abs:  Register a Message Queue Receiver

  Args: Type     Name           Access     Description
        -------  -------        ---------- ----------------------------
        char *   queueName_a    Read       Queue Name ("PNET", "PATTERN", "DATA")
        size_t   messageSize    Read       Size of one message in the queue

  Rem:  The queue index is found, the queue is checked that
        it's been created, a check is done that the queue hasn't already
        been taken, and the message size is checked. The queue ID is
        returned if all is OK. 

  Side: None

  Return: evrQueueId (0 = Failed)
==============================================================================*/

evrQueueId evrQueueRegister(char *queueName_a, size_t messageSize)
{
  int queueIdx = evrQueueIndex(queueName_a);
  
  if (queueIdx < 0) return 0;
  if ((!evrQueue_as[queueIdx].ioscanpvt) ||
      (!evrQueue_as[queueIdx].messageQueueId)) {
    errlogPrintf("evrQueueRegister - Queue %s not yet created\n", queueName_a);
    return(0);
  }
  if (evrQueue_as[queueIdx].messageSize != messageSize) {
    errlogPrintf("evrQueueRegister - Queue %s has wrong size %d\n",
                 queueName_a, evrQueue_as[queueIdx].messageSize);
    return(0);
  }
  if (evrQueue_as[queueIdx].receiverRegistered) {
    errlogPrintf("evrQueueRegister - Queue %s already has a receiver\n",
                 queueName_a);
    return(0);
  }
  evrQueue_as[queueIdx].receiverRegistered = 1;    
  
  return (&evrQueue_as[queueIdx]);
}

/*=============================================================================

  Name: evrQueueSend

  Abs:  Send a Message to a Message Queue

  Args: Type     Name           Access     Description
        -------  -------        ---------- ----------------------------
      evrQueueId evrQueue_ps    Read/Write EVR Queue Structure
      void *     message_p      Read       Message to Send 

  Rem:  Try sending the message if somebody is listening.  Wake up listener
        via I/O scan mechanism.

        THIS ROUTINE IS CALLED AT INTERRUPT LEVEL!!!!!!!!!!!!!!!!!!!

  Side: Write to an EPICS message queue.  Add callback request for I/O scan
        waveform record.

  Return: 0 = OK, -1 = Failed
==============================================================================*/

int evrQueueSend(evrQueueId evrQueue_ps, void * message_p)
{
  /* Talk only when somebody is listening */  
  if (evrQueue_ps && evrQueue_ps->receiverRegistered) {
    if (evrQueue_ps->trySendCount < EVR_MAX_INT) {
      evrQueue_ps->trySendCount++;
    } else {
      evrQueue_ps->trySendCountRollover++;
      evrQueue_ps->trySendCount = 0;
    }
    /* If unable to send, return bad status without waking up listener. */
    if (epicsMessageQueueTrySend(evrQueue_ps->messageQueueId, message_p,
                                 evrQueue_ps->messageSize)) {
      evrQueue_ps->noSendCount++;
    }
    /* Tell listener it's time to read */
    scanIoRequest(evrQueue_ps->ioscanpvt);
  } else {
    return -1;
  }
  return 0;
}

/*=============================================================================

  Name: evrQueueReceive

  Abs:  Receive a Message from a Message Queue

  Args: Type     Name           Access     Description
        -------  -------        ---------- ----------------------------
      evrQueueId evrQueue_ps    Read        EVR Queue Structure
      void *     message_p      Write       Received Message 

  Rem:  Try receiving a message.  

  Side: Receive from an EPICS message queue.

  Return: 0 = OK, -1 = Failed
==============================================================================*/

int evrQueueReceive(evrQueueId evrQueue_ps, void * message_p)
{ 
  if (evrQueue_ps) {
    /* Make sure we receive the exact amount expected */
    if (epicsMessageQueueTryReceive(evrQueue_ps->messageQueueId, message_p,
                                    evrQueue_ps->messageSize) !=
        evrQueue_ps->messageSize) return -1;
  } else {
    return -1;
  }
  return 0;
}

/*=============================================================================

  Name: evrQueueReport

  Abs:  Output Report on a Message Queue

  Args: Type     Name           Access     Description
        -------  -------        ---------- ----------------------------
      evrQueueId evrQueue_ps    Read        EVR Queue Structure

  Rem:  Printout information about this queue.  

  Side: Output to standard output.

  Return: 0 = OK, -1 = Failed
==============================================================================*/

int evrQueueReport(evrQueueId evrQueue_ps, char *queueName_a)
{ 
  if (evrQueue_ps) {
    char timestamp_c[MAX_STRING_SIZE];
    timestamp_c[0]=0;
    epicsTimeToStrftime(timestamp_c, MAX_STRING_SIZE,
                        "%a %b %d %Y %H:%M:%S.%f",
                        &evrQueue_ps->createTime_s);
    printf("%s message queue of size %d created at %s\n",
           queueName_a, evrQueue_ps->messageSize, timestamp_c);
    printf("Number of attempted sends to queue/rollover: %ld/%ld\n",
           evrQueue_ps->trySendCount, evrQueue_ps->trySendCountRollover);
    printf("Number of failed sends to queue: %ld\n",
           evrQueue_ps->noSendCount);
  } else {
    return -1;
  }
  return 0;
}

/*=============================================================================

  Name: evrQueueCounts

  Abs:  Return message queue diagnostic count values
        (for use by EPICS subroutine records)

  Args: Type     Name           Access     Description
        -------  -------        ---------- ----------------------------
  unsigned int   queueIdx         Read     Queue Index
  double *       trySendCount_p   Write    # times ISR tried to send a message
  double * trySendCountRollover_p Write    # times above rolled over
  double *       noSendCount_p    Write    # times ISR failed to send a message

  Rem:  The diagnostics count values are filled in if the queue index is valid.
  
  Side: None

  Return: 0 = OK, -1 = Failed
==============================================================================*/

int evrQueueCounts(unsigned int queueIdx,
                   double      *trySendCount_p,
                   double      *trySendCountRollover_p,
                   double      *noSendCount_p)
{
  if (queueIdx >= EVR_QUEUE_MAX ) return -1;
  *trySendCount_p         = (double)evrQueue_as[queueIdx].trySendCount;
  *trySendCountRollover_p = (double)evrQueue_as[queueIdx].trySendCountRollover;
  *noSendCount_p          = (double)evrQueue_as[queueIdx].noSendCount;
  return 0;
}
/*=============================================================================

  Name: evrQueueCountReset

  Abs:  Reset message queue diagnostic count values
        (for use by EPICS subroutine records)

  Args: Type     Name           Access     Description
        -------  -------        ---------- ----------------------------
  unsigned int   queueIdx         Read     Queue Index

  Rem:  The diagnostics count values are reset if the queue index is valid.
  
  Side: None

  Return: 0 = OK, -1 = Failed
==============================================================================*/

int evrQueueCountReset (unsigned int queueIdx)
{
  if (queueIdx >= EVR_QUEUE_MAX ) return -1;
  evrQueue_as[queueIdx].trySendCount = 0;
  evrQueue_as[queueIdx].trySendCountRollover = 0;
  evrQueue_as[queueIdx].noSendCount = 0;
  return 0;
}

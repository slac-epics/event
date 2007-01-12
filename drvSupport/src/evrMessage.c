/*=============================================================================

  Name: evrMessage.c
           evrMessageCreate    - Initialize Message Space and Init Scan I/O
           evrMessageRegister  - Register Reader of the Message
           evrMessageWrite     - Write a Message
           evrMessageRead      - Read  a Message
           evrMessageReport    - Report Information about the Message
           evrMessageCounts    - Get   Message Diagnostic Counts
           evrMessageCountReset- Reset Message Diagnostic Counts
           evrMessageIoscanpvt - Get   Message IO Scan Pointer
           evrMessageIndex     - Get Index of the Message Array

  Abs:  EVR and PNET message space creation and registration.

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
#include "epicsMutex.h"         /* epicsMutexId and protos     */
#include "epicsTime.h"          /* epicsTimeStamp              */
#include "errlog.h"             /* errlogPrintf                */
#include "evrMessage.h"         /* prototypes in this file     */

typedef struct 
{
  epicsMutexId        messageRWMutex_ps;
  evrMessage_tu       message_u;
  IOSCANPVT           ioscanpvt_p;
  long int            receiverRegistered;
  long int            messageNotRead;
  long int            updateCount;
  long int            updateCountRollover;
  long int            overwriteCount;
  long int            lockErrorCount;
  epicsTimeStamp      resetTime_s;
  
} evrMessage_ts;

/* Maintain 3 messages - PNET, PATTERN, and DATA */
static evrMessage_ts evrMessage_as[EVR_MESSAGE_MAX] = {{0}, {0}, {0}};

/*=============================================================================

  Name: evrMessageIndex

  Abs:  Get the Index into the Message Array

  Args: Type     Name           Access     Description
        -------  -------        ---------- ----------------------------
        char *   messageName_a  Read       Message Name ("PNET", "PATTERN", "DATA")
        size_t   messageSizeIn  Read       Size of a message

  Rem:  The message name is checked for validity and the index into the message
        array is returned.  The message size is also checked.

  Side: None

  Return: index (-1 = Failed)
==============================================================================*/

static int evrMessageIndex(char *messageName_a, int messageSizeIn)
{
  int messageSize;
  int messageIndex;
  
  if (!strcmp(messageName_a, EVR_MESSAGE_PNET_NAME   )) {
    messageSize  = sizeof(epicsUInt32) * EVR_PNET_MODIFIER_MAX;
    messageIndex = EVR_MESSAGE_PNET;
  } else if (!strcmp(messageName_a, EVR_MESSAGE_PATTERN_NAME)) {
    messageSize  = sizeof(evrMessagePattern_ts);
    messageIndex = EVR_MESSAGE_PATTERN;
  } else if (!strcmp(messageName_a, EVR_MESSAGE_DATA_NAME   )) {
    messageSize  = sizeof(epicsUInt32) * EVR_DATA_MAX;
    messageIndex = EVR_MESSAGE_DATA;
  } else {
    messageSize  = 0;
    messageIndex = -1;
  }
  if (messageIndex < 0) {
    errlogPrintf("evrMessageIndex - Invalid message name %s\n", messageName_a);
  } else if (messageSizeIn != messageSize) {
    errlogPrintf("evrMessageIndex - Invalid message size %d\n", messageSizeIn);
    messageIndex = -1;
  } 
  return messageIndex;
}

/*=============================================================================

  Name: evrMessageCreate

  Abs:  Initialize Message Space and Init Scan I/O

  Args: Type     Name           Access     Description
        -------  -------        ---------- ----------------------------
        char *   messageName_a  Read       Message Name ("PNET", "PATTERN", "DATA")
        size_t   messageSize    Read       Size of a message

  Rem:  The message index is found and then a check is done that
        the message space hasn't already been initialized.  A mutex is then
        created and scan I/O for this message is initialized.  The message
        index is returned if all is OK.

  Side: EPICS mutex is created and scan I/O is initialized.

  Return: -1 = Failed, 0,1,2 = message index
==============================================================================*/

int evrMessageCreate(char *messageName_a, size_t messageSize)
{
  int messageIdx = evrMessageIndex(messageName_a, messageSize);

  if (messageIdx < 0) return -1;

  if (evrMessage_as[messageIdx].messageRWMutex_ps) {
    errlogPrintf("evrMessageCreate - Message %s already intialized\n",
                 messageName_a);
    return -1;
  }
  /* set up the epics message mutex */
  evrMessage_as[messageIdx].messageRWMutex_ps = epicsMutexCreate();
  if (!evrMessage_as[messageIdx].messageRWMutex_ps) {
    errlogPrintf("evrMessageCreate - Message %s mutex creation error\n",
                 messageName_a);
    return -1;
  }
  /* Initialize scan I/O */
  scanIoInit(&evrMessage_as[messageIdx].ioscanpvt_p);
  if (!evrMessage_as[messageIdx].ioscanpvt_p) {
    errlogPrintf("evrMessageCreate - Message %s scanIoInit error\n",
                 messageName_a);
    return -1;
  }
  evrMessage_as[messageIdx].receiverRegistered = 0;
  evrMessage_as[messageIdx].messageNotRead     = 0;
  evrMessageCountReset(messageIdx);
  
  return messageIdx;
}

/*=============================================================================

  Name: evrMessageRegister

  Abs:  Register a Message Reader

  Args: Type     Name           Access     Description
        -------  -------        ---------- ----------------------------
        char *   messageName_a  Read       Message Name ("PNET", "PATTERN", "DATA")
        size_t   messageSize    Read       Size of a message

  Rem:  The message index is found, the message is checked that
        it's been created, and a check is done that the message hasn't already
        been taken. The message index is returned if all is OK. 

  Side: None

  Return: -1 = Failed, 0,1,2 = message index
==============================================================================*/

int evrMessageRegister(char *messageName_a, size_t messageSize)
{
  int messageIdx = evrMessageIndex(messageName_a, messageSize);
  
  if (messageIdx < 0) return -1;
  
  if ((!evrMessage_as[messageIdx].ioscanpvt_p) ||
      (!evrMessage_as[messageIdx].messageRWMutex_ps)) {
    errlogPrintf("evrMessageRegister - Message %s not yet created\n",
                 messageName_a);
    return -1;
  }
  if (evrMessage_as[messageIdx].receiverRegistered) {
    errlogPrintf("evrMessageRegister - Message %s already has a reader\n",
                 messageName_a);
    return -1;
  }
  evrMessage_as[messageIdx].receiverRegistered = 1;    
  
  return messageIdx;
}

/*=============================================================================

  Name: evrMessageWrite

  Abs:  Write a Message

  Args: Type     Name           Access     Description
        -------  -------        ---------- ----------------------------
  unsigned int    messageIdx     Read       Index into Message Array
  evrMessage_tu * message_pu     Read       Message to Update

  Rem:  Write a message.  Wake up listener via I/O scan mechanism.

        THIS ROUTINE IS CALLED AT INTERRUPT LEVEL!!!!!!!!!!!!!!!!!!!
        It can also be called at task level by a simulator.

  Side: Add callback request for I/O scan waveform record.
        Mutex lock and unlock.

  Return: 0 = OK, -1 = Failed
==============================================================================*/

int evrMessageWrite(unsigned int messageIdx, evrMessage_tu * message_pu)
{
  int locked = 1;
  
  if (messageIdx >= EVR_MESSAGE_MAX) return -1;

  /* Attempt to lock the message */
  if ((!evrMessage_as[messageIdx].messageRWMutex_ps) ||
      (epicsMutexTryLock(evrMessage_as[messageIdx].messageRWMutex_ps) !=
       epicsMutexLockOK)) {
    evrMessage_as[messageIdx].lockErrorCount++;
    locked = 0;
  }
  /* Update diagnostics counters */
  if (evrMessage_as[messageIdx].updateCount < EVR_MAX_INT) {
    evrMessage_as[messageIdx].updateCount++;
  } else {
    evrMessage_as[messageIdx].updateCountRollover++;
    evrMessage_as[messageIdx].updateCount = 0;
  }
  if (evrMessage_as[messageIdx].messageNotRead) {
    evrMessage_as[messageIdx].overwriteCount++;
  }
  /* Update message in holding array */
  evrMessage_as[messageIdx].message_u      = *message_pu;
  evrMessage_as[messageIdx].messageNotRead = 1;
  if (locked) epicsMutexUnlock(evrMessage_as[messageIdx].messageRWMutex_ps);
  /* Tell listener it's time to read */
  scanIoRequest(evrMessage_as[messageIdx].ioscanpvt_p);
  
  return 0;
}

/*=============================================================================

  Name: evrMessageRead

  Abs:  Read a Message

  Args: Type     Name           Access     Description
        -------  -------        ---------- ----------------------------
  unsigned int    messageIdx     Read       Index into Message Array
  evrMessage_tu * message_pu     Write      Message

  Rem:  Read a message.  

  Side: Mutex lock and unlock.

  Return: 0 = OK, -1 = Failed
==============================================================================*/

int evrMessageRead(unsigned int  messageIdx, evrMessage_tu *message_pu)
{
  int status = 0;
  int retry;
  
  if (messageIdx >= EVR_MESSAGE_MAX) return -1;

  /* Attempt to lock the message */
  if ((!evrMessage_as[messageIdx].messageRWMutex_ps) ||
      (epicsMutexLock(evrMessage_as[messageIdx].messageRWMutex_ps) !=
       epicsMutexLockOK)) return -1;

  /* Read the message only if its still available.  Retry once in case
     the ISR writes it again while we are reading */
  for (retry = 0; (retry < 2) && evrMessage_as[messageIdx].messageNotRead;
       retry++) {
    evrMessage_as[messageIdx].messageNotRead = 0;
    switch (messageIdx) {
      case EVR_MESSAGE_PNET:
        message_pu->pnet_s    = evrMessage_as[messageIdx].message_u.pnet_s;
        break;
      case EVR_MESSAGE_PATTERN:
        message_pu->pattern_s = evrMessage_as[messageIdx].message_u.pattern_s;
        break;
      case EVR_MESSAGE_DATA:
      default:
        status = -1;
        break;
    }
  }
  if (evrMessage_as[messageIdx].messageNotRead) status = -1;
  epicsMutexUnlock(evrMessage_as[messageIdx].messageRWMutex_ps);
  return status;
}

/*=============================================================================

  Name: evrMessageReport

  Abs:  Output Report on a Message Message

  Args: Type     Name           Access     Description
        -------  -------        ---------- ----------------------------
  unsigned int    messageIdx     Read       Index into Message Array
        char *    messageName_a  Read       Message Name ("PNET", "PATTERN", "DATA")

  Rem:  Printout information about this message space.  

  Side: Output to standard output.

  Return: 0 = OK, -1 = Failed
==============================================================================*/

int evrMessageReport(unsigned int  messageIdx, char *messageName_a)
{ 
  char timestamp_c[MAX_STRING_SIZE];

  if (messageIdx >= EVR_MESSAGE_MAX) return -1;
  timestamp_c[0]=0;
  epicsTimeToStrftime(timestamp_c, MAX_STRING_SIZE, "%a %b %d %Y %H:%M:%S.%f",
                      &evrMessage_as[messageIdx].resetTime_s);
  printf("%s counters reset at %s\n", messageName_a, timestamp_c);
  printf("Number of attempted message writes/rollover: %ld/%ld\n",
         evrMessage_as[messageIdx].updateCount,
         evrMessage_as[messageIdx].updateCountRollover);
  printf("Number of overwritten messages: %ld\n",
         evrMessage_as[messageIdx].overwriteCount);
  printf("Number of mutex lock errors: %ld\n",
         evrMessage_as[messageIdx].lockErrorCount);

  return 0;
}

/*=============================================================================

  Name: evrMessageCounts

  Abs:  Return message message diagnostic count values
        (for use by EPICS subroutine records)

  Args: Type     Name           Access     Description
        -------  -------        ---------- ----------------------------
  unsigned int   messageIdx       Read     Message Index
  double *       updateCount_p    Write    # times ISR wrote a message
  double * updateCountRollover_p  Write    # times above rolled over
  double *       overwriteCount_p Write    # times ISR overwrote a message
  double *       lockErrorCount_p Write    # times ISR had a mutex lock problem

  Rem:  The diagnostics count values are filled in if the message index is valid.
  
  Side: None

  Return: 0 = OK, -1 = Failed
==============================================================================*/

int evrMessageCounts(unsigned int  messageIdx,
                     double       *updateCount_p,
                     double       *updateCountRollover_p,
                     double       *overwriteCount_p,
                     double       *lockErrorCount_p)
{
  if (messageIdx >= EVR_MESSAGE_MAX ) return -1;
  *updateCount_p         = (double)evrMessage_as[messageIdx].updateCount;
  *updateCountRollover_p = (double)evrMessage_as[messageIdx].updateCountRollover;
  *overwriteCount_p      = (double)evrMessage_as[messageIdx].overwriteCount;
  *lockErrorCount_p      = (double)evrMessage_as[messageIdx].lockErrorCount;
  return 0;
}
/*=============================================================================

  Name: evrMessageCountReset

  Abs:  Reset message message diagnostic count values
        (for use by EPICS subroutine records)

  Args: Type     Name           Access     Description
        -------  -------        ---------- ----------------------------
  unsigned int   messageIdx     Read       Message Index

  Rem:  The diagnostics count values are reset if the message index is valid.
  
  Side: None

  Return: 0 = OK, -1 = Failed
==============================================================================*/

int evrMessageCountReset (unsigned int messageIdx)
{
  if (messageIdx >= EVR_MESSAGE_MAX ) return -1;
  evrMessage_as[messageIdx].updateCount         = 0;
  evrMessage_as[messageIdx].updateCountRollover = 0;
  evrMessage_as[messageIdx].overwriteCount      = 0;
  evrMessage_as[messageIdx].lockErrorCount      = 0;
  /* Save counter reset time for reporting purposes */
  epicsTimeGetCurrent(&evrMessage_as[messageIdx].resetTime_s);
  return 0;
}

/*=============================================================================

  Name: evrMessageIoscanpvt

  Abs:  Return the IOSCANPVT pointer for this message.

  Args: Type     Name           Access     Description
        -------  -------        ---------- ----------------------------
  unsigned int   messageIdx     Read       Message Index
  IOSCANPVT *    ioscanpvt_pp   Write      ioscanpvt pointer

  Rem:  The ioscanpvt pointer is returned if the message index is valid.
  
  Side: None

  Return: 0 = OK, -1 = Failed
==============================================================================*/

int evrMessageIoscanpvt (unsigned int messageIdx, IOSCANPVT *ioscanpvt_pp)
{
  if (messageIdx >= EVR_MESSAGE_MAX ) {
    *ioscanpvt_pp = 0;
    return -1;
  }
  *ioscanpvt_pp = evrMessage_as[messageIdx].ioscanpvt_p;
  if (!*ioscanpvt_pp) return -1;
  return 0;
}

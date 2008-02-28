/*=============================================================================

  Name: evrMessage.c
           evrMessageCreate    - Initialize Message Space
           evrMessageRegister  - Register Reader of the Message
           evrMessageWrite     - Write a Message
           evrMessageRead      - Read  a Message
           evrMessageStart     - Set Start Time of Message Processing
           evrMessageEnd       - Set End   Time of Message Processing
           evrMessageReport    - Report Information about the Message
           evrMessageCounts    - Get   Message Diagnostic Counts
           evrMessageCountReset- Reset Message Diagnostic Counts
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
#ifdef __rtems__
#include <bsp.h>                /* BSP*                        */
#include <basicIoOps.h>         /* for in_be32                 */
#endif

#include "dbAccess.h"           /* dbProcess,dbScan* protos    */
#include "epicsMutex.h"         /* epicsMutexId and protos     */
#include "epicsTime.h"          /* epicsTimeStamp              */
#include "errlog.h"             /* errlogPrintf                */
#include "evrMessage.h"         /* prototypes in this file     */

#define MAX_DELTA_TIME 1000000

typedef struct 
{
  epicsMutexId        messageRWMutex_ps;
  evrMessage_tu       message_u;
  dbCommon           *record_ps;
  epicsTimeStamp      resetTime_s;
  unsigned long       locked;
  unsigned long       messageNotRead;
  unsigned long       updateCount;
  unsigned long       updateCountRollover;
  unsigned long       overwriteCount;
  unsigned long       lockErrorCount;
  unsigned long       noDataCount;
  unsigned long       retryErrorCount;
  unsigned long       procTimeStart;
  unsigned long       procTimeEnd;
  unsigned long       procTimeDeltaStartMax;
  unsigned long       procTimeDeltaStartMin;
  unsigned long       procTimeDeltaMax;
  unsigned long       procTimeDeltaCount;
  unsigned long       procTimeDelta_a[MODULO720_COUNT];
  
} evrMessage_ts;

/* Maintain 4 messages - PNET, PATTERN, DATA (future), FIDUCIAL */
static evrMessage_ts evrMessage_as[EVR_MESSAGE_MAX] = {{0}, {0}, {0}, {0}};

/* Divisor to go from ticks to microseconds - initialize to no-op for linux */
static double        evrTicksPerUsec = 1;

#ifdef __rtems__
/*
 * From Till Straumann:
 * Macro for "Move From Time Base" to get current time in ticks.
 * The PowerPC Time Base is a 64-bit register incrementing usually
 * at 1/4 of the PPC bus frequency (which is CPU/Board dependent.
 * Even the 1/4 divisor is not fixed by the architecture).
 *
 * 'MFTB' just reads the lower 32-bit of the time base.
 */
#ifdef __PPC__
#define MFTB(var) asm volatile("mftb %0":"=r"(var))
#else
#define MFTB(var) do { var=0xdeadbeef; } while (0)
#endif
#endif
#ifdef linux
#define MFTB(var)  ((var)=1) /* make compiler happy */
#endif

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
  
  if (!strcmp(messageName_a, EVR_MESSAGE_PNET_NAME           )) {
    messageSize  = sizeof(epicsUInt32) * EVR_PNET_MODIFIER_MAX;
    messageIndex = EVR_MESSAGE_PNET;
  } else if (!strcmp(messageName_a, EVR_MESSAGE_PATTERN_NAME )) {
    messageSize  = sizeof(evrMessagePattern_ts);
    messageIndex = EVR_MESSAGE_PATTERN;
  } else if (!strcmp(messageName_a, EVR_MESSAGE_DATA_NAME    )) {
    messageSize  = sizeof(epicsUInt32) * EVR_DATA_MAX;
    messageIndex = EVR_MESSAGE_DATA;
  } else if (!strcmp(messageName_a, EVR_MESSAGE_FIDUCIAL_NAME)) {
    messageSize  = 0;
    messageIndex = EVR_MESSAGE_FIDUCIAL;
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

  /* Find divisor to go from ticks to microseconds
     (coarse resolution good enough). */
#ifdef __rtems__
  evrTicksPerUsec = ((double)BSP_bus_frequency/
                     (double)BSP_time_base_divisor)/1000;
#endif
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
  evrMessage_as[messageIdx].record_ps          = 0;
  evrMessage_as[messageIdx].messageNotRead     = 0;
  evrMessage_as[messageIdx].procTimeDeltaCount = 0;
  evrMessageCountReset(messageIdx);
  return messageIdx;
}

/*=============================================================================

  Name: evrMessageRegister

  Abs:  Register a Message Reader

  Args: Type     Name           Access     Description
        -------  -------        ---------- ----------------------------
        char *   messageName_a  Read       Message Name ("PNET", "PATTERN",
                                                         "DATA", "FIDUCIAL")
        size_t   messageSize    Read       Size of a message
        dbCommon *record_ps     Read       Pointer to Record Structure

  Rem:  The message index is found, the message is checked that
        it's been created, and a check is done that the message hasn't already
        been taken. The message index is returned if all is OK. 

  Side: None

  Return: -1 = Failed, 0,1,2 = message index
==============================================================================*/

int evrMessageRegister(char *messageName_a, size_t messageSize,
                       dbCommon *record_ps)
{
  int messageIdx = evrMessageIndex(messageName_a, messageSize);
  
  if (messageIdx < 0) return -1;
  if (!evrMessage_as[messageIdx].messageRWMutex_ps) {
    errlogPrintf("evrMessageRegister - Message %s not yet created\n",
                 messageName_a);
    return -1;
  }
  
  if (evrMessage_as[messageIdx].record_ps) {
    errlogPrintf("evrMessageRegister - Message %s already has a reader\n",
                 messageName_a);
    return -1;
  }
  evrMessage_as[messageIdx].record_ps = record_ps;    
  
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
  if (messageIdx >= EVR_MESSAGE_MAX) return -1;

  /* Attempt to lock the message */
  if (evrMessage_as[messageIdx].locked) {
    evrMessage_as[messageIdx].lockErrorCount++;
  }
  if (evrMessage_as[messageIdx].messageNotRead) {
    evrMessage_as[messageIdx].overwriteCount++;
  }
  /* Update message in holding array */
  evrMessage_as[messageIdx].message_u      = *message_pu;
  evrMessage_as[messageIdx].messageNotRead = 1;
  return 0;
}

/*=============================================================================

  Name: evrMessageProcess

  Abs:  Process a record that processes the Message

  Args: Type     Name           Access     Description
        -------  -------        ---------- ----------------------------
  unsigned int   messageIdx     Read       Index into Message Array

  Rem:  Calls record processing.

  Side: None.

  Return: 0 = OK
==============================================================================*/

int evrMessageProcess(unsigned int messageIdx)
{
  if (messageIdx >= EVR_MESSAGE_MAX) return -1;
  if (!interruptAccept)              return -1;
  
  /* Process record that wants the message. */
  if (evrMessage_as[messageIdx].record_ps) {
    dbScanLock(evrMessage_as[messageIdx].record_ps);
    dbProcess(evrMessage_as[messageIdx].record_ps);
    dbScanUnlock(evrMessage_as[messageIdx].record_ps);
    evrMessageEnd(messageIdx);
  }
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

  Return: 0 = OK, 1 = Input Error, 2 = Lock Error, 3 = No Data Available,
          4 = Data Overwritten
==============================================================================*/

evrMessageReadStatus_te evrMessageRead(unsigned int  messageIdx,
                                       evrMessage_tu *message_pu)
{
  evrMessageReadStatus_te status;
  int retry;
  
  if (messageIdx >= EVR_MESSAGE_MAX) return evrMessageInpError;

  /* Attempt to lock the message */
  if ((!evrMessage_as[messageIdx].messageRWMutex_ps) ||
      (epicsMutexLock(evrMessage_as[messageIdx].messageRWMutex_ps) !=
       epicsMutexLockOK)) return evrMessageLockError;
  evrMessage_as[messageIdx].locked = 1;
  if (!evrMessage_as[messageIdx].messageNotRead) {
    status = evrMessageDataNotAvail;
    evrMessage_as[messageIdx].noDataCount++;
  } else {
    status = evrMessageOK;
     /* Read the message only if its still available.  Retry in case
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
      case EVR_MESSAGE_FIDUCIAL:
        break;
      case EVR_MESSAGE_DATA:
      default:
        status = evrMessageInpError;
        break;
      }
    }
    if (evrMessage_as[messageIdx].messageNotRead) {
      status = evrMessageDataOverwrite;
      evrMessage_as[messageIdx].retryErrorCount++;
    }
  }
  evrMessage_as[messageIdx].locked = 0;
  epicsMutexUnlock(evrMessage_as[messageIdx].messageRWMutex_ps);
  return status;
}

/*=============================================================================

  Name: evrMessageStart

  Abs:  Set Start Time of Message Processing

  Args: Type     Name           Access     Description
        -------  -------        ---------- ----------------------------
  unsigned int    messageIdx     Read       Index into Message Array

  Rem:  None.

  Side: None.

  Return: 0 = OK, -1 = Failed
==============================================================================*/

int evrMessageStart(unsigned int messageIdx)
{
  unsigned long prevTimeStart, deltaTimeStart;
  
  if (messageIdx >= EVR_MESSAGE_MAX) return -1;

  /* Get time when processing starts */
  prevTimeStart = evrMessage_as[messageIdx].procTimeStart;
  MFTB(evrMessage_as[messageIdx].procTimeStart);
  deltaTimeStart = evrMessage_as[messageIdx].procTimeStart - prevTimeStart;
  if (deltaTimeStart < MAX_DELTA_TIME) {
    if (evrMessage_as[messageIdx].procTimeDeltaStartMax < deltaTimeStart)
      evrMessage_as[messageIdx].procTimeDeltaStartMax = deltaTimeStart;
    if (evrMessage_as[messageIdx].procTimeDeltaStartMin > deltaTimeStart)
      evrMessage_as[messageIdx].procTimeDeltaStartMin = deltaTimeStart;
  }
  
  /* Reset time that processing ends */
  evrMessage_as[messageIdx].procTimeEnd = 0;

  /* Update diagnostics counters */
  if (evrMessage_as[messageIdx].updateCount < EVR_MAX_INT) {
    evrMessage_as[messageIdx].updateCount++;
  } else {
    evrMessage_as[messageIdx].updateCountRollover++;
    evrMessage_as[messageIdx].updateCount = 0;
  }
  return 0;
}

/*=============================================================================

  Name: evrMessageEnd

  Abs:  Set End Time of Message Processing

  Args: Type     Name           Access     Description
        -------  -------        ---------- ----------------------------
  unsigned int    messageIdx     Read       Index into Message Array

  Rem:  None.

  Side: None.

  Return: 0 = OK, -1 = Failed
==============================================================================*/

int evrMessageEnd(unsigned int messageIdx)
{
  evrMessage_ts *em_ps = evrMessage_as + messageIdx;
  
  if (messageIdx >= EVR_MESSAGE_MAX) return -1;
  /* Attempt to lock the message */
  if ((!em_ps->messageRWMutex_ps) ||
      (epicsMutexLock(em_ps->messageRWMutex_ps) != epicsMutexLockOK) ||
      (em_ps->procTimeEnd != 0)) return -1;

  /* Get end of processing time */
  MFTB(em_ps->procTimeEnd);

  /* Update array of delta times used later to calc avg.
     Keep track of maximum. */
  em_ps->procTimeDelta_a[em_ps->procTimeDeltaCount] =
    em_ps->procTimeEnd - em_ps->procTimeStart;
  if (em_ps->procTimeDeltaMax <
      em_ps->procTimeDelta_a[em_ps->procTimeDeltaCount]) {
    em_ps->procTimeDeltaMax =
      em_ps->procTimeDelta_a[em_ps->procTimeDeltaCount];
  }
  em_ps->procTimeDeltaCount++;
  if (em_ps->procTimeDeltaCount >= MODULO720_COUNT) {
    em_ps->procTimeDeltaCount = MODULO720_COUNT-1;
  }  
  epicsMutexUnlock(em_ps->messageRWMutex_ps);
  return 0;
}

/*=============================================================================

  Name: evrMessageReport

  Abs:  Output Report on a Message Message

  Args: Type     Name           Access     Description
        -------  -------        ---------- ----------------------------
  unsigned int    messageIdx     Read       Index into Message Array
        char *    messageName_a  Read       Message Name ("PNET", "PATTERN",
                                                          "DATA", "FIDUCIAL")
        int       interest       Read       Interest level

  Rem:  Printout information about this message space.  

  Side: Output to standard output.

  Return: 0 = OK, -1 = Failed
==============================================================================*/

int evrMessageReport(unsigned int  messageIdx, char *messageName_a,
                     int interest)
{ 
  char timestamp_c[MAX_STRING_SIZE];
  int  idx;
  
  if (messageIdx >= EVR_MESSAGE_MAX) return -1;
  if (interest <= 0) return 0;
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
  printf("Number of no data available errors: %ld\n",
         evrMessage_as[messageIdx].noDataCount);
  printf("Number of read retry errors: %ld\n",
         evrMessage_as[messageIdx].retryErrorCount);
  printf("Maximum proc time delta (us) = %lf\n",
         (double)evrMessage_as[messageIdx].procTimeDeltaMax/evrTicksPerUsec);
  printf("Max/Min proc start time deltas (us) = %lf/%lf\n",
         (double)evrMessage_as[messageIdx].procTimeDeltaStartMax/
         evrTicksPerUsec,
         (double)evrMessage_as[messageIdx].procTimeDeltaStartMin/
         evrTicksPerUsec);
  if (interest > 1) {
    int count = 25;
    if (interest > 2) count = MODULO720_COUNT;
    printf("Last %d proc time deltas (us):\n", count);
    for (idx=0; idx<count; idx++) {
      printf("  %d: %lf\n", idx,
             (double)evrMessage_as[messageIdx].procTimeDelta_a[idx]/
             evrTicksPerUsec);
    }
  }
  return 0;
}

/*=============================================================================

  Name: evrMessageCounts

  Abs:  Return message diagnostic count values
        (for use by EPICS subroutine records)

  Args: Type     Name           Access     Description
        -------  -------        ---------- ----------------------------
  unsigned int   messageIdx       Read     Message Index
  double *       updateCount_p    Write    # times ISR wrote a message
  double * updateCountRollover_p  Write    # times above rolled over
  double *       overwriteCount_p Write    # times ISR overwrote a message
  double *       lockErrorCount_p Write    # times ISR had a mutex lock problem
  double *       noDataCount_p    Write    # times no data was available for a read
  double *       retryErrorCount_p  Write  # times ran out of retries for a read
  double *       procTimeStartMin_p Write  Min start time delta (us)
  double *       procTimeStartMax_p Write  Max start time delta (us)
  double *       procTimeDeltaAvg_p Write  Avg time for message processing (us)
  double *       procTimeDeltaMax_p Write  Max time for message processing (us)

  Rem:  The diagnostics count values are filled in if the message index is valid.
  
  Side: None

  Return: 0 = OK, -1 = Failed
==============================================================================*/

int evrMessageCounts(unsigned int  messageIdx,
                     double       *updateCount_p,
                     double       *updateCountRollover_p,
                     double       *overwriteCount_p,
                     double       *lockErrorCount_p,
                     double       *noDataCount_p,
                     double       *retryErrorCount_p,
                     double       *procTimeStartMin_p,
                     double       *procTimeStartMax_p,
                     double       *procTimeDeltaAvg_p,
                     double       *procTimeDeltaMax_p)
{  
  evrMessage_ts *em_ps = evrMessage_as + messageIdx;
  int    idx;

  if (messageIdx >= EVR_MESSAGE_MAX ) return -1;
  *updateCount_p         = (double)em_ps->updateCount;
  *updateCountRollover_p = (double)em_ps->updateCountRollover;
  *overwriteCount_p      = (double)em_ps->overwriteCount;
  *lockErrorCount_p      = (double)em_ps->lockErrorCount;
  *noDataCount_p         = (double)em_ps->noDataCount;
  *retryErrorCount_p     = (double)em_ps->retryErrorCount;
  *procTimeStartMin_p    = (double)em_ps->procTimeDeltaStartMin/
                           evrTicksPerUsec;
  *procTimeStartMax_p    = (double)em_ps->procTimeDeltaStartMax/
                           evrTicksPerUsec;
  *procTimeDeltaMax_p    = (double)em_ps->procTimeDeltaMax/evrTicksPerUsec;
  *procTimeDeltaAvg_p    = 0;
  if  (em_ps->procTimeDeltaCount > 0) {
    for (idx = 0; idx < em_ps->procTimeDeltaCount; idx++) {
      *procTimeDeltaAvg_p += (double)em_ps->procTimeDelta_a[idx];
    }
    *procTimeDeltaAvg_p /= em_ps->procTimeDeltaCount;
    *procTimeDeltaAvg_p /= evrTicksPerUsec;
    em_ps->procTimeDeltaCount = 0;
  }  
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
  evrMessage_as[messageIdx].updateCount           = 0;
  evrMessage_as[messageIdx].updateCountRollover   = 0;
  evrMessage_as[messageIdx].overwriteCount        = 0;
  evrMessage_as[messageIdx].lockErrorCount        = 0;
  evrMessage_as[messageIdx].noDataCount           = 0;
  evrMessage_as[messageIdx].retryErrorCount       = 0;
  evrMessage_as[messageIdx].procTimeDeltaMax      = 0;
  evrMessage_as[messageIdx].procTimeDeltaStartMax = 0;
  evrMessage_as[messageIdx].procTimeDeltaStartMin = MAX_DELTA_TIME;
  /* Save counter reset time for reporting purposes */
  epicsTimeGetCurrent(&evrMessage_as[messageIdx].resetTime_s);
  return 0;
}

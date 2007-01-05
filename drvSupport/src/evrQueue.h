/*=============================================================================
 
  Name: evrQueue.h

  Abs:  This include file contains definitions and typedefs shared by
        evrQueue.c, devWFevrQueue.c, drvPnet.c, mpgPattern.c, evrPattern.c
        for EVR/PNET queue access.

  Auth: 21-Dec-2006, S. Allison
 
-----------------------------------------------------------------------------*/
#include "copyright_SLAC.h"    
/*----------------------------------------------------------------------------- 
  Mod:  (newest to oldest)  
 
=============================================================================*/

#ifndef INCevrQueueH
#define INCevrQueueH 

#ifdef __cplusplus
extern "C" {
#endif

#include    <stddef.h>             /* size_t                 */
#include    "dbScan.h"             /* IOSCANPVT              */
#include    "epicsMessageQueue.h"  /* epicsMessageQueueId    */
#include    "epicsTime.h"          /* epicsTimeStamp         */

#define  EVR_QUEUE_PNET_NAME              "PNET"
#define  EVR_QUEUE_PATTERN_NAME           "PATTERN"
#define  EVR_QUEUE_DATA_NAME              "DATA"
#define  EVR_QUEUE_PNET                   0
#define  EVR_QUEUE_PATTERN                1
#define  EVR_QUEUE_DATA                   2
#define  EVR_QUEUE_MAX                    3

#define  EVR_QUEUE_PATTERN_VERSION        1
#define  EVR_QUEUE_DATA_VERSION           1
  
/* This number should rollover every 69 days at 360 Hz */
#define  EVR_MAX_INT  (2147483647)    /* 4 byte int - 1 bit for sign */
  
#define  EVR_PNET_MODIFIER_MAX            4 /* Number of PNET modifiers */

typedef struct 
{
  epicsMessageQueueId messageQueueId;
  size_t              messageSize;
  IOSCANPVT           ioscanpvt;
  long int            receiverRegistered;
  long int            trySendCount;
  long int            trySendCountRollover;
  long int            noSendCount;
  epicsTimeStamp      createTime_s;
  
} evrQueue_ts;

typedef evrQueue_ts * evrQueueId;
  
/* Waveform header in waveform sent by the EVG and received by the EVR */
typedef struct {
  epicsUInt16 type;
  epicsUInt16 version;
} evrQueueHeader_ts;

/* Waveform sent by the EVG and received by the EVR */
typedef struct {
  evrQueueHeader_ts header_s;
  epicsUInt32       modifier_a[EVR_PNET_MODIFIER_MAX];
  epicsUInt32       modifier5;
  epicsUInt32       bunchcharge;
  epicsTimeStamp    time;     /* epics timestamp:                        */
                              /* 1st 32 bits = # of seconds since 1990   */
                              /* 2nd 32 bits = # of nsecs since last sec */
                              /*           except lower 17 bits = pulsid */
} evrQueuePattern_ts;

evrQueueId evrQueueCreate  (char         *queueName_a, size_t  messageSize);
evrQueueId evrQueueRegister(char         *queueName_a, size_t  messageSize);
int        evrQueueSend    (evrQueueId    evrQueue_ps, void   *message_p);
int        evrQueueReceive (evrQueueId    evrQueue_ps, void   *message_p);
int        evrQueueReport  (evrQueueId    evrQueue_ps, char   *queueName_a);
int        evrQueueCounts  (unsigned int  queueIdx,
                            double       *trySendCount_p,
                            double       *trySendCountRollover_p,
                            double       *noSendCount_p);
int     evrQueueCountReset (unsigned int queueIdx);
  
#ifdef __cplusplus
}
#endif

#endif /*INCevrQueueH*/

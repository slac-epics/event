/*=============================================================================
 
  Name: evrMessage.h

  Abs:  This include file contains definitions and typedefs shared by
        evrMessage.c, devWFevrMessage.c, drvPnet.c, mpgPattern.c, evrPattern.c
        for EVR/PNET message access.

  Auth: 21-Dec-2006, S. Allison
        06-Feb-2007, DRogind added edefAvgDoneMask to evrMessagePattern_ts
 
-----------------------------------------------------------------------------*/
#include "copyright_SLAC.h"    
/*----------------------------------------------------------------------------- 
  Mod:  (newest to oldest)  
 
=============================================================================*/

#ifndef INCevrMessageH
#define INCevrMessageH 

#include    <stddef.h>             /* size_t                 */
#include    "dbScan.h"             /* IOSCANPVT              */
#include    "epicsTypes.h"         /* epicsUInt32, 16        */
#include    "epicsTime.h"          /* epicsTimeStamp         */

#ifdef __cplusplus
extern "C" {
#endif

#define  EVR_MESSAGE_PNET_NAME              "PNET"
#define  EVR_MESSAGE_PATTERN_NAME           "PATTERN"
#define  EVR_MESSAGE_DATA_NAME              "DATA"
#define  EVR_MESSAGE_PNET                   0
#define  EVR_MESSAGE_PATTERN                1
#define  EVR_MESSAGE_DATA                   2
#define  EVR_MESSAGE_MAX                    3

#define  EVR_MESSAGE_PATTERN_VERSION        1
#define  EVR_MESSAGE_DATA_VERSION           1
  
/* This number should rollover every 69 days at 360 Hz */
#define  EVR_MAX_INT  (2147483647)    /* 4 byte int - 1 bit for sign */
  
#define  EVR_PNET_MODIFIER_MAX            4 /* Number of PNET modifiers   */
#define  EVR_DATA_MAX                     9 /* Number of data epicsUInt32 */
  
/* Waveform header in waveform sent by the EVG and received by the EVR */
typedef struct {
  epicsUInt16 type;
  epicsUInt16 version;
} evrMessageHeader_ts;

/* Waveform sent by the MPG and read by the EVG IOC */
typedef struct {
  epicsUInt32         modifier_a[EVR_PNET_MODIFIER_MAX];
} evrMessagePnet_ts;
  
/* Waveform sent by the EVG and received by the EVR */
typedef struct {
  evrMessageHeader_ts header_s;
  evrMessagePnet_ts   pnet_s;
  epicsUInt32         modifier5;
  epicsUInt32         bunchcharge;
  epicsTimeStamp      time;     /* epics timestamp:                        */
                                /* 1st 32 bits = # of seconds since 1990   */
                                /* 2nd 32 bits = # of nsecs since last sec */
                                /*           except lower 17 bits = pulsid */
  epicsUInt32         edefAvgDoneMask;
  epicsUInt32         edefMinorMask;
  epicsUInt32         edefMajorMask;
  epicsUInt32         edefInitMask;
} evrMessagePattern_ts;
  
typedef union
{
  evrMessagePnet_ts    pnet_s;
  evrMessagePattern_ts pattern_s;
  epicsUInt32         *data_a;
} evrMessage_tu;


int evrMessageCreate    (char         *messageName_a, size_t  messageSize);
int evrMessageRegister  (char         *messageName_a, size_t  messageSize);
int evrMessageRead      (unsigned int  messageIdx, evrMessage_tu *message_pu);
int evrMessageWrite     (unsigned int  messageIdx, evrMessage_tu *message_pu);
int evrMessageReport    (unsigned int  messageIdx,    char   *messageName_a);
int evrMessageCounts    (unsigned int  messageIdx,
                         double       *updateCount_p,
                         double       *updateCountRollover_p,
                         double       *overwriteCount_p,
                         double       *lockErrorCount_p);
int evrMessageCountReset(unsigned int messageIdx);
int evrMessageIoscanpvt (unsigned int messageIdx, IOSCANPVT *ioscanpvt_pp);
  
#ifdef __cplusplus
}
#endif

#endif /*INCevrMessageH*/

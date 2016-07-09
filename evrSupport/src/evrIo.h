/*=============================================================================
 *  
 *    Name: evrIo.h
 *
 *      Abs:  This include file contains typedefs shared by
 *             drvEvr.c, bsa.c
 *             for moving a slower function to a lower priority task.
 *
 *
 *        Auth: 08-July-2016, Stephanie Allison and Carolina Bianchini
 *         
 *         -----------------------------------------------------------------------------*/
#include "copyright_SLAC.h"
/*----------------------------------------------------------------------------- 
 *   Mod:  (newest to oldest)  
 *           DD-MMM-YYYY, My Name:
 *                      Changed such and such to so and so. etc. etc.
 *                              DD-MMM-YYYY, Your Name:
 *                                         More changes ... The ordering of the revision history 
 *                                                    should be such that the NEWEST changes are at the HEAD of
 *                                                               the list.
 *                                                                
 *                                                                =============================================================================*/

#ifndef INCevrIoH
#define INCevrIoH 

#ifdef __cplusplus
extern "C" {
#endif

#include    "epicsTime.h"          /* epicsTimeStamp       */
#include    "epicsMessageQueue.h"

extern epicsMessageQueueId  eventTaskQueue;

typedef enum {
  evrIoEventId=0, evrIoBsaId=1
} evrIoId_te;

typedef struct {
  epicsTimeStamp edefTimeInit_s;
  epicsTimeStamp edefTime_s;
  epicsUInt32    edefAllDone;
  int            edefAvgDone;
  epicsEnum16    edefSevr;
  epicsUInt32    modifier5;
} evrIoBsaChecker_ts;

typedef struct {
  evrIoId_te function;
  union {
    epicsInt16 eventNum;
    evrIoBsaChecker_ts bsaChecker_s;
  } io_u;
} evrIoMessage_ts;


#ifdef __cplusplus
}
#endif

#endif /*INCevrIoH*/


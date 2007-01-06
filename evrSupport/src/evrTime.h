/*=============================================================================
 
  Name: evrTime.h

  Abs:  This include file contains external prototypes for EVR 
        timestamp routines that allow access to the EVR timestamp
        table (timestamp received in the EVG broadcast) 

  Auth: 17 NOV-2006, drogind created 
 
-----------------------------------------------------------------------------*/
#include "copyright_SLAC.h"    
/*----------------------------------------------------------------------------- 
  Mod:  (newest to oldest)  
        DD-MMM-YYYY, My Name:
           Changed such and such to so and so. etc. etc.
        DD-MMM-YYYY, Your Name:
           More changes ... The ordering of the revision history 
           should be such that the NEWEST changes are at the HEAD of
           the list.
 
=============================================================================*/

#ifndef INCevrTimeH
#define INCevrTimeH 

#ifdef __cplusplus
extern "C" {
#endif

#include    "epicsTime.h"   /* epicsTimeStamp */
  
/* Masks used to decode pulse ID from the nsec part of the timestamp */
#define UPPER_15_BIT_MASK       (0xFFFE0000)    /* (2^32)-1 - (2^17)-1 */
#define LOWER_17_BIT_MASK       (0x0001FFFF)    /* (2^17)-1            */
/* Pulse ID Definitions */
#define PULSEID_BIT17           (0x00020000)    /* Bit up from pulse ID*/
#define PULSEID_INVALID         (0x0001FFFF)    /* Invalid Pulse ID    */
#define PULSEID_MAX             (0x0001FFDF)    /* Rollover value      */
#define PULSEID_MAX_MINUS_2     (0x0001FFDD)    /* Rollover value - 2  */
#define PULSEID_RESYNC          (0x0001E000)    /* Resync Pulse ID     */
#define NSEC_PER_SEC            1E9             /* # nsec in one sec   */

/* Masks used to decode beam code and YY from modifier1 */
#define BEAMCODE_BIT_MASK       (0x0000001F)  /* Beam code mask        */
                                              /* Left shift 8 first    */
#define YY_BIT_MASK             (0x000000FF)  /* YY code mask          */

/* Mask used to decode timeslot 1 to 6 from modifier2   */
#define TIMESLOT_MASK           (0x0000003F)  /* timeslot   mask       */
#define TIMESLOT1_MASK          (0x00000001)  /* timeslot 1 mask       */
#define TIMESLOT2_MASK          (0x00000002)  /* timeslot 2 mask       */
#define TIMESLOT3_MASK          (0x00000004)  /* timeslot 3 mask       */
#define TIMESLOT4_MASK          (0x00000008)  /* timeslot 4 mask       */
#define TIMESLOT5_MASK          (0x00000010)  /* timeslot 5 mask       */
#define TIMESLOT6_MASK          (0x00000020)  /* timeslot 6 mask       */

/* For status */
typedef enum {
  evrTimeOK, evrTimeLastGood, evrTimeInvalid
} evrTimeStatus_te;

/* For time ID */
typedef enum {
  evrTimeCurrent=0, evrTimeNext1=1, evrTimeNext2=2, evrTimeNext3=3
} evrTimeId_te;
#define  MAX_EVR_TIME  4

int evrTimeGet       (epicsTimeStamp  *epicsTime_ps, evrTimeId_te id);
int evrTimePut       (epicsTimeStamp  *epicsTime_ps, evrTimeStatus_te status);
int evrTimePutPulseID(epicsTimeStamp  *epicsTime_ps, unsigned int pulseID);

#ifdef __cplusplus
}
#endif

#endif /*INCevrTimeH*/

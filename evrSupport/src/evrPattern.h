/*=============================================================================
 
  Name: evrPattern.h

  Abs:  This include file contains definitions and typedefs shared by
        evrPattern.c and mpgPattern.c for EVR patterns.

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

#ifndef INCevrPattH
#define INCevrPattH 

#ifdef __cplusplus
extern "C" {
#endif

#include    "epicsTypes.h"  /* epicsUInt32    */
#include    "epicsTime.h"   /* epicsTimeStamp */

/* Definitions and typedefs shared by evrPattern.c and mpgPattern.c  */
  
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

/* Waveform header defines */
#define EVR_WF_PATTERN          1  /* Waveform is a pattern    */
#define EVR_WF_PATTERN_VERSION  1  /* Waveform pattern version */
  
/* Waveform header in waveform sent by the EVG and received by the EVR */
typedef struct {
  epicsUInt16 type;
  epicsUInt16 version;
} evrWaveformHeader_ts;

/* Part of the waveform that is the Pnet pattern */
typedef struct {
  epicsUInt32 modifier1;
  epicsUInt32 modifier2;
  epicsUInt32 modifier3;
  epicsUInt32 modifier4;
} evrPatternPnet_ts;

/* Waveform sent by the EVG and received by the EVR */
typedef struct {
  evrWaveformHeader_ts header_s;
  evrPatternPnet_ts    pnet_s;
  epicsUInt32          modifier5;
  epicsUInt32          bunchcharge;    
  epicsTimeStamp       time;  /* epics timestamp:                        */
                              /* 1st 32 bits = # of seconds since 1990   */
                              /* 2nd 32 bits = # of nsecs since last sec */
                              /*           except lower 17 bits = pulsid */
} evrPatternWaveform_ts;

#ifdef __cplusplus
}
#endif

#endif /*INCevrPattH*/

/*=============================================================================

  Name: mpgEvent.c
        mpgEventSeqInit   - MPG 360Hz Seq Ram Preparation Initialization
        mpgEventSeq       - MPG 360Hz Seq Ram Preparation
        mpgEventSeqSwitch - MPG 360Hz Seq Ram Switch
        mpgEventSeqSend   - MPG 360Hz Seq Ram Send (for test stand)
        mpgEventCode      - MPG Event Code Setup
	mpgEventBeamCode  - MPG 360Hz Beam Code Event Processing
	mpgEventCounts    - Return Diagnostic Count Values
	mpgEventCountReset- Reset Diagnostic Count Values

  Abs: This file contains all subroutine support for the MPG IOC processing
       of 360Hz EVG sequence RAM/event code programming.

  Rem: All functions called by the subroutine record get passed one argument:

         psub                       Pointer to the subroutine record data.
          Use:  pointer
          Type: struct subRecord *
          Acc:  read/write
          Mech: reference

         All functions return a long integer.  0 = OK, -1 = ERROR.
         The subroutine record ignores the status returned by the Init
         routines.  For the calculation routines, the record status (STAT) is
         set to SOFT_ALARM (unless it is already set to LINK_ALARM due to
         severity maximization) and the severity (SEVR) is set to psub->brsv
         (BRSV - set by the user in the database though it is expected to
          be invalid).

  Auth: 12-feb-2007, S. Allison (saa)
  Rev:  

-----------------------------------------------------------------------------*/

#include "copyright_SLAC.h"     /* SLAC copyright comments */

/*-----------------------------------------------------------------------------

  Mod:  (newest to oldest)

=============================================================================*/

#include <string.h>           /* for memset                 */

#include "debugPrint.h"
#include "subRecord.h"        /* for struct subRecord        */
#include "sSubRecord.h"       /* for struct sSubRecord       */
#include "registryFunction.h" /* for epicsExport             */
#include "epicsExport.h"      /* for epicsRegisterFunction   */
#include "epicsMutex.h"       /* EPICS Mutex support library */
#include "ellLib.h"           /* EPICS Linked list library   */

#include "evrMessage.h"       /* EVENT_FIDUCIAL              */
#include "evrPattern.h"       /* Various bit masks           */
#include "egRecord.h"         /* egMOD1_Off, egMOD1_Single   */
#include "drvMrfEg.h"         /* MRF_NUM_EVENTS, Eg protos   */

#define EVENTS_PER_TIMESLOT 10 /* # events per timeslot      */
#define EVENTS_PER_TIMESLOT_ALWAYS 3 /* # events always sent */
#define EVENTS_PER_BEAMCODE  7 /* # events per beamcode      */
#define EVENTS_PER_BEAMCODE_ALWAYS 2 /* # events always sent */

/* Detail for one event code */
typedef struct {
  ELLNODE         node_s;
  unsigned int    code;
  int             enable;
  int             everyCycle;
  int             numPulses;
  unsigned short  ramPos_a[MRF_NUM_SEQ_RAM];
  MrfEvgSeqStruct ram_as[MRF_NUM_SEQ_RAM];
  
} mpgEvent_ts;

/* Detail for one sequence RAM */
typedef struct {
  int    change;
  int    enable;
  int    busy;  
} mpgRam_ts;

/* Tables and Lists */
static ELLLIST        eventList_s;       /* Event list in sequence RAM order */
static mpgEvent_ts    mpgEvent_as[MRF_NUM_EVENTS]; /* event code table       */
static mpgRam_ts      mpgRam_as[MRF_NUM_SEQ_RAM];  /* ram flags              */
static epicsUInt16    tsEventCode_aa[TIMESLOT_MAX][EVENTS_PER_TIMESLOT];
/* Modulo36 event code - incremented on every pulse */
static epicsUInt16    mod36EventCode   = EVENT_MODULO36_MIN;

/* Control Flags */
static unsigned int   eventEnable      = 0; /* global event enable flag      */
static unsigned int   ramNext          = 0; /* next sequence RAM             */
static epicsMutexId   mpgEventMutex_ps = 0; /* Mutex for above information   */
static unsigned int   lockStatus       = 0; /* Mutex lock status             */
#ifdef __rtems__
static EgCardStruct  *evgCard_ps       = 0; /* EVG card information          */
static unsigned int   evgLockStatus    = 0; /* Mutex lock status             */
#endif

/* Diagnostics Counters */
static unsigned int   lockErrCount     = 0; /* # times unable to lock data   */
static unsigned int   busyErrCount     = 0; /* # times a seq ram was busy    */
static unsigned int   modeErrCount     = 0; /* # times a seq ram is not in single modes */
static unsigned int   enableErrCount   = 0; /* # times a seq ram still active*/
static unsigned int   patternErrCount  = 0; /* # bad patterns                */

/*=============================================================================

  Name: mpgEventSeqInit

  Abs:  Initialization for 360Hz Sequence RAM Processing

  Args: Type                Name        Access     Description
        ------------------- ----------- ---------- ----------------------------
        subRecord *         psub        read       point to subroutine record

  Rem:  Initialization subroutine for IOC:<LOCA>:<UNIT>:TIMESLOT

  Inputs:
       None
     
  Outputs:
       Static data initialized.
       EVG sequence ram initialized.
       EVG sequence mode set to Off.
  
==============================================================================*/ 
static int mpgEventSeqInit(subRecord *psub)
{
  int tidx;
  int idx;
  int timeslot;
  int eventcode;
  int ram;
  
  /* For RTEMS, get the EVG card information and disable both sequence rams. */
#ifdef __rtems__
  evgCard_ps = (EgCardStruct *)EgGetCardStruct(0);
#endif
  
  /* Make sure this routine is called only once - should only be
     one record with this init subroutine */
  if (mpgEventMutex_ps) return -1;
  /* Create read/write mutex around shared data */
  if (!(mpgEventMutex_ps = epicsMutexCreate())) return -1;
  /* Initialize event code list used for sequence ram loading */
  ellInit(&eventList_s);
  /* Initialize time slot table */
  for (tidx = 0, timeslot = TIMESLOT_MIN; tidx < TIMESLOT_MAX;
       tidx++,   timeslot++) {
    /* First set up the event code for the other time slot */
    eventcode = timeslot + (TIMESLOT_MAX/2);
    if (eventcode > TIMESLOT_MAX) eventcode -= TIMESLOT_MAX;
    eventcode *= EVENTS_PER_TIMESLOT;
    idx        = 0;
    tsEventCode_aa[tidx][idx] = eventcode;
    /* Now set up the event codes for this time slot. */
    for (idx++, eventcode = timeslot * EVENTS_PER_TIMESLOT;
         idx < EVENTS_PER_TIMESLOT; idx++, eventcode++) {
      tsEventCode_aa[tidx][idx] = eventcode;
    }
  }
  /* Start with all events disabled.  RAM 1 will be next. */
  eventEnable = 0; /* disabled until enabled by mpgEventSeqPrep */
  ramNext     = 1; /* next sequence ram to go */

  /* Initialize event code table */
  memset(mpgEvent_as, 0, sizeof(mpgEvent_ts) * MRF_NUM_EVENTS);
  /* Initialize codes that are reserved for post-event and external
     trigger so they cannot be used here */
  mpgEvent_as[EVENT_EXTERNAL_TRIG].code      = EVENT_EXTERNAL_TRIG;
  mpgEvent_as[EVENT_MODULO720].code          = EVENT_MODULO720;
  for (eventcode = EVENT_EDEFINIT_MIN; eventcode <= EVENT_EDEFINIT_MAX;
       eventcode++) {
    mpgEvent_as[eventcode].code = eventcode;
  }
  /* Initialize codes that are reserved for MOD36 event codes,
     except for the first one which is set up like other event codes. */
  for (eventcode = EVENT_MODULO36_MIN+1; eventcode <= EVENT_MODULO36_MAX;
       eventcode++) {
    mpgEvent_as[eventcode].code = eventcode;
  }
  /* Initialize the start-of-sequence fiducial event  - this is NOT included
     in the linked list since it must ALWAYS be first */
  mpgEvent_as[EVENT_FIDUCIAL].code           = EVENT_FIDUCIAL;
  mpgEvent_as[EVENT_FIDUCIAL].enable         = 1;
  mpgEvent_as[EVENT_FIDUCIAL].everyCycle     = 1;
  /* Initialize the heartbeat event  - this is NOT included
     in the linked list since it is internal. */
  mpgEvent_as[EVENT_HEARTBEAT].code          = EVENT_HEARTBEAT;
  mpgEvent_as[EVENT_HEARTBEAT].enable        = 1;
  mpgEvent_as[EVENT_HEARTBEAT].everyCycle    = 1;
  /* Initialize the end-of-sequence event  - this is NOT included in the
     linked list since the delay (Timestamp) is unknown until other
     events are added */
  mpgEvent_as[EVENT_END_SEQUENCE].code       = EVENT_END_SEQUENCE;
  mpgEvent_as[EVENT_END_SEQUENCE].enable     = 1;
  mpgEvent_as[EVENT_END_SEQUENCE].everyCycle = 1;
  /* Initialize the sequence RAMs with the fiducial and
     end-of-sequence events */
  for (idx=0, ram=1; idx<MRF_NUM_SEQ_RAM; idx++, ram++) {
    mpgRam_as[idx].change = 0;
    mpgRam_as[idx].enable = 0;
    mpgRam_as[idx].busy   = 0;
    mpgEvent_as[EVENT_FIDUCIAL].ramPos_a[idx]             = 0;
    mpgEvent_as[EVENT_FIDUCIAL].ram_as[idx].EventCode     = EVENT_FIDUCIAL;
    mpgEvent_as[EVENT_FIDUCIAL].ram_as[idx].Timestamp     = 0;
    mpgEvent_as[EVENT_HEARTBEAT].ramPos_a[idx]            = 1;
    mpgEvent_as[EVENT_HEARTBEAT].ram_as[idx].EventCode    = EVENT_HEARTBEAT;
    mpgEvent_as[EVENT_HEARTBEAT].ram_as[idx].Timestamp    = 1;
    mpgEvent_as[EVENT_END_SEQUENCE].ramPos_a[idx]         = 2;
    mpgEvent_as[EVENT_END_SEQUENCE].ram_as[idx].EventCode = EVENT_END_SEQUENCE;
    mpgEvent_as[EVENT_END_SEQUENCE].ram_as[idx].Timestamp = 2;
#ifdef __rtems__
    if (evgCard_ps) {
      /* Set RAM off - EVG initialization will do this too. */
      EgSetSeqMode(evgCard_ps, ram, egMOD1_Off);
      EgSeqRamWrite(evgCard_ps, ram,
                    mpgEvent_as[EVENT_FIDUCIAL].ramPos_a[idx],
                    &mpgEvent_as[EVENT_FIDUCIAL].ram_as[idx]);
      EgSeqRamWrite(evgCard_ps, ram,
                    mpgEvent_as[EVENT_HEARTBEAT].ramPos_a[idx],
                    &mpgEvent_as[EVENT_HEARTBEAT].ram_as[idx]);
      EgSeqRamWrite(evgCard_ps, ram,
                    mpgEvent_as[EVENT_END_SEQUENCE].ramPos_a[idx],
                    &mpgEvent_as[EVENT_END_SEQUENCE].ram_as[idx]);
    }
#endif
  }
  return 0;
}

/*=============================================================================

  Name: mpgEventSeq

  Abs:  360Hz Sequence RAM Preparation

  Args: Type                Name        Access     Description
        ------------------- ----------- ---------- ----------------------------
        subRecord *         psub        read       point to subroutine record

  Rem:  Subroutine for IOC:<LOCA>:<UNIT>:TIMESLOTN-1

  Inputs:
       Pattern N-1 used on next fiducial:
       A - Pattern N-1 Modifier1 (mpg_ipling, mod720resync bits, beam code)
       B - Pattern N-1 Time Slot
       C - Pattern N-1 Modifier5 (for rate bits)
            
  Outputs:
       L - Event Enable/Disable
       VAL - N-1 time slot (1 to 6)
       EVG sequence ram updated.
       EVG sequence mode set to single sequence.
       Static data updated.
  
==============================================================================*/ 
static int mpgEventSeq(subRecord *psub)
{
  mpgEvent_ts     *mpgEvent_ps;
  epicsUInt32      modifier1     = psub->a;
  epicsUInt32      modifier5     = psub->c;
  epicsUInt32      modifier5mask = 1<<EDEF_MAX;
  epicsUInt32      lastTimestamp;
  int              ramPos;
  int              ram;
  int              ramUpdate;
  unsigned int     tidx;
  unsigned int     idx;

  eventEnable = 1;
  /* Lock the event data - this lock is kept throughout time slot
     and beam code event 360Hz processing and then released on the
     final record (sequence switch). */
  lockStatus = epicsMutexLock(mpgEventMutex_ps);
  if (lockStatus) {
    lockErrCount++;
    eventEnable = 0;
  }
  if (modifier1 & MPG_IPLING) {
    patternErrCount++;
    eventEnable = 0;
  }
  if ((modifier1 & MODULO720_MASK) || (mod36EventCode == EVENT_MODULO36_MAX)) {
    mod36EventCode = EVENT_MODULO36_MIN;
  } else {
    mod36EventCode++;
  }     
  /* Make sure the next ram is still in single mode and is inactive -
     after sequence control sends out a ram that's in single mode
     (which was done on the fiducial of the last pulse),
     it will automatically deactivate it.  */
  /* Find the current ram that has gone out with the fiducial */
  if (ramNext) ramNext = 0;
  else         ramNext = 1;
  ram = ramNext+1;
#ifdef __rtems__
  if (evgCard_ps) {
    int mode;

    /* Lock the EVG card - this lock is kept throughout time slot
       and beam code event 360Hz processing and then released on the
       final record (sequence switch). */
    evgLockStatus = epicsMutexLock(evgCard_ps->EgLock);
    if (evgLockStatus) {
      lockErrCount++;
      eventEnable = 0;
    }
    mode = EgGetMode(evgCard_ps, ram,
                     &mpgRam_as[ramNext].busy,
                     &mpgRam_as[ramNext].enable);
    /* For now, ignore the busy status. */
    if (mpgRam_as[ramNext].busy) {
      busyErrCount++;
    }
    /* Make sure we are in single-shot mode and disabled. */
    if (mode != egMOD1_Single) {
      EgSetSingleSeqMode(evgCard_ps, ram);
      mpgRam_as[ramNext].change = 1;
      mpgRam_as[ramNext].enable = 0;
      modeErrCount++;
    }
    /* If a RAM is still enabled, this means the fiducial didn't happen or
       the sequence ram is too long.  Disabling doesn't make a difference,
       we have to let it play out. */
    else if (mpgRam_as[ramNext].enable) {
      eventEnable = 0;
      enableErrCount++;
    }
  }
#endif
  /* Set up this sequence RAM only if all events are enabled. Note that
     the fiducial and heartbeat events are always position 0 and 1 so
     start at position 2 in the sequence ram.  The fiducial and
     heartbeat timestamps are always 0 and 1. */
  psub->l       = eventEnable;
  ramPos        = 2;
  lastTimestamp = 1;
  if (eventEnable) {
    ELLNODE *node_ps = ellFirst(&eventList_s);
    
    /* Go through each event and disable it if it's enabled unless it.
       goes every cycle and should be enabled.  If the ram needs an
       update due to user request (new event, different event code,
       different delay), rewrite the sequence ram. */
    while (node_ps) {
      mpgEvent_ps = (mpgEvent_ts *)node_ps;
      ramUpdate   = 0;
      if (mpgRam_as[ramNext].change) {
        mpgEvent_ps->ramPos_a[ramNext] = ramPos;
        ramUpdate = 1;
      }
      if (mpgEvent_ps->everyCycle && mpgEvent_ps->enable) {
        /* Modulo36 event code is actually 36 event codes that share the
           same delay.  Switch to the next event code now. */
        if (mpgEvent_ps->code == EVENT_MODULO36_MIN) {
          mpgEvent_ps->ram_as[ramNext].EventCode = mod36EventCode;
          ramUpdate = 1;
        } else if (mpgEvent_ps->ram_as[ramNext].EventCode != mpgEvent_ps->code) {
          mpgEvent_ps->ram_as[ramNext].EventCode = mpgEvent_ps->code;
          ramUpdate = 1;
        }
      } else if (mpgEvent_ps->ram_as[ramNext].EventCode) {
        mpgEvent_ps->ram_as[ramNext].EventCode = 0;
        ramUpdate = 1;
      }
#ifdef __rtems__
      if (ramUpdate && evgCard_ps)
        EgSeqRamWrite(evgCard_ps, ram, ramPos, &(mpgEvent_ps->ram_as[ramNext]));
#endif
      /* set next RAM position and save last delay (Timestamp) */
      ramPos++;
      lastTimestamp = mpgEvent_ps->ram_as[ramNext].Timestamp;
      /* get next record */
      node_ps = ellNext(node_ps);
    }
    mpgRam_as[ramNext].change = 0;
  }
  /* Always add end-of-sequence event - unless the ram is still enabled */
  if (!mpgRam_as[ramNext].enable) {
    /* When all events disabled - make sure ram is updated next time all
       events are enabled. */
    if (!eventEnable) mpgRam_as[ramNext].change = 1;
    /* Add end-of-sequence event if necessary */
    lastTimestamp++;
    mpgEvent_ps = &mpgEvent_as[EVENT_END_SEQUENCE];
    if ((mpgEvent_ps->ram_as[ramNext].Timestamp != lastTimestamp) ||
        (mpgEvent_ps->ramPos_a[ramNext]         != ramPos)) {
      mpgEvent_ps->ramPos_a[ramNext]         = ramPos;
      mpgEvent_ps->ram_as[ramNext].Timestamp = lastTimestamp;
#ifdef __rtems__
      if (evgCard_ps) EgSeqRamWrite(evgCard_ps, ram, ramPos,
                                    &(mpgEvent_ps->ram_as[ramNext]));
#endif
    }
  }
  /* Find time slot */
  psub->val = psub->b;
  tidx      = (unsigned int)psub->b - 1;
  /* Enable the event codes associated with this time slot */
  if ((tidx < TIMESLOT_MAX) && eventEnable) {
    for (idx = 0; idx < EVENTS_PER_TIMESLOT; idx++) {
      /* First 3 event codes are always sent.  The next
         8 event codes depend on the rate bits in modifier5. */
      if (idx < EVENTS_PER_TIMESLOT_ALWAYS) {
        ramUpdate = 1;
      } else {
        if (modifier5 & modifier5mask) ramUpdate = 1;
        else                           ramUpdate = 0;
        modifier5mask = modifier5mask<<1;
      }
      if (ramUpdate) {
        mpgEvent_ps = &mpgEvent_as[tsEventCode_aa[tidx][idx]];
        if (mpgEvent_ps->enable) {
          mpgEvent_ps->ram_as[ramNext].EventCode = mpgEvent_ps->code;
#ifdef __rtems__
          if (evgCard_ps) EgSeqRamWrite(evgCard_ps, ram,
                                        mpgEvent_ps->ramPos_a[ramNext],
                                        &(mpgEvent_ps->ram_as[ramNext]));
#endif
        }
      }
    }
  }  
  return 0;
}

/*=============================================================================

  Name: mpgEventBeamCode

  Abs:  360Hz Beam Code Event Codes

  Args: Type                Name        Access     Description
        ------------------- ----------- ---------- ----------------------------
        sSubRecord *        psub        read       point to subroutine record

  Rem:  Subroutine for IOC:<LOCA>:<UNIT>:LCLSBEAM, etc

  Inputs:
       A - Beam Code Base Event Code
       B - Beam Code Rate Event Code Flag
           (if zero, only set the base event code)
           (if non-zero, set the event codes for other rates
            when the time slot matches)
       C - Beam Code associated with this Event Code
       From Pattern N-1, N-2, N-3:
       D - Beam Code
       E - Modifier2
       F - Modifier3
       G - Modifier4
       H - Modifier5 (for rate bits)
       From the user:
       I - Modifier2 Inclusion Mask
       J - Modifier3 Inclusion Mask
       K - Modifier4 Inclusion Mask
       L - Modifier5 Inclusion Mask
       M - Modifier2 Exclusion Mask
       N - Modifier3 Exclusion Mask
       O - Modifier4 Exclusion Mask
       P - Modifier5 Exclusion Mask
            
  Outputs:
       Z - Pulse Counter
       VAL - Pattern Matches - one or more event codes are set
       EVG sequence ram updated.
  
==============================================================================*/ 
static int mpgEventBeamCode(sSubRecord *psub)
{
  mpgEvent_ts     *mpgEvent_ps;
  epicsUInt32      modifier5     = psub->h;
  epicsUInt32      modifier5mask = 1<<EDEF_MAX;
#ifdef __rtems__
  int              ram           = ramNext + 1;
#endif
  int              ramUpdate     = 1;
  int              eventcode     = psub->a;
  unsigned int     idx           = 0;

  if ((eventcode <= 0) || (eventcode >= MRF_NUM_EVENTS)) return -1;
  if (!eventEnable) return 0;
  psub->val = 0;  /* pattern doesn't match*/
  if (psub->c != psub->d) return 0;
  if ((((unsigned long)psub->e & (unsigned long)psub->i) ==
       (unsigned long)psub->i) &&
      (((unsigned long)psub->f & (unsigned long)psub->j) ==
       (unsigned long)psub->j) &&
      (((unsigned long)psub->g & (unsigned long)psub->k) ==
       (unsigned long)psub->k) &&
      (((unsigned long)psub->h & (unsigned long)psub->l) ==
       (unsigned long)psub->l)) {
    if ((((unsigned long)psub->e & (unsigned long)psub->m) == 0) &&
        (((unsigned long)psub->f & (unsigned long)psub->n) == 0) &&
        (((unsigned long)psub->g & (unsigned long)psub->o) == 0) &&
        (((unsigned long)psub->h & (unsigned long)psub->p) == 0)) {
      psub->val = 1;  /* pattern match */
      do {
        /* First event code is always always sent.  The reset
           depend on the rate bits in modifier5. */
        if (idx < EVENTS_PER_BEAMCODE_ALWAYS) {
          ramUpdate = 1;
        } else {
          if (modifier5 & modifier5mask) ramUpdate = 1;
          else                           ramUpdate = 0;
          modifier5mask = modifier5mask<<1;
        }
        if (ramUpdate) {
          mpgEvent_ps = &mpgEvent_as[eventcode];
          if (mpgEvent_ps->enable) {
            if (mpgEvent_ps->numPulses) {
              psub->z++;
              if (psub->z >= mpgEvent_ps->numPulses) mpgEvent_ps->enable = 0;
            } else {
              psub->z = 0;
            }  
            mpgEvent_ps->ram_as[ramNext].EventCode = mpgEvent_ps->code;
#ifdef __rtems__
            if (evgCard_ps) EgSeqRamWrite(evgCard_ps, ram,
                                          mpgEvent_ps->ramPos_a[ramNext],
                                          &(mpgEvent_ps->ram_as[ramNext]));
#endif
          } else {
            psub->z = 0;
          } 
        }
        if (psub->b == 0) idx = EVENTS_PER_BEAMCODE;
        else {
          idx++;
          eventcode++;
        }
      } while (idx < EVENTS_PER_BEAMCODE);
    }
  }  
  return 0;
}

/*=============================================================================

  Name: mpgEventSeqSwitch

  Abs:  360Hz Sequence RAM Switch

  Args: Type                Name        Access     Description
        ------------------- ----------- ---------- ----------------------------
        subRecord *         psub        read       point to subroutine record

  Rem:  Subroutine for IOC:<LOCA>:<UNIT>:SEQRAM

  Inputs:
       A - For testing - don't enable RAM if nonzero
            
  Outputs:
       VAL - Sequence Ram To Go on the next Fiducial
  
==============================================================================*/ 
static int mpgEventSeqSwitch(subRecord *psub)
{
  
#ifdef __rtems__
  if (evgCard_ps) {
    /* Enable the next RAM for the next fiducial */
    if ((!mpgRam_as[ramNext].enable) && (!psub->a)) {
      EgEnableRam(evgCard_ps, ramNext+1);
      mpgRam_as[ramNext].enable = 1;
    }
    if (!evgLockStatus) epicsMutexUnlock(evgCard_ps->EgLock);
  }
#endif
  /* Set to the ram that was just enabled */
  psub->val = ramNext;
  /* All events disabled until enabled by mpgEventSeq */
  eventEnable = 0;
  /* Unlock the static data so user requests can now be serviced. */
  if (!lockStatus) epicsMutexUnlock(mpgEventMutex_ps);
  
  return 0;
}

/*=============================================================================

  Name: mpgEventSeqSend

  Abs:  360Hz Sequence RAM Send (for test stand)

  Args: Type                Name        Access     Description
        ------------------- ----------- ---------- ----------------------------
        subRecord *         psub        read       point to subroutine record

  Rem:  Subroutine for IOC:<LOCA>:<UNIT>:SEQRAMSEND

  Inputs:
       A - For testing - send out a fiducial
            
  Outputs:
       VAL - Sequence Ram Sent
  
==============================================================================*/ 
static int mpgEventSeqSend(subRecord *psub)
{
#ifdef __rtems__
  if (evgCard_ps && psub->a) {
    /* For testing - when fiducial is not available, make it happen now. */
    epicsMutexLock(evgCard_ps->EgLock);
    if (!mpgRam_as[ramNext].enable) {
      EgEnableRam(evgCard_ps, ramNext+1);
      mpgRam_as[ramNext].enable = 1;
    }
    EgSeqTrigger(evgCard_ps, ramNext+1);
    epicsMutexUnlock(evgCard_ps->EgLock);
  }
#endif
  psub->val = ramNext;
  
  return 0;
}

/*=============================================================================

  Name: mpgEventCode

  Abs:  Process User Requested Changes to an Event in the Sequence RAM
        Not done at 360Hz!

  Args: Type                Name        Access     Description
        ------------------- ----------- ---------- ----------------------------
        subRecord *         psub        read       point to subroutine record

  Rem:  Subroutine for IOC:<LOCA>:<UNIT>:<NAME>EVNT

  Inputs:
       A - Event Code
       B - Desired Delay in the Sequence Ram (clock ticks)
       C - Minimum Delay (clock ticks)
       D - Maximum Delay (clock ticks)
       E - Enable/Disable this event code
       F - Event code applies to every cycle
       G - Sequence Ram Speed (Hz)
       H - Number of Pulses to Send - this event automatically disables
           after number is sent.  0 = always send
     
  Outputs:
       I - Desired Delay in the Sequence Ram (nsec)
       J - Actual Enable/Disable
       K - Actual Every Cycle Flag
       L - Actual Delay in the Sequence Ram (clock ticks)
       VAL = Actual Event Code

  Ret:  0 = OK, -1 = ERROR

==============================================================================*/
static long mpgEventCode(subRecord *psub)
{
  mpgEvent_ts *mpgEvent_ps;
  ELLNODE     *node_ps;
  epicsUInt32  newTimestamp = psub->b;
  epicsUInt32  prvTimestamp = psub->l;
  int          newcode = psub->a;
  int          prvcode = psub->val;

  /* Return with error if can't get mutex, outputs remain unchanged */ 
  if (epicsMutexLock(mpgEventMutex_ps)) return -1;
  
  /* Override invalid event code with the null event code.
     An event code is invalid if its already being used by another EVNT
     record or if the delay is out of bounds. */
  if ((newcode <= 0) || (newcode >= MRF_NUM_EVENTS) ||
      ((newcode != prvcode) && mpgEvent_as[newcode].code) ||
      (psub->b < psub->c) || (psub->b > psub->d)) {
    newcode      = 0;
    newTimestamp = 0;
  }
  /* Remove this event from the event list on any change to event code
     or delay. */
  if (mpgEvent_as[prvcode].code &&
      ((newcode != prvcode) || (newTimestamp != prvTimestamp))) {
    ellDelete(&eventList_s, &(mpgEvent_as[prvcode].node_s));
    mpgEvent_as[prvcode].code   = 0;
    mpgEvent_as[prvcode].enable = 0;
    mpgRam_as[0].change = mpgRam_as[1].change = 1;
  }
  /* Add this event to the event list in the proper order based on delay. */
  if (newcode && (!mpgEvent_as[newcode].code)) {
    int addEvent = 1;
    mpgEvent_as[newcode].ram_as[0].EventCode = 0;
    mpgEvent_as[newcode].ram_as[1].EventCode = 0;
    node_ps  = ellFirst(&eventList_s);
    while(node_ps) {
      mpgEvent_ps = (mpgEvent_ts *)node_ps;
      /* Insert it if it's a smaller delay than this one. */
      if (newTimestamp < mpgEvent_ps->ram_as[0].Timestamp) {
        ellInsert(&eventList_s, node_ps->previous,
                  &(mpgEvent_as[newcode].node_s));
        mpgRam_as[0].change = mpgRam_as[1].change = 1;
        node_ps  = 0;
        addEvent = 0;
      }
      /* Cannot use the same delay as another event.  Reject user request. */
      else if (newTimestamp == mpgEvent_ps->ram_as[0].Timestamp) {
        newcode      = 0;
        newTimestamp = 0;
        node_ps      = 0;
        addEvent     = 0;
      } else {
        node_ps = ellNext(node_ps);
      }
    }
    if (addEvent) {
      ellAdd(&eventList_s, &(mpgEvent_as[newcode].node_s));
      mpgRam_as[0].change = mpgRam_as[1].change = 1;
    }
  }
  if (newcode) {
    psub->j   = psub->e;
    psub->k   = psub->f;
    psub->l   = psub->b;
    psub->val = newcode;
  } else {
    psub->j = psub->k = psub->l = psub->val = 0;
  }
  /* Convert from clock ticks to nanoseconds */
  if (psub->g > 0) psub->i = psub->b/psub->g * 1E9;
  else             psub->i = 0;
  
  /* Now set up new attributes for this event - 360Hz processing will change
     RAM position and update event code */
  mpgEvent_as[newcode].code                = newcode;
  mpgEvent_as[newcode].enable              = psub->j;
  if (psub->h > 0) mpgEvent_as[newcode].numPulses = psub->h;
  else             mpgEvent_as[newcode].numPulses = 0;
  /* Burst mode will disable after sending pulses */
  if (mpgEvent_as[newcode].numPulses > 0) {
    psub->e = psub->j = 0;
  }
  mpgEvent_as[newcode].everyCycle          = psub->k;
  mpgEvent_as[newcode].ram_as[0].Timestamp = newTimestamp;  
  mpgEvent_as[newcode].ram_as[1].Timestamp = newTimestamp;
  
  epicsMutexUnlock(mpgEventMutex_ps);
    
  return 0;
}

/*=============================================================================

  Name: mpgEventCounts

  Abs:  Return diagnostic count values
        (for use by EPICS subroutine records)

  Args: Type     Name           Access     Description
        -------  -------        ---------- ----------------------------
  double *       lockErrCount_p    Write   # times unable to lock data
  double *       busyErrCount_p    Write   # times a seq ram was still busy
  double *       modeErrCount_p    Write   # times a seq ram was still busy
  double *       enableErrCount_p  Write   # times a seq ram was still active
  double *       patternErrCount_p Write   # bad patterns

  Rem:  The diagnostics count values are filled in.
  
  Side: None

  Return: 0 = OK
==============================================================================*/
int mpgEventCounts(double       *lockErrCount_p,
                   double       *busyErrCount_p,
                   double       *modeErrCount_p,
                   double       *enableErrCount_p,
                   double       *patternErrCount_p)
{
  *lockErrCount_p    = lockErrCount;
  *busyErrCount_p    = busyErrCount;
  *modeErrCount_p    = modeErrCount;
  *enableErrCount_p  = enableErrCount;
  *patternErrCount_p = patternErrCount;
  return 0;
}
/*=============================================================================

  Name: mpgEventCountReset

  Abs:  Reset diagnostic count values
        (for use by EPICS subroutine records)

  Args: None

  Rem:  The diagnostics count values are reset.
  
  Side: None

  Return: 0 = OK
==============================================================================*/
int mpgEventCountReset(void)
{
  lockErrCount    = 0;
  busyErrCount    = 0;
  modeErrCount    = 0;
  enableErrCount  = 0;
  patternErrCount = 0;
  return 0;
}
epicsRegisterFunction(mpgEventSeqInit);
epicsRegisterFunction(mpgEventSeq);
epicsRegisterFunction(mpgEventSeqSwitch);
epicsRegisterFunction(mpgEventSeqSend);
epicsRegisterFunction(mpgEventCode);
epicsRegisterFunction(mpgEventBeamCode);

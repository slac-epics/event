/*=============================================================================

  Name: mpgEvent.c
        mpgEventSeqInit   - MPG 360Hz Seq Ram Preparation Initialization
        mpgEventSeq       - MPG 360Hz Seq Ram Preparation
        mpgEventSeqSwitch - MPG 360Hz Seq Ram Switch
        mpgEventCode      - MPG Event Code Setup
	mpgEventBCSelect  - MPG 360Hz Beam Code Fanout Selection
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

#include "evrPattern.h"       /* Various bit masks           */
#include "egRecord.h"         /* egMOD1_Off, egMOD1_Single   */
#include "drvMrfEg.h"         /* MRF_NUM_EVENTS, Eg protos   */

#define EVENT_FIDUCIAL        1 /* Fiducial event number     */
#define EVENTS_PER_TIMESLOT  11 /* # events per timeslot     */

/* Detail for one event code */
typedef struct {
  ELLNODE         node_s;
  unsigned int    code;
  int             enable;
  int             everyCycle;
  unsigned short  ramPos_a[MRF_NUM_SEQ_RAM];
  MrfEvgSeqStruct ram_as[MRF_NUM_SEQ_RAM];
  
} mpgEvent_ts;

/* Detail for one sequence RAM */
typedef struct {
  int    change;
  int    enable;
  int    busy;  
} mpgRam_ts;

/* Detail for one time slot */
typedef struct {
  int         counter_a[EVENTS_PER_TIMESLOT];
  epicsUInt16 eventcode_a[EVENTS_PER_TIMESLOT];
} mpgTimeSlot_ts;

/* Tables and Lists */
static ELLLIST        eventList_s;       /* Event list in sequence RAM order */
static mpgEvent_ts    mpgEvent_as[MRF_NUM_EVENTS]; /* event code table       */
static mpgRam_ts      mpgRam_as[MRF_NUM_SEQ_RAM];  /* ram flags              */
static mpgTimeSlot_ts mpgTimeSlot_as[TIMESLOT_MAX]; /* time slot table       */
/* 120hz (other TS), 120hz, 60hz, 30hz, 10hz, 5hz, 1hz, 0.5hz, 3 spares      */
static int            tsCounterMax_a[EVENTS_PER_TIMESLOT] = 
    { 0, 0, 0, 1, 5, 11, 59, 119, 360, 360, 360 };

/* Control Flags */
static unsigned int   eventEnable      = 0; /* global event enable flag      */
static unsigned int   ramNext          = 0; /* next sequence RAM             */
static epicsMutexId   mpgEventMutex_ps = 0; /* Mutex for above information   */
static unsigned int   lockStatus       = 0; /* Mutex lock status             */
#ifdef __rtems__
static EgCardStruct  *evgCard_ps       = 0; /* EVG card information          */
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
  int idx;
  int eidx;
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
  /* Initialize event code table */
  memset(mpgEvent_as,    0, sizeof(mpgEvent_ts)    * MRF_NUM_EVENTS);
  /* Initialize time slot table */
  for (idx = 0, timeslot = TIMESLOT_MIN; idx < TIMESLOT_MAX; idx++, timeslot++) {
    /* First set up the event code for the other time slot */
    eventcode = timeslot + (TIMESLOT_MAX/2);
    if (eventcode > TIMESLOT_MAX) eventcode -= TIMESLOT_MAX;
    eventcode *= EVENTS_PER_TIMESLOT-1;
    eidx       = 0;
    mpgTimeSlot_as[idx].eventcode_a[eidx] = eventcode;
    mpgTimeSlot_as[idx].counter_a[eidx]   = 0;
    /* Now set up the event codes for this time slot. */
    for (eidx++, eventcode = timeslot * (EVENTS_PER_TIMESLOT-1);
         eidx < EVENTS_PER_TIMESLOT; eidx++, eventcode++) {
      mpgTimeSlot_as[idx].eventcode_a[eidx] = eventcode;
      mpgTimeSlot_as[idx].counter_a[eidx]   = 0;
    }
  }
  /* Start with all events disabled.  RAM 1 will be next. */
  eventEnable = 0; /* disabled until enabled by mpgEventSeqPrep */
  ramNext     = 0; /* next sequence ram to go */

  /* Initialize the start-of-sequence fiducial event  - this is NOT included
     in the linked list since it must ALWAYS be first */
  mpgEvent_as[EVENT_FIDUCIAL].code           = EVENT_FIDUCIAL;
  mpgEvent_as[EVENT_FIDUCIAL].enable         = 1;
  mpgEvent_as[EVENT_FIDUCIAL].everyCycle     = 1;
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
    mpgEvent_as[EVENT_END_SEQUENCE].ramPos_a[idx]         = 1;
    mpgEvent_as[EVENT_END_SEQUENCE].ram_as[idx].EventCode = EVENT_END_SEQUENCE;
    mpgEvent_as[EVENT_END_SEQUENCE].ram_as[idx].Timestamp = 1;
#ifdef __rtems__
    if (evgCard_ps) {
      /* Set RAM off - EVG initialization will do this too. */
      EgSetSeqMode(evgCard_ps, ram, egMOD1_Off);
      EgSeqRamWrite(evgCard_ps, ram,
                    mpgEvent_as[EVENT_FIDUCIAL].ramPos_a[idx],
                    &mpgEvent_as[EVENT_FIDUCIAL].ram_as[idx]);
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
       B - Pattern N-1 Modifier4 (time slot)
            
  Outputs:
       L - Event Enable/Disable
       VAL - N-1 time slot (1 to 6)
       EVG sequence ram updated.
       EVG sequence mode set to single sequence.
       Static data initialized.
  
==============================================================================*/ 
static int mpgEventSeq(subRecord *psub)
{
  mpgEvent_ts     *mpgEvent_ps;
  mpgTimeSlot_ts  *mpgTimeSlot_ps;
  epicsUInt32      modifier1  = psub->a;
  epicsUInt32      modifier4  = psub->b;
  epicsUInt32      lastTimestamp;
  int              ramPos;
  int              ram;
  int              idx;
  int              eidx;

  eventEnable = 1;
  /* Lock the event data - this lock is kept throughout time slot
     and beam code event 360Hz processing and then released on the
     final record (sequence switch). */
  lockStatus = epicsMutexTryLock(mpgEventMutex_ps);
  if (lockStatus) {
    lockErrCount++;
    eventEnable = 0;
  }
  if (modifier1 & MPG_IPLING) {
    patternErrCount++;
    eventEnable = 0;
  }
  /* Make sure the next ram is still in single mode and is inactive -
     after sequence control sends out a ram that's in single mode
     (which was done on the fiducial of the last pulse),
     it will automatically deactivate it.  */
  ram = ramNext + 1;
#ifdef __rtems__
  if (evgCard_ps) {
    int mode = EgGetMode(evgCard_ps, ram,
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
    else if (mpgRam_as[ram].enable) {
      eventEnable = 0;
      enableErrCount++;
    }
  }
#endif
  /* Set up this sequence RAM only if all events are enabled. Note that
     the fiducial event is always position 0 so start at position 1
     in the sequence ram.  The fiducial timestamp is always zero. */
  psub->l       = eventEnable;
  ramPos        = 1;
  lastTimestamp = 0;
  if (eventEnable) {
    int ramUpdate;
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
        if (mpgEvent_ps->ram_as[ramNext].EventCode != mpgEvent_ps->code) {
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
  /* Synch all time slot counters on MOD720 */
  if (modifier1 & MODULO720_MASK) {
    for (idx = 0; idx < TIMESLOT_MAX; idx++) {
      for (eidx = 0; eidx < EVENTS_PER_TIMESLOT; eidx++) {
        mpgTimeSlot_as[idx].counter_a[eidx] = tsCounterMax_a[eidx];
      }
    }
  }
  /* Find time slot */
  idx       = (modifier4 >> 29) & TIMESLOT_VAL_MASK;
  psub->val = idx;
  idx--;
  /* Enable the event codes associated with this time slot */
  if (idx >= 0) {
    mpgTimeSlot_ps = &mpgTimeSlot_as[idx];
    for (eidx = 0; eidx < EVENTS_PER_TIMESLOT; eidx++) {
      /* Are we ready to send this one?  Look at counter. */
      if (mpgTimeSlot_ps->counter_a[eidx] >= tsCounterMax_a[eidx]) {
        mpgTimeSlot_ps->counter_a[eidx] = 0;
        mpgEvent_ps = &mpgEvent_as[mpgTimeSlot_ps->eventcode_a[eidx]];
        if (eventEnable && mpgEvent_ps->enable) {
          mpgEvent_ps->ram_as[ramNext].EventCode = mpgEvent_ps->code;
#ifdef __rtems__
          if (evgCard_ps) EgSeqRamWrite(evgCard_ps, ram,
                                        mpgEvent_ps->ramPos_a[ramNext],
                                        &(mpgEvent_ps->ram_as[ramNext]));
#endif
        }
      /* Not ready to send. Increment counter. */
      } else {
        mpgTimeSlot_ps->counter_a[eidx]++;
      }
    }
  }  
  return 0;
}

/*=============================================================================

  Name: mpgEventBCSelect

  Abs:  360Hz Beam Code Fanout Selection

  Args: Type                Name        Access     Description
        ------------------- ----------- ---------- ----------------------------
        subRecord *         psub        read       point to subroutine record

  Rem:  Subroutine for IOC:<LOCA>:<UNIT>:BEAMCODEN-1

  Inputs:
       Beam codes supported in event code processing: 
       A - Beam Code a
       B - Beam Code b
       C - Beam Code c
       D - Beam Code d
       E - Beam Code e
       F - Beam Code f
       Pattern N-1 used on next fiducial:
       G - Pattern N-1 Beam Code
            
  Outputs:
       L - Beam code fanout selection
       VAL - N-1 Beam Code (0 to 31)
  
==============================================================================*/ 
static int mpgEventBCSelect(subRecord *psub)
{
  /* Find beam code fanout selection */
  if      (!eventEnable)       psub->l = 0;
  if      (psub->g == psub->a) psub->l = 1;
  else if (psub->g == psub->b) psub->l = 2;
  else if (psub->g == psub->c) psub->l = 3;
  else if (psub->g == psub->d) psub->l = 4;
  else if (psub->g == psub->e) psub->l = 5;
  else if (psub->g == psub->f) psub->l = 6;
  else                         psub->l = 0;
  psub->val = psub->g;
  
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
       A - For testing - send out a fiducial
            
  Outputs:
       VAL - Sequence Ram To Go on the next Fiducial
  
==============================================================================*/ 
static int mpgEventSeqSwitch(subRecord *psub)
{
  int ramCurr;
  
  /* Find the current ram that has gone out with the fiducial */
  if (ramNext) ramCurr = 0;
  else         ramCurr = 1;
#ifdef __rtems__
  if (evgCard_ps) {
    /* For testing - when fiducial is not available, make it happen now. */
    if (psub->a) {
      EgGetMode(evgCard_ps, ramCurr+1,
                &mpgRam_as[ramCurr].busy,
                &mpgRam_as[ramCurr].enable);
      if (mpgRam_as[ramCurr].enable)
        EgSeqTrigger(evgCard_ps, ramCurr+1);
    }
    /* Enable the next RAM for the next fiducial */
    if (!mpgRam_as[ramNext].enable)
      EgEnableRam(evgCard_ps, ramNext+1);
  }
#endif
  /* Set the next RAM for the next time through */
  psub->val   = ramNext;
  ramNext     = ramCurr;
  /* All events disabled until enabled by mpgEventSeq */
  eventEnable = 0;
  /* Unlock the static data so user requests can now be serviced. */
  if (!lockStatus) epicsMutexUnlock(mpgEventMutex_ps);
  
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
     
  Outputs:
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
  /* Now set up new attributes for this event - 360Hz processing will change
     RAM position and update event code */
  mpgEvent_as[newcode].code                = newcode;
  mpgEvent_as[newcode].enable              = psub->j;
  mpgEvent_as[newcode].everyCycle          = psub->k;
  mpgEvent_as[newcode].ram_as[0].Timestamp = newTimestamp;  
  mpgEvent_as[newcode].ram_as[1].Timestamp = newTimestamp;
  mpgEvent_as[newcode].ram_as[0].EventCode = 0;  
  mpgEvent_as[newcode].ram_as[1].EventCode = 0;
  
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
epicsRegisterFunction(mpgEventCode);
epicsRegisterFunction(mpgEventBCSelect);
/*epicsRegisterFunction(mpgEventBeamCode);*/

/*=============================================================================
 
  Name: evrPattern.c

  Abs: This file contains all subroutine support for evr Pattern processing
       records.
       
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

  Auth:  
  Rev:  
-----------------------------------------------------------------------------*/

#include "copyright_SLAC.h"	/* SLAC copyright comments */
 
/*-----------------------------------------------------------------------------
 
  Mod:  (newest to oldest)  
 
=============================================================================*/
#include "debugPrint.h"
#ifdef  DEBUG_PRINT
#include <stdio.h>
  int evrSubFlag = DP_DEBUG;
#endif


/* c includes */

#include <subRecord.h>        /* for struct subRecord      */
#include <sSubRecord.h>       /* for struct ssubRecord     */
#include <registryFunction.h> /* for epicsExport           */
#include <epicsExport.h>      /* for epicsRegisterFunction */
#include <dbAccess.h>         /* dbGetTimeStamp */
#include <alarm.h>            /* NO_ALARM, INVALID_ALARM */

#include "evrTime.h"
#include "evrPattern.h"       

#define  EVR_OK 0
#define  EVR_INVALID 1


/*=============================================================================

  Name: evrPatternInit

  Abs:  Initialization for 360Hz pattern processing.  Initialize pointer
        to waveform data.
		
  Args: Type	            Name        Access	   Description
        ------------------- -----------	---------- ----------------------------
        subRecord *         psub        read       point to subroutine record

  Rem:  Initialization subroutine for IOC:LOCA:UNIT:PATTERN

  Side: None.

  Sub Inputs/ Outputs:
   Inputs:
    A - PATTERNDATA.VAL
   Outputs:
    DPVT - pointer to waveform data

  Ret:  0 = OK, -1 = Error.
  
==============================================================================*/ 
static int evrPatternInit(subRecord *psub)
{
  DBADDR      *wfAddr = dbGetPdbAddrFromLink(&psub->inpa);

  /*
   * inpa must be a DB link and must be the proper type.
   */
  if (wfAddr && (wfAddr->field_type == DBF_LONG))
    psub->dpvt = (void *)wfAddr->pfield;
  else
    psub->dpvt = 0;
  if (psub->dpvt) return 0;
  else            return -1;
}
/*=============================================================================

  Name: evrPatternProc

  Abs:  360Hz Processing, Grab 7 pattern longs from PATTERNDATA waveform and
        parse into MODIFIER1-5 longins and two longin evr timestamps.

        Then parse out 2 longins from MODIFIER 1 for BEAMCODE, YY 
		PULSEID is contained in the lower 17 bits of timestamp nsec integer 
		
  Args: Type	            Name        Access	   Description
        ------------------- -----------	---------- ----------------------------
        subRecord *         psub        read       point to subroutine record

  Rem:  Subroutine for IOC:LOCA:UNIT:PATTERN

  Side: Waveform order:
        4 PNET 32 bit unsigned integers (MODIFIER 1-4)
		1 EVR 32 bit unsigned integer (MODIFIER 5)
		2 EVR timestamp 32 bit unsigned integers 
		1 bunchcharge 32 bit integer (picoCoulombs)
  Error checking:
Waveform record is invalid or has invalid size or type set record invalid, pulse ID to 0, 
and EVR timestamp/status to last-good-time/invalid
Error writing EVR timestamp set record invalid and pulse ID to 0

  Sub Inputs/ Outputs:
   Inputs:
    A - PATTERNDATA.VAL
    B - PATTERNDATA.SEVR
    C - PATTERNDATA.NORD
   Outputs:
	D - MODIFIER1N-3 (PNET)
	E - MODIFIER2N-3 (PNET)
	F - MODIFIER3N-3 (PNET)
	G - MODIFIER4N-3 (PNET)
	H - MODIFIER5N-3 (evr)
	I - BUNCHARGEN-3 (evr)
	J - BEAMCODEN-3 (decoded from PNET bits 8-12, MOD1 8-12)
	K - YYN-3       (decoded from PNET bits 0-7, MOD1 0-7)
	L - PULSEIDN-3  (decoded from PNET bits, 17)   
	VAL = 1 = error; 0 = OK
   Output to evr timestamp table
  Ret:  0

==============================================================================*/
static long evrPatternProc(subRecord *psub)
{ 

  DBADDR                *wfAddr = dbGetPdbAddrFromLink(&psub->inpa);
  evrPatternWaveform_ts *evrPatternWF_ps = (evrPatternWaveform_ts *)psub->dpvt;
  int                    status = 0;

  /* if waveform is invalid or has invalid size or type            */
  /*   set record invalid; pulse id =0, and                        */
  /*   evr timestamp/status to last good time with invalid status */
  if (psub->b ||
      (psub->c < (sizeof(evrPatternWaveform_ts)/sizeof(epicsUInt32))) ||
      (!psub->dpvt) || (!wfAddr)) {
    psub->d = psub->e = psub->f = psub->g = psub->h = psub->i = psub->j = psub->k = 0.0;
    status = -1;
  } else {

    dbScanLock(wfAddr->precord);
    /* error if the waveform has an invalid type or version */
    if ( (evrPatternWF_ps->header_s.type    != EVR_WF_PATTERN) ||
       (evrPatternWF_ps->header_s.version != EVR_WF_PATTERN_VERSION)) {
      psub->d = psub->e = psub->f = psub->g = psub->h = psub->i = psub->j = psub->k = 0.0;
      status = -1;
    } else {
      /* set outputs to the modifiers */
      psub->d = (double)(evrPatternWF_ps->pnet_s.modifier1);
      psub->e = (double)(evrPatternWF_ps->pnet_s.modifier2);
      psub->f = (double)(evrPatternWF_ps->pnet_s.modifier3);
      psub->g = (double)(evrPatternWF_ps->pnet_s.modifier4);
      psub->h = (double)(evrPatternWF_ps->modifier5);
      psub->i = (double)(evrPatternWF_ps->bunchcharge);
      /* beamcode decoded from modifier 1*/
      psub->j = (double)( (evrPatternWF_ps->pnet_s.modifier1 >> 8) &
                          BEAMCODE_BIT_MASK );
      /* yy decoded from modifier 1 bits */
      psub->k = (double)(evrPatternWF_ps->pnet_s.modifier1 & YY_BIT_MASK);
      /* decode pulseid field to output j (keep lower 17 bits) */
      psub->l = (double)(evrPatternWF_ps->time.nsec & LOWER_17_BIT_MASK);
      /* write to evr timestamp table and error check */
      if (evrTimePut (&evrPatternWF_ps->time, evrTimeOK)) {
	/* if error writing last timestamp table index - set record invalid */
	status = -1;
      }
    }
    dbScanUnlock(wfAddr->precord);
  }
  if (status) {
    psub->l   = PULSEID_INVALID;
    psub->val = EVR_INVALID;
    evrTimePut (0, evrTimeInvalid);
  } else {
    psub->val = EVR_OK;
  }
  return status;
 
}
/*=============================================================================

  Name: evrPatternSimInit

  Abs:  General purpose initialization required since all subroutine records
   require a non-NULL init routine even if no initialization is required.
   Note that most subroutines in this file use this routine as an init
   routine.  If init logic is needed for a specific subroutine, create a
   new routine for it - don't modify this one.
  
==============================================================================*/ 
static int evrPatternSimInit(sSubRecord *psub)
{

  return 0;
}
/*=============================================================================

  Name: evrPatternSimProc

  Abs:  360Hz Processing, Grab 7 pattern longs from PATTERNDATA waveform and
        parse into MODIFIER1-5 longins and two longin evr timestamps.

        Then parse out 2 longins from MODIFIER 1 for BEAMCODE, YY 
		PULSEID is contained in the lower 17 bits of timestamp nsec integer 
		
  Args: Type	            Name        Access	   Description
        ------------------- -----------	---------- ----------------------------
        subRecord *         psub        read       point to subroutine record

  Rem:  Subroutine for IOC:LOCA:UNIT:PATTERN

  Side: Waveform order:
        4 PNET 32 bit unsigned integers (MODIFIER 1-4)
		1 EVR 32 bit unsigned integer (MODIFIER 5)
		2 EVR timestamp 32 bit unsigned integers 
		1 bunchcharge 32 bit integer (picoCoulombs)
  Error checking:
Waveform record is invalid or has invalid size or type set record invalid, pulse ID to 0, 
and EVR timestamp/status to last-good-time/invalid
Error writing EVR timestamp â€“ set record invalid and pulse ID to 0

  Sub Inputs/ Outputs:
   Inputs:
    B - 1 = INVALID WF
        0 = VALID WF
 Now, for the simulated inputs (instead of the waveform:
    M - MODIFIER1
    N - MODIFIER2
    O - MODIFIER3
    P - MODIFIER4
    Q - MODIFIER5  
    U - BUNCHARGE  
    V - PULSEID
    W - YY
    X - BEAMCODE
   Outputs:
	D - MODIFIER1N-3 (PNET)
	E - MODIFIER2N-3 (PNET)
	F - MODIFIER3N-3 (PNET)
	G - MODIFIER4N-3 (PNET)
	H - MODIFIER5N-3 (evr)
	I - BUNCHARGEN-3 
	J - BEAMCODEN-3 (decoded from PNET bits 8-12, MOD1 8-12)
	K - YYN-3       (decoded from PNET bits 0-7, MOD1 0-7)
	L - PULSEIDN-3  (decoded from PNET bits, 17)
   
	VAL = 1 = error; 0 = OK
   Output to evr timestamp table
  Ret:  0

==============================================================================*/
static long evrPatternSimProc(sSubRecord *psub)
{ 
  epicsTimeStamp sys_time;

/*------------- parse input into sub outputs ----------------------------*/

  /* set outputs to the modifiers */
  psub->d = psub->m;
  psub->e = psub->n;
  psub->f = psub->o;
  psub->g = psub->p;
  psub->h = psub->q;
  /* bunch charge */
  psub->i = psub->u;
  /* grab beamcode, yy, from simulators */
  psub->j = psub->x;
  psub->k = psub->w;
  /* pulse id */
  psub->l = psub->v;
  /* form evr timestamp from system time and w pulseid   */
  if (epicsTimeGetCurrent (&sys_time)) {
	psub->l = 0.0;
	psub->val = EVR_INVALID;		
	DEBUGPRINT(DP_ERROR,evrSubFlag,("evrPatternSimProc exiting -1! \n"))
	return -1;
  }
  psub->r = (double) sys_time.secPastEpoch;
  sys_time.nsec &= UPPER_15_BIT_MASK;
  sys_time.nsec |= (unsigned long)(psub->v);
  psub->s = (double) sys_time.nsec;

  /* write to evr timestamp table and error check */
  if (evrTimePut (&sys_time, evrTimeOK)){
	/* if error writing last timestamp table index - set record invalid; pulse id =0*/
	psub->l = PULSEID_INVALID;
	psub->val = EVR_INVALID;		
	DEBUGPRINT(DP_ERROR,evrSubFlag,("evrPatternSimProc exiting from evrPatternPutTime! \n"));
	return -1;
  }
  return 0;
 
}

/*=============================================================================

  Name: evrPatternSimMod5

  Abs:  Sim Processing, Create Simulated Modifier5 from 20 simulated modmatch inputs

		
  Args: Type	            Name        Access	   Description
        ------------------- -----------	---------- ----------------------------
        subRecord *         psub        read       point to subroutine record

  Rem:  Subroutine for EVRSIM:X:1:MODIFIER5

  Side:

  Sub Inputs/ Outputs:
   Inputs:
    A-U - MODMATCH1-F1
    
   Outputs:   
    X  - lower 16 bits
    Y  - upper 16 bits
	VAL = MODIFIER5
  Ret:  0

==============================================================================*/
static long evrPatternSimMod5 (sSubRecord *psub)
{ 
  unsigned long mod5 = 0;

  mod5 |= (unsigned long)psub->a;
  mod5 |= ( ((unsigned long)psub->b) << 1 );
  mod5 |= ( ((unsigned long)psub->c) << 2 );
  mod5 |= ( ((unsigned long)psub->d) << 3 );
  mod5 |= ( ((unsigned long)psub->e) << 4 );
  mod5 |= ( ((unsigned long)psub->f) << 5 );
  mod5 |= ( ((unsigned long)psub->g) << 6 );
  mod5 |= ( ((unsigned long)psub->h) << 7 );
  mod5 |= ( ((unsigned long)psub->i) << 8 );
  mod5 |= ( ((unsigned long)psub->j) << 9 );
  mod5 |= ( ((unsigned long)psub->k) << 10 );
  mod5 |= ( ((unsigned long)psub->l) << 11 );
  mod5 |= ( ((unsigned long)psub->m) << 12 );
  mod5 |= ( ((unsigned long)psub->n) << 13 );
  mod5 |= ( ((unsigned long)psub->o) << 14 );
  mod5 |= ( ((unsigned long)psub->p) << 15 );
  mod5 |= ( ((unsigned long)psub->q) << 16 );
  mod5 |= ( ((unsigned long)psub->r) << 17 );
  mod5 |= ( ((unsigned long)psub->s) << 18 );
  mod5 |= ( ((unsigned long)psub->u) << 19 );

  psub->val = (double)mod5;
  /* split into 2 16 bit words */
  psub->x = (double) (mod5 & 0xFFFF);
  psub->y = (double) (mod5 >>16 );
  return 0;
 
}
epicsRegisterFunction(evrPatternInit);
epicsRegisterFunction(evrPatternProc);
epicsRegisterFunction(evrPatternSimInit);
epicsRegisterFunction(evrPatternSimProc);
epicsRegisterFunction(evrPatternSimMod5);

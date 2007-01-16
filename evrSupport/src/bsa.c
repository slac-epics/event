/*=============================================================================
 
  Name: bsa.c

  Abs: This file contains all subroutine support for the lcls beam synchronous
       acquisition and control subroutine records via EVG/EVR EDEFS
       
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

  ---------------------------------------------------------------------------*/

#include "copyright_SLAC.h"	/* SLAC copyright comments */
 
/*-----------------------------------------------------------------------------
 
  Mod:  (newest to oldest)  
 
=============================================================================*/
#include "debugPrint.h"
#ifdef  DEBUG_PRINT
#include <stdio.h>
  int bsaSubFlag = DP_ERROR;
#endif

/* c includes */
#include <string.h>        /* memcmp */
#include <math.h>          /* sqrt   */

#include <subRecord.h>        /* for struct subRecord      */
#include <sSubRecord.h>       /* for struct sSubRecord in site area */
#include <registryFunction.h> /* for epicsExport           */
#include <epicsExport.h>      /* for epicsRegisterFunction */
#include <epicsTime.h>        /* epicsTimeStamp */
#include <dbAccess.h>         /* dbGetTimeStamp */
#include <alarm.h>            /* NO_ALARM, INVALID_ALARM */
 
static int bsaInit(sSubRecord *psub)
{
  /*
   * General purpose initialization required since all subroutine records
   * require a non-NULL init routine even if no initialization is required.
    * Note that most subroutines in this file use this routine as an init
   * routine.  If init logic is needed for a specific subroutine, create a
   * new routine for it - don't modify this one.
   */
  return 0;
}
/*=============================================================================

  Name: bsaSecnAvg

  Abs:  Device Secondary Processing
        Computes running average and RMS values of Secondary
        for PRIM:LOCA:UNIT:$SECN$MDID.VAL 

		
  Args: Type	            Name        Access	   Description
        ------------------- -----------	---------- ----------------------------
        subRecord *        psub        read       point to subroutine record

  Rem:  Subroutine for PRIM:LOCA:UNIT:MEASCNT$MDID

  Side: INPA - PRIM:LOCA:UNIT:SECN.VAL
        INPB - EDEF:S:$(MD):AVGCNTMAX CA
        SDIS - EVR:IOC:1:MODIFIER5 BIT for this MDID 1=  match
		INPD - PRIM:LOCA:UNIT:STATUS   (EPICS STAT for device)
        INPE - PRIM:LOCA:UNIT:STATUS.L (EPICS SEVR for device)
        INPF - EVR:LOCA:UNIT:PULSEID
        INPG - 
		OUT 
           H = Variance value used in RMS calc
           I = B - 1
           J = B - 2
           K = A - previous VAL
           L = RMS value when average is done

		   M = goodmeas count
           O - reset counters, values (avcnt, goodmeas, stat, timestamp check)
               Reset occurs from reset sequence
		   P - EPICS stat for average
           S - EPICS sevr for average
	       Q = avgcnt
		   W = Timestamp mismatch
		   X = history buff enable(1)/disable (0)
		   Y = count (total pulses counted so far)
		   Z = outer loop count
	   VAL = Running average 
      
  Ret:  0

==============================================================================*/
static long bsaSecnAvg(sSubRecord *psub)
{
/* linux uses system time; rtems general time */
#ifndef linux
  epicsTimeStamp  timeEVR; 
  epicsTimeStamp  timeSecn;  /* for comparison */
#endif

  psub->x = 0;    /* default to history buff disable */

#ifndef linux

  /*EVR timestamp:*/
  if (dbGetTimeStamp(&psub->inpf, &timeEVR)){
        DEBUGPRINT(DP_ERROR, bsaSubFlag, ("bsaSecnAvg for %s: Unable to determine EVR timestamp.\n",psub->name));
        psub->w++;
        return -1;
  }
  /*Secondary timestamp:*/
  if (dbGetTimeStamp(&psub->inpa, &timeSecn)){
        DEBUGPRINT(DP_ERROR, bsaSubFlag, ("bsaSecnAvg for %s: Unable to determine device timestamp.\n",psub->name));
        psub->w++;
        return -1;
  }
  /* if timestamps are different, then processing is not synched */
  if (memcmp(&timeEVR,&timeSecn,sizeof(epicsTimeStamp))) {
	DEBUGPRINT(DP_ERROR, bsaSubFlag, ("basSecnAvg for %s: Timestamp MISMATCH.\n",psub->name));
	psub->w++;
	return -1;
  }
#endif
  /*  if (psub->n < 2) psub->n++; *//* increment first-time flag */

  /* Reinit for a new average */
  if (psub->o) {
    psub->o = 0;
	psub->z = 0; /* outer loop count reset */
    /*  reset avgcnt, meascnt, goodmeas, stat   */
    psub->val = 0;
	psub->y = 0; /* total count */
	psub->q = 0; /* avgcount    */
    psub->m = 0; /* goodmeas    */
    psub->w = 0; /* ts mismatch */
    psub->p = psub->d;
    psub->s = psub->e;
    /* cum avg stuff :*/
	psub->val = psub->a;
	psub->h = 0;
	psub->i = 0;
	psub->j = 0;
	psub->k = 0;
	psub->l = 0;
	DEBUGPRINT(DP_DEBUG, bsaSubFlag, ("bsaSecnAvg: First time thru for %s; psub->b=%ld\n", psub->name,(unsigned long) psub->b) );
     /*DEBUGPRINT(DP_DEBUG, bsaSubFlag, ("basAvgCount for %s: Reset counters; psub->i = %ld\n",psub->name, (unsigned long)psub->i));*/
  }
  /* No averaging requested? */
  if (psub->b <= 0) {
    psub->o = 1;
  } else {
    if (psub->e < INVALID_ALARM) {
      psub->m++; /* inc goodmeas */
      /* EPICS - check for worst severity */
      if (psub->s < psub->e) {
        psub->s = psub->e;
        psub->p = psub->d;
      }
    }
    /* always incr avgcnt*/
    psub->q++;
    /* if val >= EDEF AVGCNTMAX */
    if (psub->q >= psub->b) {
      /*  then averaging is done, and enable history buffer */
      psub->x = 1;  /* enable history buff */
	  psub->q = 0;  /* reset avgcnt */
	  psub->o = 1;  /* reset counters next loop */
	  psub->z++;    /* inc outer loop count */
    } else {
      /*  else averaging is NOT done */
 	  psub->o = 0;
    }
  }  
  /* now start the averaging */
  /* first time thru for new cycle; reset previous avg, variance */
  
  /* compute running avg and variance                      */
  /*        This is translated from REF_RMX_BPM:CUM.F86.   */
  /*                                                       */
  /*        CUM computes VAR as the sample variance:       */ 
  /*          VAR = (SUMSQ - SUM*SUM/N)/(N-1)              */
  /*          where SUM = sum of the N values, and         */
  /*           SUMSQ = sum of the squares of the N values. */
  /*                                                       */
  /*        Note that CUM's method of computing VAR avoids */
  /*        possible loss of significance.                 */
  psub->i = psub->m-1.0;
  psub->j = psub->m-2.0;
  psub->k = psub->a-psub->val;
  psub->val += psub->k/psub->m;
  psub->k /= psub->i;
  psub->h = (psub->j*(psub->h/psub->i)) + (psub->m*psub->k*psub->k);
  DEBUGPRINT(DP_DEBUG, bsaSubFlag, ("bsaSecnAvg for %s: Avg: %f; Var: %f \n", psub->name, psub->val, psub->h) );
  if (psub->o) {
	psub->l = psub->h/psub->m;
	psub->l = sqrt(psub->l);
  }
  psub->y++;  /* increment count */
  return 0;
}

/*=============================================================================

  Name: bsaStatus

  Abs:  Device EPICS Severity and Status, SLC Status - 120Hz Processing

		
  Args: Type	            Name        Access	   Description
        ------------------- -----------	---------- ----------------------------
        subRecord *        psub        read       point to subroutine record

  Rem:  Subroutine for PRIM:LOCA:UNIT:STATUS

  Side: INPA - PRIM:LOCA:UNIT:SECN1.STAT
        INPB - PRIM:LOCA:UNIT:SECN2.STAT 
        INPC - PRIM:LOCA:UNIT:SECN3.STAT 
        INPD - PRIM:LOCA:UNIT:SECN1.SEVR
        INPE - PRIM:LOCA:UNIT:SECN2.SEVR 
        INPF - PRIM:LOCA:UNIT:SECN3.SEVR 
        INPG - PRIM:LOCA:UNIT:CTRL
        INPH - PRIM:LOCA:UNIT:CALBSTATUS.SEVR
		OUT
	   K - SLC STAT
	   L - EPICS severity
	   VAL = EPICS STAT
      
  Ret:  0

==============================================================================*/
static long bsaStatus(subRecord *psub)
{
  psub->val = psub->a;
  psub->l   = psub->d;
  if (psub->e > psub->l) {
    psub->val = psub->b;
    psub->l   = psub->e;
  }
  if (psub->f > psub->l) {
    psub->val = psub->c;
    psub->l   = psub->f;
  }
  /* assume lcls will have the equiv - keep below to remind */
#ifdef SLCAWARE  
  if      (psub->g == 0) /* offline? */
    psub->k = STAT_DEAD;
  else if (psub->g == 1) /* maintenance? */
    psub->k = STAT_MAINTENANCE;
  else if (psub->g == 2) /* checkout? */
    psub->k = STAT_CHECKOUT;
  else if (psub->h>NO_ALARM) /* calibration bad or in-progress */
    psub->k = STAT_BADCAL;
  else if (psub->l == NO_ALARM)
    psub->k = STAT_GOOD;
  else if (psub->l <= MAJOR_ALARM)
    psub->k = STAT_OK;
  else
    psub->k = STAT_SICK;
#endif
  return 0;
}

/*=============================================================================
  
  Name: bsaSimCheckTest
  Simulater for EVG - simulates only 1 hertz and 10 hertz right now
  
  Abs:  360Hz Processing
  Check to see if current beam pulse is to be used in any
  current measurement definition.



  Args: Type	            Name        Access	   Description
        ------------------- -----------	---------- ----------------------------
        sSubRecord *        psub        read       point to sSub subroutine record

  Rem:   Subroutine for EVR:$IOC:1:CHECKEVR$MDID

  side: 
        INPA - EDEF:LCLS:$(MD):CTRL  1= active; 0 = inactive
		
		INPB - ESIM:$(IOC):1:MEASCNT$(MDID)
		INPC - EDEF:SYS0:$(MD):CNTMAX
        INPD - ESIM:SYS0:1:DONE$(MDID)

        INPF - ESIM:$(IOC):1:MODIFIER4
		INPG - INCLUSION2
        INPH - 

for testing, match on Inclusion bits only;
override modifier 4 if one hertz bit is set
        INPP - EDEF:$(IOC):$(MD):INCLUSION4

        INPU - INCLUSION2   ONE HERTZ BIT from Masksetup		
        INPV - INCLUSION3   TEN HERTZ BIT from masksetup
		  Note: above masksetup bits override MODIFIER4 input!
        INPW - ESIM:$(IOC):1:BEAMCODE.SEVR
        INPX - EVR:$(IOC):1:CNT$(MDID).Q
                OUT
	    Q - full measurement count complete - flags DONE
	    VAL = EVR pattern match = 1
		 no match = 0; enable/disable for bsaSimCount

        All three outputs are initialized to 0 before first-time processing
        when MDEF CTRL changes to ON.

  Ret:  none

==============================================================================*/
static long bsaSimCheckTest(sSubRecord *psub)
{
  psub->val = 0;
  psub->q = 0;

  if (psub->w>MAJOR_ALARM) {
    /* bad data - do nothing this pulse and return bad status */
    return(-1);
  }
  /* if this edef is not active, exit */
  if (!psub->a) return 0;

  /* now check this pulse */
  /* check inclusion mask */
  if ( (unsigned long)psub->u ) psub->p = 10; /* force one hertz processing */
  /* set modifier 4 to 10; assuming evg sim counts 1 to 10 */
  if ( (unsigned long)psub->v ) psub->p = 0 ; /* force 10  hertz processing */	 
  
  if (  (((unsigned long)psub->f & (unsigned long)psub->p)==(unsigned long)psub->p)
		/*		&&(((unsigned long)psub->g & (unsigned long)psub->q)==(unsigned long)psub->q)*/) 
	{
		DEBUGPRINT(DP_DEBUG, bsaSubFlag, 
				   ("bsaSimCheckTest: inclusion match for %s; INCL4=0x%lx, MOD4=0x%lx\n",
					psub->name, (unsigned long)psub->p, (unsigned long)psub->f ));	  
		psub->val = 1;
	
	}
	else DEBUGPRINT(DP_DEBUG, bsaSubFlag, 
					("bsaSimCheckTest: inclusion NO match for %s; INCL4=0x%lx, MOD4=0x%lx\n",
					 psub->name, (unsigned long)psub->p, (unsigned long)psub->f ));

  /* check for end of measurement */
  /* if SYS EDEF,EDEF:SYS0:MD:CNTMAX = -1, and this will go forever */ 
  if ( (psub->b==psub->c) && (!psub->d) ) { /* we're done - */
	psub->val = 0;       /* clear modmatch flag */
	psub->q = 1;         /* flag to DONE to disable downstream */
	return 0;
  }
return 0;
}

epicsRegisterFunction(bsaInit);
epicsRegisterFunction(bsaSecnAvg);
epicsRegisterFunction(bsaStatus);
epicsRegisterFunction(bsaSimCheckTest);

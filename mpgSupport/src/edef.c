/*=============================================================================

  Name: edef.c
        edefInit            - General Initialization
	edefPatternCheck    - Check pattern against edef for bsa enable/disable
	edefEnableMask      - Assemble EDEF enable mask

  Abs: This file contains subroutine support for the MPG IOC processing
       of the incoming Pnet pattern against event definitions for possible match.

  Rem: All functions called by the subroutine record get passed one argument:

         psub                       Pointer to the subroutine record data.
          Use:  pointer
          Type: struct sSubRecord *
          Acc:  read/write
          Mech: reference

         All functions return a long integer.  0 = OK, -1 = ERROR.
         The subroutine record ignores the status returned by the Init
         routines.  For the calculation routines, the record status (STAT) is
         set to SOFT_ALARM (unless it is already set to LINK_ALARM due to
         severity maximization) and the severity (SEVR) is set to psub->brsv
         (BRSV - set by the user in the database though it is expected to
          be invalid).

  Auth: 18-dec-2006, drogind, some logic taken from src/micro/epicsbsac/src/bsac.c
  Rev:  

-----------------------------------------------------------------------------*/

#include "copyright_SLAC.h"     /* SLAC copyright comments */

/*-----------------------------------------------------------------------------

  Mod:  (newest to oldest)

=============================================================================*/
#include "debugPrint.h"
#include "sSubRecord.h"       /* for struct sSubRecord       */
#include "registryFunction.h" /* for epicsExport            */
#include "epicsExport.h"      /* for epicsRegisterFunction  */
#include "epicsTime.h"        /* for epicsTimeGetCurrent    */
#include "dbAccess.h"         /* DBADDR typedef and protos  */
#include "alarm.h"            /* NO_ALARM, INVALID_ALARM */

#ifdef DEBUG_PRINT
  /* ENUM with values: DP_FATAL, DP_ERROR, DP_WARN, DP_INFO, DP_DEBUG
     Anything less than or equal to mpgPatternFlag gets printed out. Can
     be set to 0 here and altered via prompt or in st.cmd file */
int edefSubFlag = 0;
#endif

/*=============================================================================

  Name: edefInit

  Abs:  General purpose initialization required since all subroutine records
   require a non-NULL init routine even if no initialization is required.
   Note that most subroutines in this file use this routine as an init
   routine.  If init logic is needed for a specific subroutine, create a
   new routine for it - don't modify this one.
  
==============================================================================*/ 
static int edefInit(sSubRecord *psub)
{
  return 0;
}
/*=============================================================================

  Name: edefPatternCheck

  Abs:  360Hz Processing
        Check to see if current beam pulse is to be used in any
		current event measurement definition.
		Keep running measurement count.
        Keep done flag.
		Keep pattern match flag (val).
		

  Args: Type	            Name        Access	   Description
        ------------------- -----------	---------- ----------------------------
        sSubRecord *        psub        read       point to sSub subroutine record

  Rem:   Subroutine for PATTERNCHECK
         Enabled when the EDEF CTRL is ON
  Inputs
        INPA - Spare
        INPB - Beam Code
        INPC - Reset 
        INPD - Pnet Modifier 1
        INPE - Pnet Modifier 2
        INPF - Pnet Modifier 3 
        INPG - Pnet Modifier 4 
        INPH - Spare for additional modifier bits        
        INPI - EDEF Beam Code
        INPJ - Spare (ie for future beam codes)
        INPK - Spare (ie for future beam codes)
        INPL - Spare (ie for future beam codes)
        INPM - EDEF INCLUSION1
        INPN - EDEF INCLUSION2
        INPO - EDEF INCLUSION3
        INPP - EDEF INCLUSION4
        INPQ - EDEF INCLUSION5 (not coded yet)
        INPR - EDEF EXCLUSION1
        INPS - EDEF EXCLUSION2
        INPT - EDEF EXCLUSION3
        INPU - EDEF EXCLUSION4
        INPV - EDEF EXCLUSION5 (not coded yet)
        INPW - Beam Code SEVR
        INPX - EDEF CNTMAX (avg cnt * meas cnt)
		
  Outputs
	   VAL = Pattern match = 1
     	 no match = 0; enable/disable for bsacMeasCount
           Y   = Measurement Count
	   Z   = Done =1; not done measuring = 0
  Ret:  none

==============================================================================*/
static long edefPatternCheck(sSubRecord *psub)
{
  psub->val = 0;

  if (psub->w>MAJOR_ALARM) {
    /* bad data - do nothing with this pulse and return bad status */
    return(-1);
  }
  /* are we done? - if so exit now */
  /* Note: the sequence disables this record upon DONE, so this shouldn't happen */
  if (psub->z) return 0;  

  /* reset? */
  if (psub->c) {
	psub->z=0;   /* reset done          */
    psub->val=0; /* reset pattern match */
    psub->y=0;   /* reset meas. count   */
	psub->c=0;   /* reset the reset signal */
  }

  /* if Pattern beam code == EDEF beamcode.. */
  if (psub->b==psub->i) {
	/* check inclusion mask */
	DEBUGPRINT(DP_DEBUG, edefSubFlag, ("\t edefPatternCheck: Beam Codes match \n"));	  
	if (    (((unsigned long )psub->d & (unsigned long)psub->m)
			 == (unsigned long)psub->m) 
			&&(((unsigned long)psub->e & (unsigned long)psub->n)== 
			   (unsigned long)psub->n)
		    &&(((unsigned long)psub->f & (unsigned long)psub->o)== 
			   (unsigned long)psub->o)
			&&(((unsigned long)psub->g & (unsigned long)psub->p)== 
			   (unsigned long)psub->p)) {
	  DEBUGPRINT(DP_DEBUG, edefSubFlag, ("edefPatternCheck: inclusion match\n"));	  
	  if (    (((unsigned long)psub->r & (unsigned long)psub->d)== 0) 
			  &&(((unsigned long)psub->s & (unsigned long)psub->e)== 0)
			  &&(((unsigned long)psub->t & (unsigned long)psub->f)==0)
			  &&(((unsigned long)psub->u & (unsigned long)psub->g)== 0)) {
		DEBUGPRINT(DP_DEBUG, edefSubFlag, ("edefPatternCheck: exclusion match\n"));

		psub->val = 1;  /* pattern match*/
		psub->y++;      /* inc. meas. count */
        /* Allow for X = -1 = forever */
		if ((psub->y >= psub->x) && (psub->x > 0)) {
		  /* we're done with measurement */
		  psub->z=1;
		}

	  }
	}
	
  }  	
  return 0;
}

/*=============================================================================

  Name: edefEnableMask

  Abs:  Create EDEF enable mask from the pattern check records.

		
  Args: Type	            Name        Access	   Description
        ------------------- -----------	---------- ----------------------------
        sSubRecord *         psub        read       point to subroutine record

  Rem:  Subroutine for IOC:LOCA:UNIT:EDEFMASK

  Side:

  Sub Inputs/ Outputs:
   Inputs:
    A-U - Pattern Check Results (for 20 EDEFs)
    
   Outputs:   
    X  - lower 16 bits
    Y  - upper 16 bits
    VAL = Enable Mask
  Ret:  0

==============================================================================*/
static long edefEnableMask (sSubRecord *psub)
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

/*=============================================================================

  Name: edefPatternCheck

  Abs:  360Hz Processing
        Check to see if current beam pulse is to be used in any
		current event measurement definition.
		Keep running measurement count.
        Keep done flag.
		Keep pattern match flag (val).
		

  Args: Type	            Name        Access	   Description
        ------------------- -----------	---------- ----------------------------
        sSubRecord *        psub        read       point to sSub subroutine record

  Rem:   Subroutine for PATTERNCHECK
         Enabled when the EDEF CTRL is ON
  Inputs
        INPA - Spare
        INPB - Beam Code
        INPC - Reset 
        INPD - Pnet Modifier 1
        INPE - Pnet Modifier 2
        INPF - Pnet Modifier 3 
        INPG - Pnet Modifier 4 
        INPH - Spare for additional modifier bits        
        INPI - EDEF Beam Code
        INPJ - Spare (ie for future beam codes)
        INPK - Spare (ie for future beam codes)
        INPL - Spare (ie for future beam codes)
        INPM - EDEF INCLUSION1
        INPN - EDEF INCLUSION2
        INPO - EDEF INCLUSION3
        INPP - EDEF INCLUSION4
        INPQ - EDEF INCLUSION5 (not coded yet)
        INPR - EDEF EXCLUSION1
        INPS - EDEF EXCLUSION2
        INPT - EDEF EXCLUSION3
        INPU - EDEF EXCLUSION4
        INPV - EDEF EXCLUSION5 (not coded yet)
        INPW - Beam Code SEVR
        INPX - EDEF CNTMAX (avg cnt * meas cnt)
		
  Outputs
	   VAL = Pattern match = 1
     	 no match = 0; enable/disable for bsacMeasCount
           Y   = Measurement Count
	   Z   = Done =1; not done measuring = 0
  Ret:  none

==============================================================================*/
static long edefPatternCheckSim(sSubRecord *psub)
{
  psub->val = 0;

  /* are we done? - if so exit now */
  /* Note: the sequence disables this record upon DONE, so this shouldn't happen */
  if (psub->z) return 0;  

  /* reset? */
  if (psub->c) {
	psub->z=0;   /* reset done          */
    psub->val=0; /* reset pattern match */
    psub->y=0;   /* reset meas. count   */
	psub->c=0;   /* reset the reset signal */
  }

  DEBUGPRINT(DP_DEBUG, edefSubFlag, ("\t edefPatternCheckSim:  match \n"));	  


  psub->val = 1;  /* pattern match*/
  psub->y++;      /* inc. meas. count */
  if ((psub->y >= psub->x) && (psub->x > 0)) {
	/* we're done with measurement */
	psub->z=1;
  }
 

  return 0;
}

epicsRegisterFunction(edefInit);
epicsRegisterFunction(edefPatternCheck);
epicsRegisterFunction(edefEnableMask);
epicsRegisterFunction(edefPatternCheckSim);

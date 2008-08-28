/*=============================================================================
 
  Name: evrModifier5.c - Pattern Bit Manipulation Routines
        evrModifer5     - Modifier 5 Creation using EDEF Check Bits
        evrModifer5Bits - Get EDEF Check Bits out of Modifier 5
	mpgEdefMeasSevrMasks- Encodes 20 MEASSEVRS into 2 (minor and major) masks
	evrEdefMeasSevr     - Decodes edefMinorMask and edefMajorMask into 20 meassevrs
	mpgEdefInitMask     - Encodes 20 EDEFINITs into mask
	evrEdefInitEvent    - Post Init Events for 20 EDEFs

  Abs: This file contains all subroutine support for evr Pattern processing
       records.
       
  Rem: All functions called by the subroutine record get passed one argument:

         psub                       Pointer to the subroutine record data.
          Use:  pointer
          Type: struct longSubRecord *
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

/* c includes */

#include "longSubRecord.h"    /* for struct longSubRecord  */
#include "alarm.h"            /* for *_ALARM defines       */
#include "dbScan.h"           /* for post_event            */
#include "registryFunction.h" /* for epicsExport           */
#include "epicsExport.h"      /* for epicsRegisterFunction */
#include "evrPattern.h"       /* EDEF_MAX, MOD5_NOEDEF_MASK*/

/*=============================================================================

  Name: evrModifier5

  Abs:  Create modifier 5 by adding EDEF enable bits from the pattern
        check records into the rest of Modifer5.

		
  Args: Type	            Name        Access	   Description
        ------------------- -----------	---------- ----------------------------
        longSubRecord *     psub        read       point to subroutine record

  Rem:  Subroutine for IOC:LOCA:UNIT:MODIFIER5N-3
                   and IOC:LOCA:UNIT:EDAVGDONEN-3 on the MPG 

  Side:

  Sub Inputs/ Outputs:
   Inputs:
    A-T - Pattern Check Results (for 20 EDEFs)
    U   - Modifier5 without EDEF bits
          (or zero when processing for EDAVGDONE)
   Outputs:   
    VAL = Modifier5
  Ret:  0

==============================================================================*/
static long evrModifier5(longSubRecord *psub)
{ 
  unsigned long *check_p = &psub->a;
  unsigned long  value;
  int            edefIdx;

  value = psub->u & MOD5_NOEDEF_MASK;
  for (edefIdx = 0; edefIdx < EDEF_MAX; edefIdx++, check_p++) {
    value |= ((*check_p) << edefIdx);
  }
  psub->val = value;
  return 0;
}

/*=============================================================================

  Name: evrModifier5Bits

  Abs:  Get EDEF Check Bits out of Modifier 5
		
  Args: Type	            Name        Access	   Description
        ------------------- -----------	---------- ----------------------------
        longSubRecord *     psub        read       point to subroutine record

  Rem:  Subroutine for IOC:LOCA:UNIT:MODIFIER5

  Side:

  Sub Inputs/ Outputs:
   Inputs:
    V = Modifier5
    
   Outputs:   
    A-T - Pattern Check Results (for 20 EDEFs)
    U   - Modifier5 without EDEF bits
    VAL = Modifier5
  Ret:  0

==============================================================================*/
static long evrModifier5Bits(longSubRecord *psub)
{ 
  unsigned long *check_p = &psub->a;
  int            edefIdx;
  
  psub->val = psub->v;
  psub->u   = psub->v & MOD5_NOEDEF_MASK;
  for (edefIdx = 0; edefIdx < EDEF_MAX; edefIdx++, check_p++) {
    if (psub->v & (1 << edefIdx)) *check_p = 1.0;
    else                          *check_p = 0.0;
  }
  return 0;
}

/*=============================================================================

  Name: mpgEdefMeasSevrMasks

  Abs:  Create meas sevr masks by adding the 20 edef meas sevr settings
        into appropriate masks. These masks are part of the outgoing pattern.

		
  Args: Type	            Name        Access	   Description
        ------------------- -----------	---------- ----------------------------
        longSubRecord *     psub        read       point to subroutine record

  Rem:  Subroutine for MPG IOC:LOCA:UNIT:EDEFMEASSEVR 

  Side:

  Sub Inputs/ Outputs:
   Inputs:
    A-T - Edef Meas Sevr setting (for 20 EDEFs)
    U   - spare
   Outputs:   
    V   - edefMinorMask - bit set if MEASSEVR is set to MINOR
    W   - edefMajorMask - bit set if MEASSEVR is set to either MAJOR or MINOR
    
  Ret:  0

==============================================================================*/
static long mpgEdefMeasSevrMasks(longSubRecord *psub)
{ 
  unsigned long *check_p = &psub->a;
  int            edefIdx;

  psub->v = 0;
  psub->w = 0;
  /* for minormask, a bit is set if MEASSEVR is set to MINOR */
  /* for majormask, a bit is set if MEASSEVR is set to either MINOR or MAJOR */
  for (edefIdx = 0; edefIdx < EDEF_MAX; edefIdx++, check_p++) {
    if ((*check_p) == MINOR_ALARM) {
      psub->v |= 1 << edefIdx;
      psub->w |= 1 << edefIdx;
    } else if ((*check_p) == MAJOR_ALARM) {
      psub->w |= 1 << edefIdx;
    }
  }
  return 0;
}

/*=============================================================================

  Name: evrEdefMeasSevr

  Abs:  Create meas sevr value for each edef, then place into 20 edef meas sevr bits
 
		
  Args: Type	            Name        Access	   Description
        ------------------- -----------	---------- ----------------------------
        longSubRecord *     psub        read       point to subroutine record

  Rem:  Decodes edefMinorMask and edefMajorMask into 20 bits with value of either

  Side: Subroutine for EVR IOC:LOCA:UNIT:EDEFMEASSEVR 
 
 Sub Inputs/ Outputs:
   Inputs:
    U = spare
    V = edefMinorMask
    W = edefMajorMask
    
   Outputs:   
    A-T - MeasSevr Results (for 20 EDEFs)
  Ret:  0

==============================================================================*/
static long evrEdefMeasSevr (longSubRecord *psub)
{ 
  unsigned long *check_p = &psub->a;
  int            edefIdx;

  /* for minormask, a bit is set if MEASSEVR is set to MINOR */
  /* for majormask, a bit is set if MEASSEVR is set to either MINOR or MAJOR */
  for (edefIdx = 0; edefIdx < EDEF_MAX; edefIdx++, check_p++) {
    if      (psub->v & (1 << edefIdx)) *check_p = MINOR_ALARM;
    else if (psub->w & (1 << edefIdx)) *check_p = MAJOR_ALARM;
    else                               *check_p = INVALID_ALARM;
  }
  return 0;
}

/*=============================================================================

  Name: mpgEdefInitMask

  Abs:  Create edefInitMask by encoding 20 EDEFINIT inputs

		
  Args: Type	            Name        Access	   Description
        ------------------- -----------	---------- ----------------------------
        longSubRecord *     psub        read       point to subroutine record

  Rem:  Subroutine for IOC:LOCA:UNIT:EDEFINIT 

  Side:

  Sub Inputs/ Outputs:
   Inputs:
    A-T - Inits  (for 20 EDEFs)

   Outputs:   
    A-T - Inits  (for 20 EDEFs), set back to 0
    VAL = EdefInitMask
  Ret:  0

==============================================================================*/
static long mpgEdefInitMask (longSubRecord *psub)
{ 
  unsigned long *check_p = &psub->a;
  int            edefIdx;

  psub->val = 0;
  for (edefIdx = 0; edefIdx < EDEF_MAX; edefIdx++, check_p++) {
    psub->val |= (*check_p) << edefIdx;
    *check_p = 0;  /* now set back to zero for next time */
  } 
  return 0;
}

/*=============================================================================

  Name: evrEdefInitEvent

  Abs:  Post Event for each EDEF Init Flag

  Args: Type                Name        Access     Description
        ------------------- ----------- ---------- ----------------------------
        unsigned long       edefInitMask  read     EDEF Initialization Mask
        unsigned int    edefInitEventCode read     EDEF Initialization Mask

  Rem:  

  Side: Events posted.
  
  Ret:  0 = OK

==============================================================================*/
int evrEdefInitEvent(unsigned long edefInitMask,
                     unsigned int  edefInitEventCode)
{
  int edefIdx;

  for (edefIdx = 0; edefIdx < EDEF_MAX; edefIdx++) {
    if (edefInitMask & (1 << edefIdx)) /* init is set */
      post_event(edefIdx + edefInitEventCode);
  }
  return 0;
}

epicsRegisterFunction(evrModifier5);
epicsRegisterFunction(evrModifier5Bits);
epicsRegisterFunction(mpgEdefMeasSevrMasks);
epicsRegisterFunction(evrEdefMeasSevr);
epicsRegisterFunction(mpgEdefInitMask);

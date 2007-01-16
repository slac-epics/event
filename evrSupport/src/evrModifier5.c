/*=============================================================================
 
  Name: evrModifier5.c
           evrModifer5     - Modifier 5 Creation using EDEF Check Bits
           evrModifer5Bits - Get EDEF Check Bits out of Modifier 5

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

/* c includes */

#include "sSubRecord.h"       /* for struct ssubRecord     */
#include "registryFunction.h" /* for epicsExport           */
#include "epicsExport.h"      /* for epicsRegisterFunction */

#define EDEF_MAX    20
#define NOEDEF_MASK 0xFFF0000

/*=============================================================================

  Name: evrModifier5

  Abs:  Create modifier 5 by adding EDEF enable bits from the pattern
        check records into the rest of Modifer5.

		
  Args: Type	            Name        Access	   Description
        ------------------- -----------	---------- ----------------------------
        sSubRecord *         psub        read       point to subroutine record

  Rem:  Subroutine for IOC:LOCA:UNIT:MODIFIER5NEW

  Side:

  Sub Inputs/ Outputs:
   Inputs:
    A-T - Pattern Check Results (for 20 EDEFs)
    U   - Modifier5 without EDEF bits
    
   Outputs:   
    VAL = Modifier5
  Ret:  0

==============================================================================*/
static long evrModifier5(sSubRecord *psub)
{ 
  unsigned long  mod5;
  double        *check_p = &psub->a;
  int            edefIdx;

  mod5 = (unsigned long)psub->u & NOEDEF_MASK;
  for (edefIdx = 0; edefIdx < EDEF_MAX; edefIdx++, check_p++) {
    mod5 |= ((unsigned long)(*check_p)) << edefIdx;
  }
  psub->val = (double)mod5;
  return 0;
}

/*=============================================================================

  Name: evrModifier5Bits

  Abs:  Get EDEF Check Bits out of Modifier 5
		
  Args: Type	            Name        Access	   Description
        ------------------- -----------	---------- ----------------------------
        sSubRecord *         psub        read       point to subroutine record

  Rem:  Subroutine for IOC:LOCA:UNIT:MODIFIER5NEW

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
static long evrModifier5Bits(sSubRecord *psub)
{ 
  unsigned long  mod5 = (unsigned long)psub->v;
  double        *check_p = &psub->a;
  int            edefIdx;

  psub->u = mod5 & NOEDEF_MASK;
  for (edefIdx = 0; edefIdx < EDEF_MAX; edefIdx++, check_p++) {
    *check_p = mod5 & (1 << edefIdx);
  }
  psub->val = psub->v;
  return 0;
}
epicsRegisterFunction(evrModifier5);
epicsRegisterFunction(evrModifier5Bits);

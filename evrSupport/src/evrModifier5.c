/*=============================================================================
 
  Name: evrModifier5.c
           evrModifer5  - Modifier 5 Creation using EDEF Check Bits

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
    U   - Modifer5 without EDEF bits
    
   Outputs:   
    X  - lower 16 bits (Modifier5A)
    Y  - upper 16 bits (Modifier5B)
    VAL = Modifier5
  Ret:  0

==============================================================================*/
static long evrModifier5(sSubRecord *psub)
{ 
  unsigned long mod5;

  mod5 =     (unsigned long)psub->u;
  mod5 |=    (unsigned long)psub->a;
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
  mod5 |= ( ((unsigned long)psub->t) << 19 );

  psub->val = (double)mod5;
  /* split into 2 16 bit words */
  psub->x = (double) (mod5 & 0xFFFF);
  psub->y = (double) (mod5 >>16 );
  return 0;
 
}
epicsRegisterFunction(evrModifier5);

/*=============================================================================
 
  Name: mpgEvent.h

  Abs:  This include file contains external prototypes for MPG event
        processing routines.

  Auth: 17-Feb-2007, S. Allison 
 
-----------------------------------------------------------------------------*/
#include "copyright_SLAC.h"    
/*----------------------------------------------------------------------------- 
  Mod:  (newest to oldest)  
 
=============================================================================*/

#ifndef INCmpgEventH
#define INCmpgEventH

int mpgEventCounts(double       *lockErrCount_p,
                   double       *busyErrCount_p,
                   double       *modeErrCount_p,
                   double       *enableErrCount_p,
                   double       *patternErrCount_p);
int mpgEventCountReset(void);

#endif /*INCmpgEventH*/

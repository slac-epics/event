/*=============================================================================
 
  Name: pnetSeqCheck.h

  Abs:  This include file contains external prototypes for PNET sequence
        checking routines.

  Auth: 07-Dec-2006, S. Allison 
 
-----------------------------------------------------------------------------*/
#include "copyright_SLAC.h"    
/*----------------------------------------------------------------------------- 
  Mod:  (newest to oldest)  
 
=============================================================================*/

#ifndef INCpnetSeqH
#define INCpnetSeqH

int pnetSeqCheckData1(unsigned char currentSeqCheck1,
                      unsigned int  seqCheckSynced);
int pnetSeqCheckData2(unsigned char currentSeqCheck2,
                      unsigned int  seqCheckSynced);
int pnetSeqCheckData3(unsigned char currentSeqCheck1,
                      unsigned char currentSeqCheck2);
int pnetSeqCheck1Buf(void);
int pnetSeqCheck2Buf(void);
int pnetSeqCheck3Buf(void);

#endif /*INCpnetSeqH*/

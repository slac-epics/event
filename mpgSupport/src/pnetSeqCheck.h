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

#define PNET_SEQ_CHECK          (0x00000007)  /* Sequence check mask         */
                                              /* Left shift 29 first         */
#ifdef PNET_SEQ_GLOBAL
unsigned int  seqCheck1ErrCount = 0;
unsigned int  seqCheck2ErrCount = 0;
unsigned int  seqCheck3ErrCount = 0;
#else
extern unsigned int  seqCheck1ErrCount;
extern unsigned int  seqCheck2ErrCount;
extern unsigned int  seqCheck3ErrCount;
#endif

int pnetSeqCheckData1(unsigned char currentSeqCheck1);
int pnetSeqCheckData2(unsigned char currentSeqCheck2);
int pnetSeqCheckData3(unsigned char currentSeqCheck1,
                      unsigned char currentSeqCheck2);
int pnetSeqCheck1Buf(void);
int pnetSeqCheck2Buf(void);
int pnetSeqCheck3Buf(void);

#endif /*INCpnetSeqH*/

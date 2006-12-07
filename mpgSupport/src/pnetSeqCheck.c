/*=============================================================================

  Name: pnetSeqCheck.c
           pnetSeqCheckData1 - 360Hz PNET Data Seq Check 1
           pnetSeqCheckData2 - 360Hz PNET Data Seq Check 2
           pnetSeqCheckData3 - 360Hz PNET Data Seq Check 3
           pnetSeqCheck1Buf   - PNET Data Seq Check 1 Printout
           pnetSeqCheck2Buf   - PNET Data Seq Check 2 Printout
           pnetSeqCheck3Buf   - PNET Data Seq Check 3 Printout

  Abs: This file contains support for the PNET sequence checking routines.

  Rem:
  
  Auth: 07-jul-2005, Dayle Kotturi (dayle)
  Rev:  

-----------------------------------------------------------------------------*/

#include "copyright_SLAC.h"     /* SLAC copyright comments */

/*-----------------------------------------------------------------------------

  Mod:  (newest to oldest)

=============================================================================*/
#include <stdio.h>            /* printf prototype     */
#include "evrPattern.h"       /* TIMESLOT definitions */
#define PNET_SEQ_GLOBAL
#include "pnetSeqCheck.h"     /* pnetSeqCheck protos  */

#define SEQ_CHECK1_BUF 		(100)		/* last n checksum 1 values */
#define SEQ_CHECK2_BUF 		(100)		/* last n checksum 2 values */
static unsigned char seqCheck1Buf[SEQ_CHECK1_BUF]; 
static unsigned char seqCheck2Buf[SEQ_CHECK2_BUF]; 
static unsigned int  seqCheck1BufIndex = 0; 
static unsigned int  seqCheck2BufIndex = 0;

/*=============================================================================

  Name: pnetSeqCheckData1

  Abs: This routine is a checksum equivalent. The PNET signal contains 
  two independent values that can be used to ensure that (a) no 360 Hz 
  pulse is missed and (b) both ends of the data are present. pnetCheckData1 
  is looking at the front end of the pnet data to see that a "traveling 1" 
  is sent in six bits. The pattern looks like this across six adjacent pulses:
  000001
  000010
  000100
  001000
  010000
  100000

  Upon detection of an error, a counter is incremented and the reference
  is reset. 

=============================================================================*/
int pnetSeqCheckData1(unsigned char currentSeqCheck1) {
  static unsigned char seqCheck1 = 0;
  static unsigned int  seqCheck1First = 1;   /* used to start/reset the data validity checking */
  
  /* the 6 bit "traveling one" pattern for this beam pulse */

  /* synch up the first time or after an error by setting what was read 
     to be the baseline */
  if (seqCheck1First) {
    seqCheck1 = currentSeqCheck1;
    seqCheck1First = 0; /* now reset */
    seqCheck1BufIndex = 0;
  }
  else {
    seqCheck1BufIndex++;
    if (seqCheck1BufIndex >= SEQ_CHECK1_BUF) {
      seqCheck1BufIndex = 0;
    }
  }
    
  /* Good or bad, store in a ring buffer of last SEQ_CHECK1_BUF chars, so
     it can be accessed */
  seqCheck1Buf[seqCheck1BufIndex] = currentSeqCheck1;

  if (seqCheck1 == currentSeqCheck1) {
    /* data frame is fine. increment traveling_one for next time */

    if (seqCheck1 == TIMESLOT6_MASK) { 
      seqCheck1 = TIMESLOT1_MASK;
    } else {
      seqCheck1 = (seqCheck1 << 1);
    }
  } else {
    /* Data is not fine. flag and set flag to resync */
    seqCheck1ErrCount++;
    seqCheck1First = 1;
    return -1;
  }
  return 0;
}

/*=============================================================================

  Name: pnetSeqCheckData2

  Abs: This routine is a checksum equivalent.  The PNET signal contains 
  two independent values that can be used to ensure that (a) no 360 Hz 
  pulse is missed and (b) both ends of the data are present. pnetCheckData2
  is looking at the tail end of the pnet data to see that a cyclic counter
  [0-5] is sent. The pattern looks like this across six adjacent pulses:
  001
  010
  011
  100
  101
  110

  Upon detection of an error, a counter is incremented and the reference
  is reset.  

=============================================================================*/
int pnetSeqCheckData2(unsigned char currentSeqCheck2) {
  static unsigned char seqCheck2 = 0;
  static unsigned int  seqCheck2First = 1;   /* used to start/reset the data validity checking */
  
  /* the 3 bit counter for this beam pulse */


  /* synch up the first time or after an error by setting what was read 
     to be the baseline */
  if (seqCheck2First) {
    seqCheck2 = currentSeqCheck2;
    seqCheck2First = 0; /* now reset */
    seqCheck2BufIndex = 0;
  }
  else {
    seqCheck2BufIndex++;
    if (seqCheck2BufIndex >= SEQ_CHECK2_BUF) {
      seqCheck2BufIndex = 0;
    }
  }

  /* Good or bad, store in a ring buffer of last SEQ_CHECK2_BUF chars, so
     it can be accessed */
  seqCheck2Buf[seqCheck2BufIndex] = currentSeqCheck2;

  if (seqCheck2 == currentSeqCheck2) {
    /* End of data frame is fine. increment seqchk for next time */
    if (seqCheck2 == 6) { /* 6 time slots */
      seqCheck2 = 1;
    } else {
      seqCheck2++;
    }
  } else {
    /* End of data frame is not fine. flag and set flag to resync */
    /* Need to do more here to really handle this condition. TEG says
       if MPG gets "off" chances are beam is hitting the sides. The
       reaction of choice is for the micro to start sending zero beam
     */
    seqCheck2ErrCount++;
    seqCheck2First = 1;
    return -1;
  }
  return 0;
}

/*=============================================================================

  Name: pnetSeqCheckData3

  Abs: This routine is a checksum equivalent.  The PNET signal contains 
  two independent values that can be used to ensure that (a) no 360 Hz 
  pulse is missed and (b) both ends of the data are present. pnetCheckData3
  is looking at both the head and the tail ends of the pnet data to see that
  their values stay in sync such that 
  TSLOG_U6 has value when PNET_SEQCHK has value    
   000001                 001
   000010                 010
   000100                 011
   001000                 100
   010000                 101
   100000                 110

  Upon detection of an error, a counter is incremented and the reference
  is reset. 

=============================================================================*/
int pnetSeqCheckData3(unsigned char currentSeqCheck1,
                      unsigned char currentSeqCheck2) {

  switch(currentSeqCheck1) {
    case TIMESLOT1_MASK: if ((int) currentSeqCheck2 != 1) {
              seqCheck3ErrCount++;
              return -1;
            }
            break;
          
    case TIMESLOT2_MASK: if ((int) currentSeqCheck2 != 2) {
              seqCheck3ErrCount++;
              return -1;
            }
            break;

    case TIMESLOT3_MASK: if ((int) currentSeqCheck2 != 3) {
              seqCheck3ErrCount++;
              return -1;
            }
            break;

    case TIMESLOT4_MASK: if ((int) currentSeqCheck2 != 4) {
              seqCheck3ErrCount++;
              return -1;
            }
            break;

    case TIMESLOT5_MASK: if ((int) currentSeqCheck2 != 5) {
              seqCheck3ErrCount++;
              return -1;
            }
            break;

    case TIMESLOT6_MASK: if ((int) currentSeqCheck2 != 6) {
              seqCheck3ErrCount++;
              return -1;
            }
            break;
    default: 
            seqCheck3ErrCount++;
            return -1;
            break; 
    }

    return 0;
}

/*=============================================================================

  Name: pnetSeqCheck1Buf

  Abs:  Dump the last SEQ_CHECK1_BUF values of the TSLOG_U6 field

  Rem:  The TSLOG_U6 value is significant. It tells which BGRP it is.
        The six values correspond to BGPs 0,1,2,3,4,5 (where 3,4,5 and just
        a super-cycle of 0,1,2, at least they are if you're a 120 Hz machine).

        Healthy output looks like this:
        .
        .
        .
        57 25 1
        58 26 2
        59 27 4
        60 28 8
        61 29 16
        62 30 32
        63 31 1
        64 32 2
        65 33 4
        66 34 8
        67 35 16
        68 36 32
        .
        .
        .
=============================================================================*/
int pnetSeqCheck1Buf(void) {
  int ii;     /* loop counter */
  int offset; /* the offset to the oldest data in the ring buffer */
  int jj;     /* current position in ring buffer */

  if (seqCheck1BufIndex == (SEQ_CHECK1_BUF - 1)) {
    offset=0; /* if the newest data is right at the end, then the oldest is
                 at the beginning */
  } else {
    offset=seqCheck1BufIndex + 1; /* the oldest data is found right after the 
                                     end of the newest */
  }

  printf("loop actpos val\n"); 
  for (ii=0; ii<SEQ_CHECK1_BUF; ii++) {
    jj = offset + ii; 

    if (jj > (SEQ_CHECK1_BUF-1)) { /* wrap around if at end */
      jj = jj - SEQ_CHECK1_BUF;
    }
    
    printf("%d %d %d\n", ii, jj, 
      ((int) seqCheck1Buf[jj]) & TIMESLOT_MASK /* only care about 6 LSBs */);
  }
  printf("\n");

  return 0;
}

/*=============================================================================

  Name: pnetSeqCheck2Buf
  
  Abs:  Dump the last SEQ_CHECK2_BUF values of the PNET_SEQCHK field
        Healthy output looks like this:
   	.
   	.
   	.
   	70 90 6
   	71 91 1
   	72 92 2
   	73 93 3
   	74 94 4
   	75 95 5
   	76 96 6
   	77 97 1
   	78 98 2
   	79 99 3
   	80 0 4

=============================================================================*/
int pnetSeqCheck2Buf(void) {
  int ii;
  int offset; /* the offset to the oldest data in the ring buffer */
  int jj;     /* current position in ring buffer */

  if (seqCheck2BufIndex == (SEQ_CHECK2_BUF - 1)) {
    offset=0; /* if the newest data is right at the end, then the oldest is
                 at the beginning */
  } else {
    offset=seqCheck2BufIndex + 1; /* the oldest data is found right after the 
                                     end of the newest */
  }

  printf("loop actpos val\n"); 
  for (ii=0; ii<SEQ_CHECK2_BUF; ii++) {
    jj = offset + ii; 

    if (jj > (SEQ_CHECK2_BUF-1)) { /* wrap around if at end */
      jj = jj - SEQ_CHECK2_BUF;
    }
    
    printf("%d %d %d\n", ii, jj, 
      ((int) seqCheck2Buf[jj]) & PNET_SEQ_CHECK /* only care about 3 LSBs */);
  }
  printf("\n");
  return 0;
}

int pnetSeqCheck3Buf(void) {
  int ii;
  int offset; /* the offset to the oldest data in the ring buffer */
  int jj;     /* current position in ring buffer */

  if (seqCheck2BufIndex == (SEQ_CHECK2_BUF - 1)) {
    offset=0; /* if the newest data is right at the end, then the oldest is
                 at the beginning */
  } else {
    offset=seqCheck2BufIndex + 1; /* the oldest data is found right after the 
                                     end of the newest */
  }

  printf("loop actpos val\n");
  for (ii=0; ii<SEQ_CHECK2_BUF; ii++) {
    jj = offset + ii;
                                                                                
    if (jj > (SEQ_CHECK2_BUF-1)) { /* wrap around if at end */
      jj = jj - SEQ_CHECK2_BUF;
    }
                                                                                
    printf("%d %d %d %d\n", ii, jj, (int) seqCheck1Buf[jj],  (int) seqCheck2Buf[jj]);
  }
  printf("\n");
  return 0;
}

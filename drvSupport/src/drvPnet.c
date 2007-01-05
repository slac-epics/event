/*=============================================================================
 
  Name: drvPnet.c
        pnetInitialise - hardware initialisation
        pnetReport     - driver report
        pnetISR        - interrupt service routine
        ...

  Abs:  Driver support for VME PNET Receiver module. 
  
  Refs: See list of docs under the heading "VME PNET Module" on page:
        http://www.slac.stanford.edu/grp/lcls/controls/global/subsystems/timing

  Compiler Flags:
  DEBUGPRINT - 
  PNET_TIMING - 

  Auth: 07-jul-2005, Dayle Kotturi (dayle):
  Rev:  dd-mmm-200y, Tom Slick (TXS):

-----------------------------------------------------------------------------*/

#include "copyright_SLAC.h"	/* SLAC copyright comments */
 
/*-----------------------------------------------------------------------------
 
  Mod:  (newest to oldest)  
        DD-MMM-YYYY, My Name:
           Changed such and such to so and so. etc. etc.
        DD-MMM-YYYY, Your Name:
           More changes ... The ordering of the revision history 
           should be such that the NEWEST changes are at the HEAD of
           the list.
 
=============================================================================*/

#include <math.h>               /* for sqrt */

#include <drvSup.h> 		/* for DRVSUPFN */
#ifdef __rtems__
#include <devLib.h>		/* for dev*routines, atVMEA16 */
#include <basicIoOps.h>         /* for in_be32 */
#endif
#include <errlog.h>		/* for errlogPrintf */
#include <epicsExport.h> 	/* for epicsExportAddress */
#include <debugPrint.h>		/* for DEBUGPRINT */
#include <evrQueue.h>		/* for evrQueueCreate */

#define PNET_IRQ_LEVEL 		(1)	/* PNET interrupt level */
#define PNET_IRQ_VECTOR 	(0xAA)	/* PNET interrupt vector */
#define PNET_DEF_BASE_ADDRESS   (0x110) /* PNET default base address */

/* Define this in order to measure/store durations in routines */
#ifdef PNET_TIMING      
  #ifdef __PPC__
    #define MFTB(var) asm volatile("mftb %0":"=r"(var))
  #else
    #define MFTB(var) do {} ()
  #endif
  #define PNET_ISR_TIMING_SIZE	(100)
  #define CLOCK_SPEED           (16.7)          /* MHz */
#endif

static int pnetReport();
static int pnetInitialise();
struct drvet drvPnet = {
  2,
  (DRVSUPFUN) pnetReport, 	/* subroutine defined in this file */
  (DRVSUPFUN) pnetInitialise 	/* subroutine defined in this file */
};
epicsExportAddress(drvet, drvPnet);

#ifdef DEBUG_PRINT
  /* ENUM with values: DP_FATAL, DP_ERROR, DP_WARN, DP_INFO, DP_DEBUG
     Anything less than or equal to drvPnetFlag gets printed out. Can
     be set to 0 here and altered via prompt or in st.cmd file */
int drvPnetFlag = 0;
#endif

int intrEnabled = 0;

static unsigned long    vmePnetAddr = PNET_DEF_BASE_ADDRESS; /* base addr of PNET */
#ifdef __rtems__
static epicsUInt32     *pLocalBuf   = NULL; /* keep pLocalBuf a ptr for devRegAddr */
#endif

static evrQueueId       pnetQueue   = 0;    /* PNET data queue information */

#ifdef PNET_TIMING
  static unsigned long isrStart[PNET_ISR_TIMING_SIZE];
  static unsigned long isrEnd[PNET_ISR_TIMING_SIZE];
  static int isrTimingIndex = 0;
#endif

#ifdef PNET_TIMING
/*=============================================================================

  Name: pnetGetIsrTimes

  Abs: Writes out the avg and stddev of last 100 durations of ISR processing 
        time in microseconds 

  Side: Output written to serial console stdout 

=============================================================================*/
int pnetGetIsrTimes() {
  int ii;
  int jj;
  double duration[PNET_ISR_TIMING_SIZE];
  double sumDurations = 0.;
  double sumSqDurations = 0.;

  for (ii=0; ii<PNET_ISR_TIMING_SIZE; ii++) {
    jj = isrTimingIndex + ii;
    if (jj > (PNET_ISR_TIMING_SIZE-1)) {
      jj -= PNET_ISR_TIMING_SIZE;  
    } 
    duration[ii] = ((double)isrEnd[jj] - (double)isrStart[jj])/CLOCK_SPEED;
    sumDurations += duration[ii]; 
  }

  /* convert sum to be the average */
  sumDurations /= PNET_ISR_TIMING_SIZE;
  printf("Average time in ISR for last 100 interrupts: %f usec\n", sumDurations);

  /* std dev needs 2nd loop. formula is: SD = sqrt { sum(x - mu)^2 / (n-1) } */
  for (ii=0; ii<PNET_ISR_TIMING_SIZE; ii++) {
    sumSqDurations += (duration[ii] - sumDurations) *
                      (duration[ii] - sumDurations);
  }
  sumSqDurations /= (PNET_ISR_TIMING_SIZE - 1);
  sumSqDurations = sqrt(sumSqDurations);
  printf("Stddev of times in ISR for last 100 interrupts: %f usec\n",
         sumSqDurations);
  return 0;
}
#endif

/*=============================================================================

  Name: pnetReport

  Abs: Driver support registered function for reporting system info

  Note: Although this routine takes an input "interest", it is not used
        except as the return value (silly).  

=============================================================================*/
static int pnetReport( int interest )
{
  evrQueueReport(pnetQueue, EVR_QUEUE_PNET_NAME);
  
  #ifdef PNET_TIMING
  pnetGetIsrTimes();
  #endif
  return interest; 	/* silly */
}

#ifdef __rtems__
/*=============================================================================
 
  Name: pnetISR

  Abs: Process interrupts at PNET_IRQ_LEVEL, put each buffer of PNET data
       into a message queue.

  Rem: Strategy 1: read the entire buffer in before incrementing the pointer.
       This way the pointer always points to a complete buffer and so there
       is no chance of reading a partial buffer.

       Strategy 2: keep this routine to a minimum, so that CPU not blocked 
       too long processing each interrupt. Some lessons learned about what 
       can't be in an ISR, for the record.
        - no printfs
        - no floating point calculations
        - no calls to epicsTimeGetCurrent (while it's true that an epicsTime-
          stamp variable only contains longs, there is some sys call deep
          down that uses f.p. and throws the ISR into this error:
 
          rtems-4.6.2(PowerPC/PowerPC 7455/mvme5500)
          FATAL ERROR:
          Internal error: No
          Environment: RTEMS Core
            Error occurred in a Thread Dispatching DISABLED context (level 1)
            Error occurred from ISR context (ISR nest level 1)
          Error 18: INTERNAL_ERROR_CALLED_FROM_WRONG_ENVIRONMENT
          Stack Trace:

          0x000B9D2C--> 0x000B9D2C--> 0x000DE6B8--> 0x001103CC--> 0x01F3798C
          0x01F36EEC--> 0x01DA63B8--> 0x01D011A4--> 0x000BF460--> 0x000B8AA0
          0x000B8844--> 0x000B8ED8

=============================================================================*/
static void pnetISR (
  /* if there was more than one PNET board, this would spec which one */
  void *arg 
) {
  unsigned long ii;
  volatile epicsUInt32 modifier_a[EVR_PNET_MODIFIER_MAX];

  #ifdef PNET_TIMING
  /* Start counting the clock ticks to see how long spent in here */
  MFTB(isrStart[isrTimingIndex]);
  #endif

 /* Take a local copy of the message into the NEXT space so that it won't be
     accessed */
  for (ii=0; ii<EVR_PNET_MODIFIER_MAX; ii++) {
    modifier_a[ii] = in_be32((volatile void *)&(pLocalBuf[ii]));
  }
  /* Send the data on to the waveform record.*/
  
  evrQueueSend(pnetQueue, (void *)modifier_a);
  
  #ifdef PNET_TIMING
  /* Stop counting the clock ticks to see how long spent in here */
  MFTB(isrEnd[isrTimingIndex]);
  if (isrTimingIndex == PNET_ISR_TIMING_SIZE ) {
    isrTimingIndex = 0;
  } else {
    isrTimingIndex++; 
  }
  #endif
}
#endif  /* ifdef __rtems__ */

/*=============================================================================
 
  Name: pnetSetBaseAddress

  Abs:  Changes the base address of the PNET VME receiver board

  Rem:  Here's how to use it: 
        Call this in st.cmd prior to iocInit if you want to change 
        from the default base address which is the address to the start
        of the header (16 Bytes long), which is followed by 16 Bytes
        of data. 
 
=============================================================================*/
int pnetSetBaseAddress(unsigned long addr) {
  vmePnetAddr = addr;
  return 0;
}
int pnetEnableIntr() {

  if (!intrEnabled ) { /* only enable it once, even with recurring calls */
    
#ifdef __rtems__
      /* enable the interrupt. Note: if this code is executed, the LED on the 
         PNET board starts flashing */
      int rc = devEnableInterruptLevelVME(PNET_IRQ_LEVEL);
      if (rc != 0) {
        errlogPrintf("pnetEnableIntr: unable to enable the interrupt. rc = %d\n",
                     rc);
        return -1;
      }
#endif  /* ifdef __rtems__ */
      intrEnabled = 1;     
  }
  return 0;
}

int pnetDisableIntr() {

#ifdef __rtems__
  /* disable the interrupt. Note: if this code is executed, the LED on the 
     PNET board starts flashing */
  int rc = devDisableInterruptLevelVME(PNET_IRQ_LEVEL);
  if (rc != 0) {
    errlogPrintf("pnetDisableIntr: unable to disable the interrupt. rc = %d\n",
                 rc);
    return -1;
  }
#endif  /* ifdef __rtems__ */
  intrEnabled = 0;
  return 0;
}

static int pnetInitialise() {
#ifdef __rtems__
  int rc;

  /* Notes:
     - vmePnetAddr was set to default at boot but could have been changed by
     invoking pnetSetBaseAddress() prior to iocInit during startup. 
     - no volatile typecast on (void**) &pLocalBuf because pnetBackplane_t i
     is volatile 
     - last arg is ptr to a ptr - really. you get a warning re: incompatible 
     ptr type, but without '&', you only read zeroes (and you STILL have 
     the warning 
   */
  rc = devRegisterAddress( "pnet", atVMEA24, vmePnetAddr,
                           sizeof(epicsUInt32)*EVR_PNET_MODIFIER_MAX,
                           (void*)&pLocalBuf); 
  if (rc != 0) {
    errlogPrintf("pnetInitialise: unable to register VME/A24 address 0x%08x\n",
                 (int) vmePnetAddr);
    return -1;
  }

  DEBUGPRINT(DP_INFO, drvPnetFlag,
             ("pnetInitialise: hardware registered at 0x%08x\n",
              (int) vmePnetAddr));
  
  /* connect up the interrupt vector with the interrupt service routine */
  rc = devConnectInterruptVME(PNET_IRQ_VECTOR, pnetISR, 0);
  if (rc != 0) {
    errlogPrintf("pnetInitialise: unable to connect intr vector to ISR. rc = %d\n",
                 rc);
    return -1;
  }
#endif  /* ifdef __rtems__ */
  pnetQueue = evrQueueCreate(EVR_QUEUE_PNET_NAME,
                             sizeof(epicsUInt32)*EVR_PNET_MODIFIER_MAX);
  if (!pnetQueue) {
    errlogPrintf("pnetInitialise: %s message queue creation failed\n",
                 EVR_QUEUE_PNET_NAME);
    return -1;
  }

  /* enable the interrupt. Note: if this code is executed,
     the LED on the PNET board starts flashing */
  if(pnetEnableIntr()) return -1;

  return (0);
}

/*=============================================================================
 
  Name: pnetSend

  Abs: Called by a subroutine record for the PNET simulator.

  Rem:
  
=============================================================================*/
void pnetSend(void *message)
{
  evrQueueSend(pnetQueue, message);
}

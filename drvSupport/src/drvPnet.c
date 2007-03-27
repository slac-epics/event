/*=============================================================================
 
  Name: drvPnet.c
        pnetInitialise - hardware initialisation
        pnetReport     - driver report
        pnetISR        - interrupt service routine
        pnetTask       - high priority pnet processing task

  Abs:  Driver support for VME PNET Receiver module. 
  
  Refs: See list of docs under the heading "VME PNET Module" on page:
        http://www.slac.stanford.edu/grp/lcls/controls/global/subsystems/timing

  Compiler Flags:
  DEBUG_PRINT - To define debug flag. 

  Auth: 07-jul-2005, Dayle Kotturi (dayle):
  Rev:  13-mar-2007, S. Allison (saa)

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

#include <drvSup.h> 		/* for DRVSUPFN */
#ifdef __rtems__
#include <devLib.h>		/* for dev*routines, atVMEA16 */
#include <basicIoOps.h>         /* for in_be32 */
#endif
#include <errlog.h>		/* for errlogPrintf */
#include <epicsExport.h> 	/* for epicsExportAddress */
#include <epicsEvent.h> 	/* for epicsEvent* */
#include <epicsThread.h> 	/* for epicsThreadCreate */
#include <debugPrint.h>		/* for DEBUGPRINT */
#include <evrMessage.h>		/* for evrMessage* protos */

#define PNET_IRQ_LEVEL 		(1)	/* PNET interrupt level */
#define PNET_IRQ_VECTOR 	(0xAA)	/* PNET interrupt vector */
#define PNET_DEF_BASE_ADDRESS   (0x110) /* PNET default base address */

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

static int              intrEnabled = 0;
static unsigned long    vmePnetAddr = PNET_DEF_BASE_ADDRESS; /* base addr of PNET */
static epicsUInt32     *pLocalBuf   = NULL; /* keep pLocalBuf a ptr for devRegAddr */
static epicsEventId     pnetTaskEventSem = NULL;  /* pnet task semaphore */
static volatile int     pnetAvailable    = 0;     /* flag that pnet data is ready */

/*=============================================================================

  Name: pnetReport

  Abs: Driver support registered function for reporting system info

=============================================================================*/
static int pnetReport( int interest )
{
  if (interest > 0) {
    printf ("PNET Card Address = %8.8x,  Vector = %3.3X,  Level = %d, %s\n",
            (unsigned int)pLocalBuf, PNET_IRQ_VECTOR, PNET_IRQ_LEVEL,
            intrEnabled? "Enabled" : "Disabled");
    evrMessageReport(EVR_MESSAGE_PNET, EVR_MESSAGE_PNET_NAME, interest);
  }
  return interest;
}

/*=============================================================================
 
  Name: pnetISR

  Abs: Process interrupts at PNET_IRQ_LEVEL - record start time and wake
       up pnetTask.

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
void pnetISR (
  /* if there was more than one PNET board, this would spec which one */
  void *arg)
{
#ifdef __rtems__
  unsigned long ii;
  evrMessage_tu message_u;
#endif
  
  evrMessageStart(EVR_MESSAGE_PNET);
#ifdef __rtems__
  for (ii=0; ii<EVR_PNET_MODIFIER_MAX; ii++) {
    message_u.pnet_s.modifier_a[ii] =
      in_be32((volatile void *)&(pLocalBuf[ii]));
  }
  evrMessageWrite(EVR_MESSAGE_PNET, &message_u);
#endif
  evrMessageStart(EVR_MESSAGE_FIDUCIAL);
  pnetAvailable = 1;
  epicsEventSignal(pnetTaskEventSem);
}

/*=============================================================================
                                                                                
  Name: pnetTask
                                                                                
  Abs:  This task performs record processing and monitors the PNET module.                                                                         
  Rem:  It's started by pnetInitialise after the PNET module is configured. 
    
=============================================================================*/
static int pnetTask()
{  
  for (;;)
  {
    epicsEventMustWait(pnetTaskEventSem);
    if (pnetAvailable) {
      /* Advance the pipeline and then process the data */
      evrMessageProcess(EVR_MESSAGE_FIDUCIAL);
      evrMessageProcess(EVR_MESSAGE_PNET);
      pnetAvailable = 0;
    }
  }
  return 0;
}

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

  int rc;

  /* Create space for the fiducial + diagnostics */
  rc = evrMessageCreate(EVR_MESSAGE_FIDUCIAL_NAME, 0);
  if (rc != EVR_MESSAGE_FIDUCIAL) return -1;
  
  /* Create space for the PNET message + diagnostics */
  rc = evrMessageCreate(EVR_MESSAGE_PNET_NAME, sizeof(evrMessagePnet_ts));
  if (rc != EVR_MESSAGE_PNET) return -1;
  /* Create the semaphore used by the ISR to wake up the pnet task */
  pnetTaskEventSem = epicsEventMustCreate(epicsEventEmpty);
  if (!pnetTaskEventSem) {
    errlogPrintf("pnetInitialise: unable to create the PNET task semaphore\n");
    return -1;
  }  
  /* Create the task to read the message */
  if (!epicsThreadCreate("pnetTask", epicsThreadPriorityHigh,
                         epicsThreadGetStackSize(epicsThreadStackMedium),
                         (EPICSTHREADFUNC)pnetTask, 0)) {
    errlogPrintf("pnetInitialise: unable to create the PNET task\n");
    return -1;
  }
  
#ifdef __rtems__

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

  /* enable the interrupt. Note: if this code is executed,
     the LED on the PNET board starts flashing */
  if(pnetEnableIntr()) return -1;

  return (0);
}

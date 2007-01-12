/*=============================================================================
 
  Name: drvEvr.c
        evrInitialise  - EVR data queue initialisation
        evrReport      - driver report
        evrSend        - Send to EVR data queue

  Abs:  Driver data support for EVR Receiver module or EVR simulator.   

  Auth: 22-dec-2006, S. Allison:
  Rev:  

-----------------------------------------------------------------------------*/

#include "copyright_SLAC.h"	/* SLAC copyright comments */
 
/*-----------------------------------------------------------------------------
 
  Mod:  (newest to oldest)  
 
=============================================================================*/

#include <drvSup.h> 		/* for DRVSUPFN */
#ifdef __rtems__
#include <drvMrfEr.h>		/* for Er* prototypes */
#endif
#include <errlog.h>		/* for errlogPrintf */
#include <epicsExport.h> 	/* for epicsExportAddress */
#include <evrMessage.h>		/* for evrQueueCreate */

static int evrReport();
static int evrInitialise();
struct drvet drvEvr = {
  2,
  (DRVSUPFUN) evrReport, 	/* subroutine defined in this file */
  (DRVSUPFUN) evrInitialise 	/* subroutine defined in this file */
};
epicsExportAddress(drvet, drvEvr);

/*=============================================================================

  Name: evrReport

  Abs: Driver support registered function for reporting system info

=============================================================================*/
static int evrReport( int interest )
{
  if (interest > 0) {
    evrMessageReport(EVR_MESSAGE_PATTERN, EVR_MESSAGE_PATTERN_NAME);
    evrMessageReport(EVR_MESSAGE_DATA,    EVR_MESSAGE_DATA_NAME);
  }
  return interest;
}

/*=============================================================================
 
  Name: evrSend

  Abs: Called by either ErIrqHandler to put each buffer of EVR data
       into the proper message area or by a subroutine record for
       the EVR simulator.

  Rem: Keep this routine to a minimum, so that CPU not blocked 
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
void evrSend(void *pCard, epicsInt16 messageSize, void *message)
{
  evrMessageWrite(((evrMessageHeader_ts *)message)->type,
                  (evrMessage_tu *)message);
}

static int evrInitialise()
{
#ifdef __rtems__
  ErCardStruct  *pCard;
#endif

  /* Initialize space to hold EVR data buffer - for now,
     only pattern is supported */
  if (!evrMessageCreate(EVR_MESSAGE_PATTERN_NAME,
                        sizeof(evrMessagePattern_ts))) return -1;
#ifdef __rtems__
  /* Get first EVR in the list */
  pCard = ErGetCardStruct(-1);
  if (!pCard) {
    errlogPrintf("evrInitialise: cannot find an EVR module\n");
  /* Register the ISR function in this file with the EVR */
  } else {
    ErRegisterDevDBuffHandler(pCard, (DEV_DBUFF_FUNC)evrSend);
    /* Finally, enable the data stream on the EVR */
    ErEnableDBuff(pCard, 1);
  }
#endif
  
  return 0;
}

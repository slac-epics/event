/*=============================================================================

  Name: devWFevrQueue.c

  Abs:  Waveform device support for reading waveforms from the EVR/PNET message
        queue.

  Rem:
  
  Auth: 21-Dec-2006, S. Allison
  Rev:
-----------------------------------------------------------------------------*/

#include "copyright_SLAC.h"     /* SLAC copyright comments */

/*-----------------------------------------------------------------------------

  Mod:  (newest to oldest)

=============================================================================*/

#include <string.h>             /* strlen                      */
#include <stdlib.h>             /* calloc                      */

#include "alarm.h"              /* READ_ALARM, INVALID_ALARM   */
#include "recGbl.h"             /* recGbl* prototypes          */
#include "dbFldTypes.h"         /* DBF_LONG                    */
#include "dbAccess.h"           /* S_db_badField               */
#include "dbScan.h"             /* IOSCANPVT                   */
#include "devSup.h"             /* DEVSUPFUN                   */
#include "waveformRecord.h"     /* waveformRecord typedef      */
#include "epicsExport.h"        /* epicsExportAddress          */
#include "epicsMessageQueue.h"  /* epicsMessageQueueTryReceive */

#include "evrQueue.h"           /* evrQueueRegister            */

/* Create the dset for devWfMsgQueue */
static long init_record();
static long read_wf();
static long get_ioint_info();
struct {
	long		number;
	DEVSUPFUN	report;
	DEVSUPFUN	init;
	DEVSUPFUN	init_record;
	DEVSUPFUN	get_ioint_info;
	DEVSUPFUN	read_wf;
}devWFevrQueue={
	5,
	NULL,
	NULL,
	init_record,
	get_ioint_info,
	read_wf
};
epicsExportAddress(dset,devWFevrQueue);

static long init_record(waveformRecord *pwf)
{  
  /* Only unsigned longs are supported */
  if (pwf->ftvl != DBF_ULONG) {
    recGblRecordError(S_db_badField,(void *)pwf,
                      "devWFevrQueue (init_record) FTVL must be LONG");
    return(S_db_badField);
  }
  /* INP field must be INST_IO string of less than 40 characters - only
     pattern and data are supported. */
  if (pwf->inp.type != INST_IO) {
    recGblRecordError(S_db_badField,(void *)pwf,
                      "devWFevrQueue (init_record) Illegal INP field");
    return(S_db_badField);
  }
  /* Register this waveform with the EVR or PNET support and get queue ID */
  pwf->dpvt = evrQueueRegister(pwf->inp.value.instio.string,
                               pwf->nelm * sizeof(long));
  if (!pwf->dpvt) return -1;
  return 0;
}

static long read_wf(waveformRecord *pwf)
{
  if (evrQueueReceive((evrQueueId)pwf->dpvt, pwf->bptr)) {
    recGblSetSevr(pwf, READ_ALARM, INVALID_ALARM);
    pwf->nord = 0;
    return -1;
  }
  pwf->nord = pwf->nelm;
  return 0;
} 
static long get_ioint_info(int cmd, waveformRecord *pwf, IOSCANPVT *ppvt)
{
  if (pwf->dpvt) {
    *ppvt =((evrQueueId)pwf->dpvt)->ioscanpvt;
    return 0;
  }
  *ppvt = 0;
  return -1;
}

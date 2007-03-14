/*=============================================================================

  Name: devWFevrMessage.c

  Abs:  Waveform device support for reading waveforms from the EVR/PNET message
        space.

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
#include "dbFldTypes.h"         /* DBF_ULONG                   */
#include "dbAccess.h"           /* S_db_badField               */
#include "devSup.h"             /* DEVSUPFUN                   */
#include "waveformRecord.h"     /* waveformRecord typedef      */
#include "epicsExport.h"        /* epicsExportAddress          */

#include "evrMessage.h"         /* evrMessage* prototypes      */

/* Create the dset for devWfMsgMessage */
static long init_record();
static long read_wf();
struct {
	long		number;
	DEVSUPFUN	report;
	DEVSUPFUN	init;
	DEVSUPFUN	init_record;
	DEVSUPFUN	get_ioint_info;
	DEVSUPFUN	read_wf;
}devWFevrMessage={
	5,
	NULL,
	NULL,
	init_record,
	NULL,
	read_wf
};
epicsExportAddress(dset,devWFevrMessage);

static long init_record(waveformRecord *pwf)
{  
  /* Only unsigned longs are supported */
  if (pwf->ftvl != DBF_ULONG) {
    recGblRecordError(S_db_badField,(void *)pwf,
                      "devWFevrMessage (init_record) FTVL must be LONG");
    return(S_db_badField);
  }
  /* INP field must be INST_IO string of less than 40 characters - only
     pattern and data are supported. */
  if (pwf->inp.type != INST_IO) {
    recGblRecordError(S_db_badField,(void *)pwf,
                      "devWFevrMessage (init_record) Illegal INP field");
    return(S_db_badField);
  }
  /* Register this waveform with the EVR or PNET support and get queue ID */
  pwf->dpvt = (void *)evrMessageRegister(pwf->inp.value.instio.string,
                                         pwf->nelm * sizeof(long),
                                         (dbCommon *)pwf);
  if (pwf->dpvt < 0) return -1; 
  return 0;
}

static long read_wf(waveformRecord *pwf)
{
  if (evrMessageRead((unsigned int)pwf->dpvt, (evrMessage_tu *)pwf->bptr)) {
    recGblSetSevr(pwf, READ_ALARM, INVALID_ALARM);
    pwf->nord = 0;
    return -1;
  }
  pwf->nord = pwf->nelm;
  return 0;
} 

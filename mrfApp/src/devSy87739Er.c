#include <devSup.h>
#include <epicsExport.h>
#include <sy87739Record.h>
#include <drvMrfEr.h>
#undef EVR_MAX_BUFFER /* What a horrible hack! */
#include <erapi.h>
#include <errlog.h>
#include <sy87739.h>

static long init_record(sy87739Record *prec)
{
ErCardStruct *pCard;
	if ( VME_IO != prec->out.type ) {
		errlogPrintf("devSy87739Er: invalid OUT link type\n");
		prec->pact = TRUE;
		return -1;
	}
	if ( ! (pCard = ErGetCardStruct( prec->out.value.vmeio.card )) ) {
		errlogPrintf("devSy87739Er: invalid OUT link value - no card found\n");
		prec->pact = TRUE;
		return -1;
	}

	prec->cwd = EvrGetFracDiv( pCard->pEr );

	prec->dpvt = pCard;

	return 0;
}

static long write_out(sy87739Record *prec)
{
ErCardStruct *pCard = (ErCardStruct*)prec->dpvt;
Sy87739CtlWrd cwrd  = prec->cwd;

	cwrd &= 0x1fffc7ff;

	epicsMutexLock( pCard->CardLock );
		EvrSetFracDiv( pCard->pEr, cwrd );
	epicsMutexUnlock( pCard->CardLock );

	return 0;
}

static struct {
		long number;
		DEVSUPFUN report;
		DEVSUPFUN init;
		DEVSUPFUN init_record;
		DEVSUPFUN get_ioint_info;
		DEVSUPFUN write;
} devSy87739ErSup = {
		5,
		(DEVSUPFUN)NULL,
		(DEVSUPFUN)NULL,
		(DEVSUPFUN)init_record,
		(DEVSUPFUN)NULL,
		(DEVSUPFUN)write_out
};

epicsExportAddress(dset, devSy87739ErSup);

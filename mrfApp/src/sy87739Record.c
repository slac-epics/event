/* sy87739Record.c */
/* record support module for Sy87739 PLL (used in EVR) */

/* T. Straumann, 2016 */
  
#include <stddef.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "epicsMath.h"
#include "alarm.h"
#include "dbAccess.h"
#include "recGbl.h"
#include "dbEvent.h"
#include "dbDefs.h"
#include "dbAccess.h"
#include "devSup.h"
#include "errMdef.h"
#include "recSup.h"
#include "special.h"

#include <sy87739.h>

#define GEN_SIZE_OFFSET
#include "sy87739Record.h"
#undef  GEN_SIZE_OFFSET
#include "epicsExport.h"

/* Create RSET - Record Support Entry Table */
#define report NULL
#define initialize NULL
static long init_record();
static long process();
#define special NULL
#define get_value NULL
#define cvt_dbaddr NULL
#define get_array_info NULL
#define put_array_info NULL
static long get_units();
static long get_precision();
#define get_enum_str NULL
#define get_enum_strs NULL
#define put_enum_str NULL
static long get_graphic_double();
static long get_control_double();
static long get_alarm_double();
static void checkAlarms(sy87739Record *prec);

rset sy87739RSET={
	RSETNUMBER,
	report,
	initialize,
	init_record,
	process,
	special,
	get_value,
	cvt_dbaddr,
	get_array_info,
	put_array_info,
	get_units,
	get_precision,
	get_enum_str,
	get_enum_strs,
	put_enum_str,
	get_graphic_double,
	get_control_double,
	get_alarm_double
};
epicsExportAddress(rset,sy87739RSET);

typedef struct sy87739dset { /* sy87739 output dset */
	long		number;
	DEVSUPFUN	dev_report;
	DEVSUPFUN	init;
	DEVSUPFUN	init_record; /*returns: (-1,0)=>(failure,success)*/
	DEVSUPFUN	get_ioint_info;
	DEVSUPFUN	write_sy87739;
}sy87739dset;

static void monitor(sy87739Record *prec);

static void rec2p(Sy87739Parms p, sy87739Record *prec)
{
	p->p    = prec->p;
	p->qp   = prec->qp;
	p->qpm1 = prec->qpm1;
	p->m    = prec->m;
	p->n    = prec->n;
	p->pp   = prec->pp;
}

static void p2rec(sy87739Record *prec, Sy87739Parms p)
{
Sy87739CtlWrd newCtlWrd;

	prec->p    = p->p;
	prec->qp   = p->qp;
	prec->qpm1 = p->qpm1;
	prec->m    = p->m;
	prec->n    = p->n;
	prec->pp   = p->pp;

	newCtlWrd = sy87739Parms2CtlWrd( p );
	if ( ! (SY87739_ERR & newCtlWrd) )
		prec->cwd = newCtlWrd;
}

static long init_record(void *precord,int pass)
{
    sy87739Record	*prec = (sy87739Record *)precord;
    sy87739dset	   *pdset = (sy87739dset *)(prec->dset);
	Sy87739CtlWrd   orig;
    long	status;
	Sy87739ParmsRec parms;

    if (pass==0) return(0);

	orig = prec->cwd;

    if( pdset && pdset->init_record ) {
		if((status=(*pdset->init_record)(prec))) return(status);
    }

	/* If a CWD value is specified in the database then
	 * override a possible HW readback
	 */

	if ( orig )
		prec->cwd = orig;

	prec->fref = 24000000;

	/* Read back from hardware if no user values present */
	if (   0 == prec->val
		&& 0 == prec->p
		&& 0 == prec->qp
		&& 0 == prec->qpm1
		&& 0 == prec->n
		&& 0 == prec->pp
		&& 0 != prec->cwd ) {
		sy87739CtlWrd2Parms( &parms, prec->cwd );
		p2rec( prec, &parms );
		prec->lp   = prec->p;
		prec->lqp  = prec->qp;
		prec->lqpm = prec->qpm1;
		prec->lm   = prec->m;
		prec->ln   = prec->n;
		prec->lpp  = prec->pp;
		prec->lcwd = prec->cwd;
		prec->val  = prec->lval = sy87739Parms2Freq( &parms, prec->fref );

		if ( prec->val != 0 )
			prec->udf = FALSE;
	}


    return(0);
}

static unsigned long freqOOR(sy87739Record *prec, unsigned long freq)
{
	if ( prec->drvh > prec->drvl ) {
		if ( freq > prec->drvh )
			return prec->drvh;
		else if ( prec->val < prec->drvl )
			return prec->drvl;
	}
	return 0;
}

static long process(void *precord)
{
	sy87739Record	*prec   = (sy87739Record *)precord;
	sy87739dset		*pdset  = (sy87739dset *)(prec->dset);
	long		     status = 0;
	unsigned char    pact=prec->pact;

	Sy87739ParmsRec  newParms;
	Sy87739Freq      newF;
	Sy87739CtlWrd    newCtlWrd;

	if ( ! prec->pact ) {
		if ( prec->val != prec->lval ) {
			if ( (newF = freqOOR(prec, prec->val)) )
				prec->val = newF;
			if ( 0 < sy87739Freq2Parms(prec->val, prec->fref, CLOSEST, &newParms, 1) ) {
				p2rec(prec, &newParms);
			} else {
				prec->val = prec->lval;
				if ( prec->val == 0 )
					recGblSetSevr(prec, UDF_ALARM, INVALID_ALARM);
			}
		} else if (   prec->p    != prec->lp
				|| prec->qp   != prec->lqp
				|| prec->qpm1 != prec->lqpm
				|| prec->m    != prec->lm
				|| prec->n    != prec->ln
				|| prec->pp   != prec->lpp ) {
				rec2p( &newParms, prec );

				if ( sy87739CheckParms( stdout, &newParms )
				    || 0 == ( newF = sy87739Parms2Freq( &newParms, prec->fref ))
					|| freqOOR(prec, newF) ) {

					/* don't restore -- we want to hold (temporarily) invalid state
					 * until user completes...
					 */
					recGblSetSevr(prec, WRITE_ALARM, INVALID_ALARM );
				} else {
					prec->val  = newF;
					newCtlWrd = sy87739Parms2CtlWrd( &newParms );
					if ( ! (SY87739_ERR & newCtlWrd) )
						prec->cwd = newCtlWrd;
					else
						recGblSetSevr(prec, WRITE_ALARM, INVALID_ALARM );
				}
		} else if ( prec->cwd != prec->lcwd ) {
			/* Control word changed */
			prec->cwd &= 0x1fffc7ff;
			sy87739CtlWrd2Parms( &newParms, prec->cwd );
			if ( 0 == sy87739CheckParms( 0, &newParms ) ) {
				newF = sy87739Parms2Freq( &newParms, prec->fref );
			} else {
				newF = 0;
			}
			if ( 0 == newF || freqOOR( prec, newF) ) {
				prec->cwd = prec->lcwd;
			} else {
				p2rec( prec, &newParms );
				prec->val = newF;
			}
		}
		prec->lval = prec->val;
	}

	/* check for alarms */
	checkAlarms(prec);

	if ( prec->nsev < INVALID_ALARM ) {

		if( (pdset==NULL) || (pdset->write_sy87739==NULL) ) {
			Sy87739ParmsRec p;

			rec2p(&p, prec);

			sy87739DumpParms(stdout, &p);
		} else {
			/* pact must not be set until after calling device support */
			status=(*pdset->write_sy87739)(prec);
		}

	}

	/* check if device support set pact */
	if ( !pact && prec->pact ) return(0);
	prec->pact = TRUE;

	recGblGetTimeStamp(prec);
	/* check event list */
	monitor(prec);
	/* process the forward scan link record */
    recGblFwdLink(prec);

	prec->pact=FALSE;
	return(status);
}

static long get_units(DBADDR *paddr, char *units)
{
    sy87739Record	*prec=(sy87739Record *)paddr->precord;

    strncpy(units,prec->egu,DB_UNITS_SIZE);
    return(0);
}

static long get_precision(DBADDR *paddr, long *precision)
{
    sy87739Record	*prec=(sy87739Record *)paddr->precord;

    *precision = prec->prec;
    if(paddr->pfield == (void *)&prec->val) return(0);
    recGblGetPrec(paddr,precision);
    return(0);
}

static long get_graphic_double(DBADDR *paddr,struct dbr_grDouble *pgd)
{
    sy87739Record	*prec=(sy87739Record *)paddr->precord;
    int		fieldIndex = dbGetFieldIndex(paddr);

    if(fieldIndex == sy87739RecordVAL
    || fieldIndex == sy87739RecordHOPR
    || fieldIndex == sy87739RecordLOPR) {
        pgd->upper_disp_limit = prec->hopr;
        pgd->lower_disp_limit = prec->lopr;
    } else recGblGetGraphicDouble(paddr,pgd);
    return(0);
}

static long get_control_double(DBADDR *paddr,struct dbr_ctrlDouble *pcd)
{
    sy87739Record	*prec=(sy87739Record *)paddr->precord;
    int		fieldIndex = dbGetFieldIndex(paddr);

    if(fieldIndex == sy87739RecordVAL){
	pcd->upper_ctrl_limit = prec->hopr;
	pcd->lower_ctrl_limit = prec->lopr;
    } else recGblGetControlDouble(paddr,pcd);
    return(0);
}

static long get_alarm_double(DBADDR *paddr,struct dbr_alDouble *pad)
{
    recGblGetAlarmDouble(paddr,pad);
    return(0);
}

static void checkAlarms(sy87739Record *prec)
{
Sy87739ParmsRec parms;
Sy87739Freq     f;

	if(prec->udf == TRUE ){
		recGblSetSevr(prec,UDF_ALARM,INVALID_ALARM);
		return;
	}

	sy87739CtlWrd2Parms( &parms, prec->cwd );

	if ( sy87739CheckParms( NULL, &parms ) ) {
		recGblSetSevr(prec,UDF_ALARM,INVALID_ALARM);
		return;
	}

	if ( 0 == (f = sy87739Parms2Freq( &parms, prec->fref )) ) {
		recGblSetSevr(prec,UDF_ALARM,INVALID_ALARM);
		return;
	}

	if ( prec->drvh > prec->drvl ) {
		if ( f > prec->drvh )
			recGblSetSevr(prec, HIHI_ALARM, MAJOR_ALARM);
		else if ( f < prec->drvl )
			recGblSetSevr(prec, LOLO_ALARM, MAJOR_ALARM);
	}
	
	return;
}

static void monitor(sy87739Record *prec)
{
	unsigned short	monitor_mask;

	monitor_mask = recGblResetAlarms(prec);
	/* check for value change */

	if ( prec->mdel < 0 || abs( prec->val - prec->mlst ) > prec->mdel ) {
		/* post events for value change */
		monitor_mask |= DBE_VALUE;
		/* update last value monitored */
		prec->mlst = prec->val;
	}

	if ( prec->adel < 0 || abs( prec->val - prec->alst ) > prec->adel ) {
		/* post events for value change */
		monitor_mask |= DBE_LOG;
		/* update last value monitored */
		prec->alst = prec->val;
	}

	/* send out monitors connected to the value field */
	if (monitor_mask){
		db_post_events(prec,&prec->val,monitor_mask);
	}

	if ( prec->p != prec->lp ) {
		db_post_events(prec, &prec->p, DBE_LOG | DBE_VALUE);
		prec->lp = prec->p;
	}
	if ( prec->qp != prec->lqp ) {
		db_post_events(prec, &prec->qp, DBE_LOG | DBE_VALUE);
		prec->lqp = prec->qp;
	}

	if ( prec->qpm1 != prec->lqpm ) {
		db_post_events(prec, &prec->qpm1, DBE_LOG | DBE_VALUE);
		prec->lqpm = prec->qpm1;
	}

	if ( prec->m != prec->lm ) {
		db_post_events(prec, &prec->m, DBE_LOG | DBE_VALUE);
		prec->lm = prec->m;
	}

	if ( prec->n != prec->ln ) {
		db_post_events(prec, &prec->n, DBE_LOG | DBE_VALUE);
		prec->ln = prec->n;
	}

	if ( prec->pp != prec->lpp ) {
		db_post_events(prec, &prec->pp, DBE_LOG | DBE_VALUE);
		prec->lpp = prec->pp;
	}

	return;
}

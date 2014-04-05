#ifdef __rtems__
#include <rtems.h>            /* required for timex.h      */
#endif
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/timex.h>        /* for ntp_adjtime           */
#include "dbAccess.h"
#include "epicsTypes.h"
#include "registryFunction.h" /* for epicsExport           */
#include "epicsExport.h"      /* for epicsRegisterFunction */
#include "longSubRecord.h"    /* for struct longSubRecord */
#include "aSubRecord.h"       /* for struct aSubRecord  */
#include "alarm.h"           

static long lsubTrigSelInit(longSubRecord *prec)
{
    if ( prec->tpro )
    	printf("lsubTrigSelInit for %s\n", prec->name);

    return 0;
}

/* Find index of first non-zero input */
static long lsubTrigSel(longSubRecord *prec)
{
	epicsUInt32    i   = 0;
	epicsUInt32    *p  = &prec->a;
	epicsUInt32    *pp = &prec->z;

	for(i=0; (p+i) <= pp; i++) {
		if(*(p+i)) {
			prec->val = i;
			if ( prec->tpro >= 2 )
				printf( "lsubTrigSel %s: Input %u non-zero\n", prec->name, i );
			return 0;
		}
	}

	if ( prec->tpro >= 2 )
		printf( "lsubTrigSel %s: No inputs non-zero!\n", prec->name );
	prec->val	= -1;
	prec->brsv	= INVALID_ALARM;
	return -1;
}

static long lsubEvSelInit(longSubRecord *prec)
{
	if ( prec->tpro )
    	printf("lsubEvSelInit for %s\n", prec->name);

    return 0;
}

static long lsubEvSel(longSubRecord *prec)
{
    epicsInt32		i	= prec->z;
    epicsUInt32	*	p	= &prec->a;

    if ( i < 0 || i >= 24 )
	{
		prec->val	= 0;
		prec->brsv	= INVALID_ALARM;
    	return -1;
	}

	prec->val = *(p+i);
	if ( prec->tpro >= 2 )
    	printf("lsubEvSel %s: Input %u is %u\n", prec->name, i, prec->val);

    return 0;
}

static long aSubEvOffsetInit(aSubRecord *prec)
{
	assert( dbValueSize(prec->fta)  == sizeof(epicsUInt32) );
	assert( dbValueSize(prec->ftb)  == sizeof(epicsUInt32) );
	assert( dbValueSize(prec->ftc)  == sizeof(epicsUInt32) );
	assert( dbValueSize(prec->ftd)  == sizeof(epicsUInt32) );
	assert( dbValueSize(prec->ftva) == sizeof(epicsUInt32) );

    return 0;
}

/*
 *   -----------------
 *   Input/Output list 
 *   -----------------
 *
 * INPA: Input for event number: event number selector (epicsUInt32 type)
 * INPB: Activate/Deactivate event invariant delay (epicsUInt32 type)
 * INPC: Input for EVG delay - lookup PV (epicsUInt32 type waveform)
 * INPD: Input for previous delay (just in case, if EVNT:SYSx:1:DELAY array is not available, invalid severity)

 * OUTA: Output for delay
 *
 */
static long aSubEvOffset(aSubRecord *prec)
{
    epicsUInt32		eventNumber		= *(epicsUInt32*)(prec->a);
    epicsUInt32		activeFlag		= *(epicsUInt32*)(prec->b);
    epicsUInt32 *	pdelayArray		=  (epicsUInt32*)(prec->c);
    epicsUInt32		defaultDelay	= *(epicsUInt32*)(prec->d);
    epicsUInt32	*	poutputDelay	=  (epicsUInt32*)(prec->vala);
    epicsEnum16		sevr;

    if(dbGetSevr(&prec->inpc, &sevr)) {
        printf("%s: CA connection serverity check error\n", prec->name);
        return 0;
    }

    if(sevr                          ||     /* record is not initialized */
       !activeFlag                   ||     /* deactivate */
       !pdelayArray                  ||     /* no lookup table */
       (eventNumber<0 || eventNumber>255)   /* out of range for event number */) {
        *poutputDelay = defaultDelay;                   /* if something is wrong, just use default delay */
    }
    else {
		*poutputDelay = *(pdelayArray + eventNumber);  /* Everything is OK, let's use look up table */

		/* Save the computed delay as the new default */
		*(epicsUInt32*)(prec->d) = *poutputDelay;
	}
    return 0;
}

static long lsubCountNonZeroInit(longSubRecord *prec)
{
    if ( prec->tpro )
		printf("lsubCountNonZeroInit for %s\n", prec->name);
    return 0;
}

static long lsubCountNonZero(longSubRecord *prec)
{
	epicsUInt32    count	= 0;
	epicsUInt32    i		= 0;
	epicsUInt32    *p		= &prec->a;
	epicsUInt32    *pp		= &prec->z;

	for ( i = 0; (p+i) <= pp; i++ )
	{
		if ( *(p+i) )
			++count;
	}

	if ( prec->tpro >= 2 )
		printf("lsubCountNonZero %s: Count is %u\n", prec->name, count );

	prec->val	= count;
	return 0;
}

#define	N_ASUB_ARGS		21
static long asubCopyInToOutInit( aSubRecord * prec )
{
    epicsUInt32		i;
    void		**	pVoidIn		= &prec->a;
    void		**	pVoidOut	= &prec->vala;
    epicsUInt32	**	pIn			= (epicsUInt32 **)pVoidIn;
    epicsUInt32	**	pOut		= (epicsUInt32 **)pVoidOut;
    epicsUInt32	*	pInCnt		= &prec->nea;
    epicsUInt32	*	pOutCnt		= &prec->noa;
    epicsEnum16	*	pInType		= &prec->fta;
    epicsEnum16	*	pOutType	= &prec->ftva;

	if ( prec->tpro )
    	printf("asubCopyInToOutInit for %s\n", prec->name);
    for ( i = 0; i < N_ASUB_ARGS; i++ )
	{
		assert( dbValueSize(*pInType)	== dbValueSize(DBF_ULONG) );
		assert( dbValueSize(*pOutType)	== dbValueSize(DBF_ULONG) );
		if ( prec->tpro >= 2 )
		{
			printf( "INP%c: pIn  = 0x%p, cnt = %u\n",
					'A' + i, pIn[i], *pInCnt );
			printf( "OUT%c: pOut = 0x%p, cnt = %u\n",
					'A' + i, pOut[i], *pOutCnt );
		}
		pInCnt++;	pOutCnt++;
		pInType++;	pOutType++;
	}

    return 0;
}

static long asubCopyInToOut( aSubRecord * prec )
{
    epicsUInt32		i;
    void		**	pVoidIn		= &prec->a;
    void		**	pVoidOut	= &prec->vala;
    epicsUInt32	**	pIn			= (epicsUInt32 **)pVoidIn;
    epicsUInt32	**	pOut		= (epicsUInt32 **)pVoidOut;
    epicsUInt32	*	pInCnt		= &prec->nea;
    epicsUInt32	*	pOutCnt		= &prec->noa;

    for ( i = 0; i < N_ASUB_ARGS; i++ )
	{
		size_t		count	= *pInCnt;
		if( count > *pOutCnt )
			count = *pOutCnt;
		if ( prec->tpro >= 2 )
			printf( "%s.OUT%c: memcpy( pOut=0x%p, pIn=0x%p, cnt=%zu ) *pIn=%u\n",
					prec->name, 'A' + i, pOut[i], pIn[i], count, *pIn[i] );
		memcpy( pOut[i], pIn[i], count );
		pInCnt++; pOutCnt++;
	}

	if ( prec->tpro >= 2 )
    	printf("asubCopyInToOutInit for %s\n", prec->name);
    return 0;
}

epicsRegisterFunction(lsubTrigSelInit);
epicsRegisterFunction(lsubTrigSel);
epicsRegisterFunction(lsubEvSelInit);
epicsRegisterFunction(lsubEvSel);
epicsRegisterFunction(aSubEvOffsetInit);
epicsRegisterFunction(aSubEvOffset);
epicsRegisterFunction(lsubCountNonZeroInit);
epicsRegisterFunction(lsubCountNonZero);
epicsRegisterFunction(asubCopyInToOutInit);
epicsRegisterFunction(asubCopyInToOut);

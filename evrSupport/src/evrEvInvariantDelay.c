#ifdef __rtems__
#include <rtems.h>            /* required for timex.h      */
#endif
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/timex.h>        /* for ntp_adjtime           */
#include "epicsTypes.h"
#include "registryFunction.h" /* for epicsExport           */
#include "epicsExport.h"      /* for epicsRegisterFunction */
#include "longSubRecord.h"    /* for struct longSubRecord  */

typedef struct {
    epicsUInt32  code;
    epicsUInt32  offset; 
} evOffset_t;


/* for LCLS */
#if     defined(LCLS)
#include "evrEvInvDelayLCLS.h"

/* for FACET */
#elif defined(FACET)
#include "evrEvInvDelayFACET.h"


/* for XTA */
#elif defined(XTA)
#include "evrEvInvDelayXTA.h"


#else
#error "Please, defind your facility in the RELEASE_SITE file. ex) SITE_FACILITY = LCLS | FACET | XTA
#endif


static long lsubTrigSelInit(longSubRecord *prec)
{
    /* printf("lsubTrigSelInit for %s\n", prec->name); */

    return 0;
}

static long lsubTrigSel(longSubRecord *prec)
{
   epicsUInt32    i   = 0;
   epicsUInt32    *p  = &prec->a;
   epicsUInt32    *pp = &prec->z;

   /* printf("lsubTrigSel for %s\n", prec->name);  */

   for(i=0; (p+i) <= pp; i++) {
       if(*(p+i)) {
           prec->val = i;
           return 0;
       }
   }

   return -1;
}

static long lsubEvSelInit(longSubRecord *prec)
{
    /* printf("lsubEvSelInit for %s\n", prec->name); */

    return 0;
}

static long lsubEvSel(longSubRecord *prec)
{
    epicsUInt32  i  = prec->v;
    epicsUInt32  *p = &prec->a;

    /* printf("lsubEvSel for %s\n", prec->name); */

    prec->val = *(p+i);

    return 0;
}

static long lsubLookupOffsetInit(longSubRecord *prec)
{
    /* printf("lsubLookupOffsetInit for %s\n", prec->name); */

    return 0;
}

static long lsubLookupOffset(longSubRecord *prec)
{

    /* printf("lsubLookupOffset for %s\n", prec->name); */

   if(prec->a>255) {         /* exception on event code */
       prec->val = prec->c;  /* just give default value */
       return 0;
   }

    if(prec->b) {    /* when activate the event code invariant delay */
        prec->val = evOffset[prec->a].offset;
        if(!prec->val) prec->val = prec->c;  /* if, offset is 0, then use default */
    }
    else  prec->val = prec->c; /* when deactivate, use default offset */

    return 0;
}



epicsRegisterFunction(lsubTrigSelInit);
epicsRegisterFunction(lsubTrigSel);
epicsRegisterFunction(lsubEvSelInit);
epicsRegisterFunction(lsubEvSel);
epicsRegisterFunction(lsubLookupOffsetInit);
epicsRegisterFunction(lsubLookupOffset);

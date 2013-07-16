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
#include "stringinRecord.h"   /* for struct stringinRecord */

#include "dbAccess.h"

typedef struct {
    epicsUInt32  code;
    char        *desc;
} evDesc_t;


/* for LCLS */
#if     defined(LCLS)
#include "evrEvDescLCLS.h"

/* for FACET */
#elif defined(FACET)
#include "evrEvDescFACET.h"


/* for XTA */
#elif defined(XTA)
#include "evrEvDescXTA.h"


#else
#error "Please, defind your facility in the RELEASE_SITE file. ex) SITE_FACILITY = LCLS | FACET | XTA
#endif


static long lsubEvDescInit(longSubRecord *prec)
{
    return 0; /* Nothing todo during initialization */
}

static long lsubEvDesc(longSubRecord *prec)
{
    struct stringinRecord *pstringin;
    DBADDR *pAddr;

     pAddr = dbGetPdbAddrFromLink(&prec->inpb);
      if(!pAddr) return 0;  /* there is no LINK. Nothing to do */
      pstringin = (stringinRecord*)pAddr->precord;


      if(prec->a >=0 && prec->a <256)
          if (evDesc[prec->a].desc) strcpy(pstringin->val, evDesc[prec->a].desc);
          else                      strcpy(pstringin->val, "NOT DEFINED");
      else
          strcpy(pstringin->val, "OUT OF RANGE");

    return 0;
}

epicsRegisterFunction(lsubEvDescInit);
epicsRegisterFunction(lsubEvDesc);

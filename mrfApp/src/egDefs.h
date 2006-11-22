/* $Id: egDefs.h,v 1.2 2006/11/02 23:24:25 saa Exp $ */
#ifndef EPICS_EGDEFS_H
#define EPICS_EGDEFS_H

#ifdef INCdevSuph
typedef struct
{
  long	number;
  DEVSUPFUN       	report;
  DEVSUPFUN             init;
  DEVSUPFUN             initRec;
  DEVSUPFUN       	get_ioint_info;
  DEVSUPFUN		proc;
} EgDsetStruct;
#endif

#define EG_SEQ_RAM_SIZE                 (1024*512)
 
#endif

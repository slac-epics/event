#define PVASSIGN(a,b) { while (1) { if (pvStatOK == pvAssign(a,b)) break; epicsThreadSleep(delay_time); } }

#define PVGET(a) { while (1) { if (pvStatOK == pvGet(a,SYNC)) break; epicsThreadSleep(delay_time); } }

#define PVPUT(a) { while (1) { if (pvStatOK == pvPut(a,SYNC)) break; epicsThreadSleep(delay_time); } }

float delay_time;

#ifdef __rtems__
#include <rtems.h>            /* required for timex.h      */
#endif
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/timex.h>        /* for ntp_adjtime           */
#include "registryFunction.h" /* for epicsExport           */
#include "epicsExport.h"      /* for epicsRegisterFunction */
#include "longSubRecord.h"    /* for struct longSubRecord  */

typedef struct {
    unsigned long  code;
    unsigned long  offset; 
} evOffset_t;

static evOffset_t evOffset[256] = {
    {  0, 0},     {  1, 0},     {  2, 0},     {  3, 0},     {  4, 0},     {  5, 0},     {  6, 0},     {  7, 0},     {  8, 0},     {  9, 12950},
    { 10, 13001}, { 11, 13011}, { 12, 13021}, { 13, 13031}, { 14, 13041}, { 15, 13051}, { 16, 13061}, { 17, 0},     { 18, 0},     { 19, 0}, 
    { 20, 13002}, { 21, 13012}, { 22, 13022}, { 23, 13032}, { 24, 13042}, { 25, 13052}, { 26, 13062}, { 27, 0},     { 28, 0},     { 29, 0},
    { 30, 13003}, { 31, 13013}, { 32, 13023}, { 33, 13033}, { 34, 13043}, { 35, 13053}, { 36, 13063}, { 37, 0},     { 38, 0},     { 39, 0},
    { 40, 13004}, { 41, 13014}, { 42, 13024}, { 43, 13034}, { 44, 13044}, { 45, 13054}, { 46, 13064}, { 47, 0},     { 48, 0},     { 49, 0},
    { 50, 13005}, { 51, 13015}, { 52, 13025}, { 53, 13035}, { 54, 13045}, { 55, 13055}, { 56, 13065}, { 57, 0},     { 58, 0},     { 59, 0},
    { 60, 13006}, { 61, 13016}, { 62, 13026}, { 63, 13036}, { 64, 13046}, { 56, 13056}, { 66, 13066}, { 67, 11967}, { 68, 11968}, { 69, 11969},
    { 70, 11970}, { 71, 11971}, { 72, 11972}, { 73, 11973}, { 74, 11974}, { 75, 11975}, { 76, 11976}, { 77, 11977}, { 78, 11978}, { 79, 11979},
    { 80, 11980}, { 81, 11981}, { 82, 11982}, { 83, 11983}, { 84, 11984}, { 85, 11985}, { 86, 11986}, { 87, 11987}, { 88, 11988}, { 89, 11989},
    { 90, 11990}, { 91, 11991}, { 92, 11992}, { 93, 11993}, { 94, 11994}, { 95, 11995}, { 96, 11996}, { 97, 11997}, { 98, 11998}, { 99, 0}, 
    {100, 0},     {101, 0},     {102, 0},     {103, 0},     {104, 0},     {105, 0},     {106, 0},     {107, 0},     {108, 0},     {109, 0},
    {110, 0},     {111, 0},     {112, 0},     {113, 0},     {114, 0},     {115, 0},     {116, 0},     {117, 0},     {118, 0},     {119, 0},
    {120, 0},     {121, 0},     {122, 0},     {123, 0},     {124, 0},     {125, 0},     {126, 0},     {127, 0},     {128, 0},     {129, 0},
    {130, 0},     {131, 0},     {132, 0},     {133, 0},     {134, 0},     {135, 0},     {136, 0},     {137, 0},     {138, 0},     {139, 0},
    {140, 11900}, {141, 11901}, {142, 11902}, {143, 11903}, {144, 11904}, {145, 11905}, {146, 11906}, {147, 11907}, {148, 11908}, {149, 11909},
    {150, 11910}, {151, 11911}, {152, 11912}, {153, 11913}, {154, 11914}, {155, 11915}, {156, 11916}, {157, 11917}, {158, 11918}, {159, 11919},
    {160, 11920}, {161, 11921}, {162, 11890}, {163, 11922}, {164, 11922}, {165, 0},     {166, 0},     {167, 12067}, {168, 12068}, {169, 12069},
    {170, 12070}, {171, 12071}, {172, 12072}, {173, 12073}, {174, 12074}, {175, 12075}, {176, 12076}, {177, 12077}, {178, 12078}, {179, 12079},
    {180, 12080}, {181, 12081}, {182, 12082}, {183, 12083}, {184, 12084}, {185, 12085}, {186, 12086}, {187, 12087}, {188, 12088}, {189, 12089},
    {190, 12090}, {191, 12091}, {192, 12092}, {193, 12093}, {194, 12094}, {195, 12095}, {196, 12096}, {197, 12097}, {198, 12098}, {199, 0},
    {200, 0},     {201, 13000}, {202, 0},     {203, 0},     {204, 0},     {205, 0},     {206, 0},     {207, 0},     {208, 0},     {209, 0},
    {210, 0},     {211, 0},     {212, 0},     {213, 0},     {214, 0},     {215, 0},     {216, 0},     {217, 0},     {218, 0},     {219, 0},
    {220, 0},     {221, 0},     {222, 0},     {223, 0},     {224, 0},     {225, 0},     {226, 0},     {227, 0},     {228, 0},     {229, 0},
    {230, 0},     {231, 0},     {232, 0},     {233, 0},     {234, 0},     {235, 0},     {236, 0},     {237, 0},     {238, 0},     {239, 0},
    {240, 0},     {241, 0},     {242, 0},     {243, 0},     {244, 0},     {245, 0},     {246, 0},     {247, 0},     {248, 0},     {249, 0},
    {250, 0},     {251, 0},     {252, 0},     {253, 0},     {254, 0},     {255, 0}    
};



static int lsubTrigSelInit(longSubRecord *prec)
{
    /* printf("lsubTrigSelInit for %s\n", prec->name); */

    return 0;
}

static int lsubTrigSel(longSubRecord *prec)
{
   unsigned long i   = 0;
   unsigned long *p  = &prec->a;
   unsigned long *pp = &prec->z;

   /* printf("lsubTrigSel for %s\n", prec->name);  */

   for(i=0; (p+i) <= pp; i++) {
       if(*(p+i)) {
           prec->val = i;
           return 0;
       }
   }

   return -1;
}

static int lsubEvSelInit(longSubRecord *prec)
{
    /* printf("lsubEvSelInit for %s\n", prec->name); */

    return 0;
}

static int lsubEvSel(longSubRecord *prec)
{
    unsigned long i  = prec->v;
    unsigned long *p = &prec->a;

    /* printf("lsubEvSel for %s\n", prec->name); */

    prec->val = *(p+i);

    return 0;
}

static int lsubLookupOffsetInit(longSubRecord *prec)
{
    printf("lsubLookupOffsetInit for %s\n", prec->name);

    return 0;
}

static int lsubLookupOffset(longSubRecord *prec)
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

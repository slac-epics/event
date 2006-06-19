/* dayle 03apr2006 SLAC */
#ifndef DRV_PMC_H
#define DRV_PMC_H


volatile unsigned long getINTCSR(void);
int getPCICRSAdrs(int Card, unsigned long baseAddr, volatile unsigned long *reg1);
int getPCIRegAdrs(int Card, unsigned long baseAddr, volatile unsigned long *reg1);
int setPCIintrvec(void* isr, uint32_t clr, uint32_t set);
int getPCIintrvec(void);
int pmcConnectInterruptToISR(void* isr);
void pmcEnableInterrupt(void);
int pmcUninstallISR(void);
#endif 


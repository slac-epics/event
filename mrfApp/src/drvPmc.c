/* Author: Dayle Kotturi <dayle@slac.stanford.edu> 2006 */

/* the ugly is that stuff in here is global. so there can be only one
   0x9030 device. future is to modify ErCardStruct so that vals are
   kept in there and struct is passed into these routines for setting.
   or something like that.. */

#ifdef __rtems__
#include <rtems.h>
#include <rtems/pci.h>
#include <rtems/irq.h>		/* for rtems_irq_connect_data */ 
#include <libcpu/io.h>
#include <bsp.h>
#include <bsp/pci.h>
#include <bsp/bspExt.h>          /* for bspExtMemProbe */
#include <debugPrint.h>          /* for DEBUGPRINT */
#else  
#include <vxWorks.h>
#include <sysLib.h>
#include <vxLib.h>
#include <pci.h>
#include <drv/pci/pciConfigLib.h>
#include <drv/pci/pciConfigShow.h>
#include <drv/pci/pciIntLib.h>
#include <intLib.h>
#include <vme.h>
#include </home/Tornado2.2/target/config/mv5500/universe.h>
#include </home/Tornado2.2/target/config/mv5500/mv5500A.h>
#include <unistd.h>
#endif

#include <stdio.h>
#include <stdlib.h>
#include <debugPrint.h>
#include "drvPmc.h"

/* From JP 4/1/2006: "We have plans to acquire our own PCI vendor id, 
   but for now these boards are using the default for the PLX PCI chip. */
#define MRF_VENDOR_ID (PCI_VENDOR_ID_PLX)

/* More notes 4/6/2006: 0x9030 is also generic PLX device. So we could
   have more of them than just the MRF PMC EVR. The way to ensure that
   the h/w is REALLY MRF, would be to check sub-vendor id and sub-device id
   stored in the EEPROM. Only they aren't there. MRF could/should burn
   them in. We can write to it and set these, in the meantime, but that
   code isn't here. See plx9080_eeprom.c in this dir */
#define MRF_DEVICE_ID (0x9030)

typedef struct PmcIsrArg_s {
  rtems_irq_connect_data   ic;
  volatile uint32_t     *addr;
  uint32_t	        set;
  uint32_t	        clr;
} PmcIsrArg_t, *PmcIsrArg;

#ifdef __rtems__
  int pciFlag = 2;    /* Sets debugging level. at 2 it dumps errors;
                              at 4 it dumps info */
#endif

/* make part of ErCardStruct? */
typedef struct myPmcEvr_s {
  unsigned short vendorid;
  unsigned short deviceid;
  int pciBusNo;
  int pciDevNo;
  int pciFuncNo;
  volatile unsigned long configRegAddr;
  volatile unsigned long regAddr;
  volatile unsigned long irqVector;
  PmcIsrArg pIsrData;
} myPmcEvr_t;

myPmcEvr_t stuff;

PmcIsrArg pmc_irq_install(void* isr, uint32_t clr, uint32_t set);

static void noop() {}
static int  noop1() {return 0;}

volatile unsigned pmc_irqs = 0;

volatile unsigned long getINTCSR() {
  return stuff.irqVector;
}


/* Finds the PCI device for the known (constant) vendor and device id
   at the given PCI address. 

   Returns a local address to the configuration register space 

   Attempts to set up the interrupt vector, too, but this part is commented
   out for now. 

 */
int getPCICRSAdrs(int Card, unsigned long baseAddr, volatile unsigned long *reg1)
{
  stuff.vendorid = MRF_VENDOR_ID;
  stuff.deviceid = MRF_DEVICE_ID;

  #ifdef __rtems__ 
    if (pci_find_device(stuff.vendorid, stuff.deviceid, Card,
      &(stuff.pciBusNo), &(stuff.pciDevNo), &(stuff.pciFuncNo)) == ERROR) {
      printf
        ("getPCIBaseAdrs: Unable to find %d PCI device %d in PMC slot %d\n",
        stuff.vendorid, stuff.deviceid, Card);
      return ERROR;
    }

    printf("getPCIBaseAdrs: Found PMC-EVR at Bus 0x%x|Device 0x%x|Function 0x%x\n",
      stuff.pciBusNo, stuff.pciDevNo, stuff.pciFuncNo);
    pci_read_config_dword((unsigned char) stuff.pciBusNo, (unsigned char) stuff.pciDevNo,
      (unsigned char) stuff.pciFuncNo, baseAddr, &(stuff.configRegAddr)); 
    printf("getPCIBaseAdrs: pci_read_config_dword of Bus 0x%x|Device 0x%x|Function 0x%x at PCI addr 0x%08x returns 0x%08x\n", stuff.pciBusNo, stuff.pciDevNo, stuff.pciFuncNo, (unsigned int) baseAddr, stuff.configRegAddr);

  #else /* assume vxWorks */
    if (pciFindDevice(stuff.vendorid, stuff.deviceid, Card, 
      &(stuff.pBusNo), &(stuff.pDevNo), &(stuff.pFuncNo)) == ERROR) {
      printf
        ("getPCIBaseAdrs: Unable to find %lu PCI device %lu in PMC slot %d\n",
        stuff.vendorid, stuff.deviceid, stuff.Card);
      return ERROR;
    } 
  
    printf("getPCIBaseAdrs: Found PMC-EVR at Bus 0x%x Device 0x%x Function 0x%x\n",
      stuff.pciBusNo, stuff.pciDevNo, stuff.pciFuncNo);
    pciConfigInLong(stuff.pBusNo, stuff.pDevNo, stuff.pFuncNo, baseAddr,
      &(stuff.configRegAddr));
  #endif

  stuff.irqVector = stuff.configRegAddr + 0x4c; /* 4/11/2006 this seems wrong 
                                                   the address of the vector
                                                   is 0xe110004C, but, at this
                                                   addr, is a short with 
                                                   contents as per PLX PCI9030
                                                   section 10.6 */
 
  printf("getPCIBaseAdrs: Setting irqVector to 0x%08x\n", 
    (unsigned int) stuff.irqVector);

  *reg1 = stuff.configRegAddr; /* later: pass struct, not reg1 */
  return OK;
}


int getPCIRegAdrs(int Card, unsigned long baseAddr, volatile unsigned long *reg1)
{
  if (stuff.configRegAddr == 0) {
     printf("getPCIRegAdrs: Call getPCICRSAdrs first!\n"); 
     return ERROR;
  }

  #ifdef __rtems__ 
    pci_read_config_dword((unsigned char) stuff.pciBusNo, (unsigned char) stuff.pciDevNo,
      (unsigned char) stuff.pciFuncNo, baseAddr, &(stuff.regAddr)); 
  #else /* assume vxWorks */
    pciConfigInLong(stuff.pBusNo, stuff.pDevNo, stuff.pFuncNo, baseAddr,
      &(stuff.regAddr));
  #endif
  printf("getPCIRegAdrs: pci_read_config_dword of Bus 0x%x|Device 0x%x|Function 0x%x at PCI addr 0x%08x returns 0x%08x\n", stuff.pciBusNo, stuff.pciDevNo, stuff.pciFuncNo, (unsigned int) baseAddr, stuff.regAddr);
  *reg1 = stuff.regAddr;
  return OK;
}


/* Sample ISR */
void pmc_isr(rtems_irq_hdl_param arg) {
uint32_t    v = in_le32(stuff.pIsrData->addr); 

	pmc_irqs++;

        /* TO DO: dissect this, make var names more meaninful, add comments */
	out_le32(stuff.irqVector, (v & ~stuff.pIsrData->clr) | stuff.pIsrData->set);
}

/* This routine will set bit 6 of the interrupt vector (base addr + 0x4c) */
void pmcEnableInterrupt() {
uint32_t    v = in_le32(stuff.pIsrData->addr);      
        out_le32(stuff.irqVector, (v | stuff.pIsrData->clr));        
}

/* Fills in the rtems_irq_hdl_param including vector, ISR, checking the pin
   and setting the line.
   NB: convention for return codes are reversed. 0 = error
 */
PmcIsrArg pmc_irq_install(void* isr, uint32_t clr, uint32_t set) {
  uint32_t      v;
  uint8_t       il;
  PmcIsrArg d = 0;

        fprintf(stderr,"pmc_irq_install: top\n");

	d = calloc(sizeof(*d),1);

	if ( !d ) {
                fprintf(stderr,"pmc_irq_install: NULL PmcIsrArg structure\n");
		return 0;
        }
        fprintf(stderr,"pmc_irq_install: PmcIsrArg ok\n");

	if ( !stuff.irqVector ) {
		fprintf(stderr,"pmc_irq_install: NULL irqVector\n");
		return 0;
	}
	fprintf(stderr,"pmc_irq_install: irqVector ok\n");

	if ( ! (clr || set) ) {
		fprintf(stderr,"pmc_irq_install: 'clr' and 'set' parameters cannot both be 0\n");
		return 0;
	}
	fprintf(stderr,"pmc_irq_install: 'clr' and 'set' parameters ok\n");

	pci_read_config_dword(stuff.pciBusNo, stuff.pciDevNo, stuff.pciFuncNo, 0 ,&v);

	if ( 0xffffffff == v ) {
		fprintf(stderr,"pmc_irq_install: No device in slot (%i/%i.%i)\n", stuff.pciBusNo, stuff.pciDevNo, stuff.pciFuncNo);
		return 0;
	}

	pci_read_config_byte(stuff.pciBusNo, stuff.pciDevNo, stuff.pciFuncNo, 
          PCI_INTERRUPT_PIN, &il);

	if ( !il ) {
		fprintf(stderr,"pmc_irq_install: Interrupt PIN is 0 -- is this device 'normal' ?\n");
		return 0;
	}
	fprintf(stderr,"pmc_irq_install: INFO: Interrupt PIN is %d\n", il);

	pci_read_config_byte(stuff.pciBusNo, stuff.pciDevNo, stuff.pciFuncNo,
          PCI_INTERRUPT_LINE, &il);

	d->ic.name   = il;
	d->ic.hdl    = isr; 
	d->ic.handle = (rtems_irq_hdl_param)d;
	d->ic.on     = noop;
        d->ic.off    = noop;
        d->ic.isOn   = noop1;
	d->addr      = stuff.irqVector;
	d->clr       = clr;
	d->set       = set;

	if ( ! BSP_install_rtems_irq_handler(&d->ic) ) {
		fprintf(stderr,"pmc_irq_install: ISR installation failed\n");
		free(d);
		return 0;
	}
	stuff.pIsrData = d;
	fprintf(stderr,"pmc_irq_install: ISR installation successful\n");
        return d;
}

int pmc_isr_uninstall(PmcIsrArg d)
{
	if ( !d )
		return 0xdeadbeef;
	if ( !BSP_remove_rtems_irq_handler(&d->ic) ) {
		fprintf(stderr,"ISR removal failed\n");
		return -1;
	}
	free(d);
	return 0;
}

/* USED FOR TESTING ONLY. 
   Not used by PMC-EVR.
   Connects IRQ vector to dummy ISR routine: pmc_isr (d->ic.hdl) */
int setPCIintrvec(void* isr, uint32_t clr, uint32_t set) {
  stuff.pIsrData->clr = clr;
  stuff.pIsrData->set = set;
  stuff.pIsrData = pmc_irq_install(isr, clr, set); 
  return OK;
}

int getPCIintrvec(void) {
  fprintf(stderr,"getPCIintrvec: Contents of PmcIsrArg structure\n");
  fprintf(stderr,"getPCIintrvec: ic\n");
  fprintf(stderr,"getPCIintrvec: interrupt vector=%lu\n", stuff.irqVector);
  fprintf(stderr,"getPCIintrvec: set=%lu\n", stuff.pIsrData->set);
  fprintf(stderr,"getPCIintrvec: clr=%lu\n", stuff.pIsrData->clr);
  return OK;
}

/* Connect IRQ vector to passed in isr.
   Ensures that interrupts are disabled, by the way. */
int pmcConnectInterruptToISR(void *isr) {
  stuff.pIsrData = pmc_irq_install(isr, 0x40, 0); 
  return OK;
}

int pmcUninstallISR() {
  int rc;
  rc = pmc_isr_uninstall(stuff.pIsrData);
  return rc;
}

/* Do I need some PCI MemProbe here? both rtems and vxworks */

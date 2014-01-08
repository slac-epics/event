/*
  ELI.c -- 
  Test setup for ELI ETS

  EVG Setup:
  - 0.5 Hz on MXC to produce EVR prescaler reset event
  -   1 Hz on MXC0 (looped externally on UNIV0 for second count)
  -  1 kHz on MXC1 (looped externally on UNIV1 for timestamp kHz fraction) 

  Author: Jukka Pietarinen (MRF)
  Date:   03.10.2012

*/

#include <endian.h>
#include <byteswap.h>
#include <errno.h>
#include <stdio.h>
#include <sys/ioctl.h>
#include <signal.h>
#include "egcpci.h"
#include "egapi.h"
#include "erapi.h"

int main(int argc, char *argv[])
{
  struct MrfEgRegs *pEg1;
  struct MrfErRegs *pEr1;
  struct MrfErRegs *pEr2;
  struct MrfErRegs *pErtg1;
  int              fdEg1;
  int              fdEr1;
  int              fdEr2;
  int              fdErtg1;
  int              i;
  char             c;

  //open evg, evr devices
  fdEg1 = EvgOpen(&pEg1, "/dev/eg3a3");
  if (fdEg1 == -1)
  {
    printf("Open evg error: %s - %s %d \n", argv[1], strerror(errno), fdEg1);
    return errno;
  }
  printf("Open evg1 '/dev/eg3a3' OK. %d \n", fdEg1);

  fdEr1 = EvrOpen(&pEr1, "/dev/er3b3");
  if (fdEr1 == -1)
  {
  	EvgClose(fdEg1);
    	printf("Open evr1 '/dev/er3b3' error: %s\n", strerror(errno));
      	return errno;
  }
  printf("Open evr1 '/dev/er3b3' OK. %d \n", fdEr1);

  fdEr2 = EvrOpen(&pEr2, "/dev/er3a3");
  if (fdEr1 == -1)
  {
  	EvgClose(fdEg1);
  	EvrClose(fdEr1);
    	printf("Open evr2 '/dev/er3a3' error: %s\n", strerror(errno));
      	return errno;
  }
  printf("Open evr2 '/dev/er3a3' OK. %d \n", fdEr2);

#ifdef EVRTG
  fdErtg1 = EvrOpen(&pErtg1, "/dev/evrtga3");
  if (fdEr1 == -1)
  {
  	EvgClose(fdEg1);
  	EvrClose(fdEr1);
  	EvrClose(fdEr2);
    	printf("Open evrtg1 '/dev/evrtga3' error: %s\n", strerror(errno));
      	return errno;
  }
  printf("Open evrtg1 '/dev/evrtga3' OK. %d \n", fdErtg1);
#endif

  int cnt = 1;

  printf("%d\n", cnt++);

  /*
  EvgSetRFInput(pEg, 1, C_EVG_RFDIV_6);
  */
//  EvgSetRFInput(pEg, 0, C_EVG_RFDIV_6);	//for generating 80MHz
//  EvgSetFracDiv(pEg, 0x0106822d);
//  EvrSetFracDiv(pEr, 0x0106822d);

  //setup event frequency as 120MHz
  EvgSetRFInput(pEg1, 0, C_EVG_RFDIV_4);		//for generating 120MHz
  EvgSetFracDiv(pEg1, 0x00820140);
  EvrSetFracDiv(pEr1, 0x00820140);
  EvrSetFracDiv(pEr2, 0x00820140);
#ifdef EVRTG
  EvrSetFracDiv(pErtg1, 0x00820140);
#endif
  sleep(2);

  printf("%d\n", cnt++);

  //enable evg and evrs
  EvgEnable(pEg1, 1);
  printf("Enabling Evg1: %08x\n", EvgGetEnable(pEg1));
  EvrEnable(pEr1, 1);
  EvrEnable(pEr2, 1);
#ifdef EVRTG
  EvrEnable(pErtg1, 1);
  printf("Enabling Evr1:%08x, Evr2:%08x, Evrtg1:%08x\n", EvrGetEnable(pEr1), EvrGetEnable(pEr2), EvrGetEnable(pErtg1));
#endif

  //check signal violation
  EvrGetViolation(pEr1, 1);
  if (EvrGetViolation(pEr1, 0))
  {
  	printf("Could not clear Evr1 violation flag.\n");
  }
  EvrGetViolation(pEr2, 1);
  if (EvrGetViolation(pEr2, 0))
  {
  	printf("Could not clear Evr2 violation flag.\n");
  }
#ifdef EVRTG
  EvrGetViolation(pErtg1, 1);
  if (EvrGetViolation(pErtg1, 0))
  {
  	printf("Could not clear Evrtg1 violation flag.\n");
  }
#endif

  //allow timestamp on evrs
  EvrMapRamEnable(pEr1, 1, 1);
  EvrSetTimestampDivider(pEr1, 0);
  pEr1->Control |= be32_to_cpu(1 << C_EVR_CTRL_TS_CLOCK_DBUS);
  
  EvrMapRamEnable(pEr2, 1, 1);
  EvrSetTimestampDivider(pEr2, 0);
  pEr2->Control |= be32_to_cpu(1 << C_EVR_CTRL_TS_CLOCK_DBUS);
  
#ifdef EVRTG
  EvrMapRamEnable(pErtg1, 1, 1);
  EvrSetTimestampDivider(pErtg1, 0);
  pErtg1->Control |= be32_to_cpu(1 << C_EVR_CTRL_TS_CLOCK_DBUS);
#endif

  printf("%d\n", cnt++);

  //set up 1Hz and 1KHz on distributed bus for time stamp increamenting
  EvgEvanResetCount(pEg1);
  printf("Setting up multiplexed couters\n");
  /* 1 kHz on MXC0 */
  EvgSetMXCPrescaler(pEg1, 0, 120000);
  /* 1 Hz on MXC1 */
  EvgSetMXCPrescaler(pEg1, 1, 120000000);
  
  /* 0.5 Hz on MXC2 */
  EvgSetMXCPrescaler(pEg1, 2, 240000000);
  EvgSyncMxc(pEg1);
  EvgSetTriggerEvent(pEg1, 2, 0x7b, 1);						//for synchronise the Prescalers in EVRs
  EvgSetMxcTrigMap(pEg1, 2, 2);
  
  EvgMXCDump(pEg1);
  EvgSetDBusEvent(pEg1, 1 << 5);
  EvgSetDBusMap(pEg1, 0, C_EVG_DBUS_SEL_MXC);
  EvgSetDBusMap(pEg1, 1, C_EVG_DBUS_SEL_MXC);
  EvgSetDBusMap(pEg1, 4, C_EVG_DBUS_SEL_EXT);
  EvgSetDBusMap(pEg1, 5, C_EVG_DBUS_SEL_EXT);
  printf("DBus Event Enable %08x\n", EvgGetDBusEvent(pEg1));
  /* Map 1 kHz to UnivOut0 */
  EvgSetUnivOutMap(pEg1, 0, C_SIGNAL_MAP_PRESC);
  /* Map 1 Hz to UnivOut1 */
  EvgSetUnivOutMap(pEg1, 1, C_SIGNAL_MAP_PRESC+1);
  /* Map 1 kHz to UnivOut2 */
  EvgSetUnivOutMap(pEg1, 2, C_SIGNAL_MAP_PRESC);				//this seems no needed
  /* Map 1 Hz to UnivOut3 */
  EvgSetUnivOutMap(pEg1, 3, C_SIGNAL_MAP_PRESC+1);				//this seems no needed
  /* Input UnivIn1 (1 Hz) to Ext. DBUS5 */
  EvgSetUnivinMap(pEg1, 1, -1, 5, -1, -1);
  /* Input UnivIn0 (1 kHz) to Ext. DBUS4 */
  EvgSetUnivinMap(pEg1, 0, -1, 4, -1, -1);
  EvgTimestampLoad(pEg1, 0);
  EvgTimestampEnable(pEg1, 1);
  if (EvgGetTimestampEnable(pEg1) != 1)
    printf("Could not enable Timestamp generator\n");

  printf("%d\n", cnt++);

  //***********set up evr1 (EVR-300) pulse generator, delay, width etc.*********************
  //
  // Channel  module           frequency
  // 0        TTL plugin        1kHz
  // 1        TTL plugin        1kHz
  // 2        TTL plugin        1kHz
  // 3        TTL plugin        1kHz
  // 8        TTL DLY plugin    1kHz
  // 9        TTL  DLY plugin   1kHz
  // 10       TTL DLY plugin    2kHz
  // 11       TTL DLY plugin    2kHz
  //
  //******************************************************************************
  EvrSetPrescalerPolarity(pEr1, 1);

  EvrSetPrescaler(pEr1, 0, 240000000);	//0.5Hz
  EvrSetPrescaler(pEr1, 1, 120000000);	//1Hz
  EvrSetPrescaler(pEr1, 2,  12000000);	//10Hz
  EvrSetPrescaler(pEr1, 3,   1200000);	//100Hz
  EvrSetPrescaler(pEr1, 4,    120000);	//1kHz
  EvrSetPrescaler(pEr1, 5,     60000);	//2kHz
  EvrSetPrescaler(pEr1, 6,      1200);	//100kHz
  EvrSetPrescaler(pEr1, 7,         3);	//40MHz

//  EvrSetPrescalerTrig(pEr1, 0, 0x3F);
//  EvrSetPrescalerTrig(pEr1, 1, 2);
//  EvrSetPrescalerTrig(pEr1, 2, 4);
//  EvrSetPrescalerTrig(pEr1, 3, 8);
  EvrSetPrescalerTrig(pEr1, 4, 0x3F);
  EvrSetPrescalerTrig(pEr1, 5, 0xC0);
//  EvrSetPrescalerTrig(pEr1, 6, 64);
//  EvrSetPrescalerTrig(pEr1, 7, 128);

  EvrSetPulseParams(pEr1, 0, 1, 0, 10000);
  EvrSetPulseParams(pEr1, 1, 1, 1000, 10000);
  EvrSetPulseParams(pEr1, 2, 1, 1000, 10000);
  EvrSetPulseParams(pEr1, 3, 1, 0, 10000);
  EvrSetPulseParams(pEr1, 4, 1, 0, 10000);
  EvrSetPulseParams(pEr1, 5, 1, 0, 10000);
  EvrSetPulseParams(pEr1, 6, 1, 0, 10000);
  EvrSetPulseParams(pEr1, 7, 1, 0, 1000);

  EvrSetPulseProperties(pEr1, 0, 0, 0, 0, 1, 1);
  EvrSetPulseProperties(pEr1, 1, 0, 0, 0, 1, 1);
  EvrSetPulseProperties(pEr1, 2, 0, 0, 0, 1, 1);
  EvrSetPulseProperties(pEr1, 3, 0, 0, 0, 1, 1);
  EvrSetPulseProperties(pEr1, 4, 0, 0, 0, 1, 1);
  EvrSetPulseProperties(pEr1, 5, 0, 0, 0, 1, 1);
  EvrSetPulseProperties(pEr1, 6, 0, 0, 0, 1, 1);
  EvrSetPulseProperties(pEr1, 7, 0, 0, 0, 1, 1);

  //evr1 trigger mapping
  EvrSetUnivOutMap(pEr1, 0, 0);
  EvrSetUnivOutMap(pEr1, 1, 1);
  EvrSetUnivOutMap(pEr1, 2, 2);
  EvrSetUnivOutMap(pEr1, 3, 3);
//  EvrSetUnivOutMap(pEr1, 4, 4);
//  EvrSetUnivOutMap(pEr1, 5, 5);
//  EvrSetUnivOutMap(pEr1, 6, 6);
//  EvrSetUnivOutMap(pEr1, 7, 7);
  EvrSetUnivOutMap(pEr1, 8, 4);
  EvrSetUnivOutMap(pEr1, 9, 5);
  EvrSetUnivOutMap(pEr1, 10, 6);
  EvrSetUnivOutMap(pEr1, 11, 7);

  EvrUnivDlyEnable(pEr1, 0, 1);
  EvrUnivDlyEnable(pEr1, 1, 1);
  EvrUnivDlySetDelay(pEr1, 0, 0, 0);
  EvrUnivDlySetDelay(pEr1, 1, 0, 1000);

  EvrSetLedEvent(pEr1, 1, 0x7B, 1);


  //***********set up evr2 (EVR-300) pulse generator, delay, width etc.*********************
  //
  // Channel   module                frequency
  // 0         TTL plugin            1Hz
  // 1         TTL plugin            1Hz
  // 2         TTL plugin            10Hz
  // 3         TTL plugin            10Hz
  // 4         TTL plugin            100Hz
  // 5         TTL plugin            100Hz
  // 10        TTL DLY plugin        2kHz
  // 11        TTL DLY plugin        2kHz
  //
  //******************************************************************************
  EvrSetPrescalerPolarity(pEr2, 1);

  EvrSetPrescaler(pEr2, 0, 240000000);	//0.5Hz
  EvrSetPrescaler(pEr2, 1, 120000000);	//1Hz
  EvrSetPrescaler(pEr2, 2,  12000000);	//10Hz
  EvrSetPrescaler(pEr2, 3,   1200000);	//100Hz
  EvrSetPrescaler(pEr2, 4,    120000);	//1kHz
  EvrSetPrescaler(pEr2, 5,     60000);	//2kHz
  EvrSetPrescaler(pEr2, 6,      1200);	//100kHz
  EvrSetPrescaler(pEr2, 7,         3);	//40MHz

//  EvrSetPrescalerTrig(pEr2, 0, 1);
  EvrSetPrescalerTrig(pEr2, 1, 0x3);
  EvrSetPrescalerTrig(pEr2, 2, 0xC);
  EvrSetPrescalerTrig(pEr2, 3, 0x30);
//  EvrSetPrescalerTrig(pEr2, 4, 16);
  EvrSetPrescalerTrig(pEr2, 5, 0xC0);
//  EvrSetPrescalerTrig(pEr2, 6, 64);
//  EvrSetPrescalerTrig(pEr2, 7, 128);

  EvrSetPulseParams(pEr2, 0, 1, 0, 10000);
  EvrSetPulseParams(pEr2, 1, 1, 0, 10000);
  EvrSetPulseParams(pEr2, 2, 1, 0, 10000);
  EvrSetPulseParams(pEr2, 3, 1, 0, 10000);
  EvrSetPulseParams(pEr2, 4, 1, 0, 10000);
  EvrSetPulseParams(pEr2, 5, 1, 0, 10000);
  EvrSetPulseParams(pEr2, 6, 1, 0, 100);
  EvrSetPulseParams(pEr2, 7, 1, 0, 1);

  EvrSetPulseProperties(pEr2, 0, 0, 0, 0, 1, 1);
  EvrSetPulseProperties(pEr2, 1, 0, 0, 0, 1, 1);
  EvrSetPulseProperties(pEr2, 2, 0, 0, 0, 1, 1);
  EvrSetPulseProperties(pEr2, 3, 0, 0, 0, 1, 1);
  EvrSetPulseProperties(pEr2, 4, 0, 0, 0, 1, 1);
  EvrSetPulseProperties(pEr2, 5, 0, 0, 0, 1, 1);
  EvrSetPulseProperties(pEr2, 6, 0, 0, 0, 1, 1);
  EvrSetPulseProperties(pEr2, 7, 0, 0, 0, 1, 1);

  //evr2 trigger mapping
  EvrSetUnivOutMap(pEr2, 0, 0);
  EvrSetUnivOutMap(pEr2, 1, 1);
  EvrSetUnivOutMap(pEr2, 2, 2);
  EvrSetUnivOutMap(pEr2, 3, 3);
  EvrSetUnivOutMap(pEr2, 4, 4);
  EvrSetUnivOutMap(pEr2, 5, 5);
//  EvrSetUnivOutMap(pEr2, 6, 6);
//  EvrSetUnivOutMap(pEr2, 7, 7);
//  EvrSetUnivOutMap(pEr2, 8, 0);
//  EvrSetUnivOutMap(pEr2, 9, 1);
  EvrSetUnivOutMap(pEr2, 10, 6);
  EvrSetUnivOutMap(pEr2, 11, 7);

  EvrUnivDlyEnable(pEr2, 0, 1);
  EvrUnivDlyEnable(pEr2, 1, 1);
  EvrUnivDlySetDelay(pEr2, 0, 0, 0);
  EvrUnivDlySetDelay(pEr2, 1, 0, 0);
 
  EvrSetLedEvent(pEr2, 1, 0x7B, 1);




  //***********set up evrtg1 pulse generator, delay, width etc.*********************
  //
  // Channel   module                frequency
  // 0         TTL plugin            1kHz
  // 1         TTL plugin            1kHz
  // 2         TTL plugin            100kHz
  // 3         TTL plugin            100kHz
  // 4         LVPEC                 10Hz
  // 5         LVPEC                 1kHz
  // 6         SFP                   1kHz
  // 7         SFP                   40MHz
  //
  //******************************************************************************
#ifdef EVRTG
  EvrSetPrescalerPolarity(pErtg1, 1);

  EvrSetPrescaler(pErtg1, 0, 240000000);	//0.5Hz
  EvrSetPrescaler(pErtg1, 1, 120000000);	//1Hz
  EvrSetPrescaler(pErtg1, 2,  12000000);	//10Hz
  EvrSetPrescaler(pErtg1, 3,   1200000);	//100Hz
  EvrSetPrescaler(pErtg1, 4,    120000);	//1kHz
  EvrSetPrescaler(pErtg1, 5,     60000);	//2kHz
  EvrSetPrescaler(pErtg1, 6,      1200);	//100kHz
  EvrSetPrescaler(pErtg1, 7,         3);	//40MHz

//  EvrSetPrescalerTrig(pErtg1, 0, 1);
// EvrSetPrescalerTrig(pErtg1, 1, 2);
  EvrSetPrescalerTrig(pErtg1, 2, 0x10);
//  EvrSetPrescalerTrig(pErtg1, 3, 8);
  EvrSetPrescalerTrig(pErtg1, 4, 0x63);
//  EvrSetPrescalerTrig(pErtg1, 5, 32);
  EvrSetPrescalerTrig(pErtg1, 6, 0xC);
  EvrSetPrescalerTrig(pErtg1, 7, 0x80);

  EvrSetPulseParams(pErtg1, 0, 1, 0, 10000);
  EvrSetPulseParams(pErtg1, 1, 1, 0, 10000);
  EvrSetPulseParams(pErtg1, 2, 1, 0, 100);
  EvrSetPulseParams(pErtg1, 3, 1, 0, 100);
  EvrSetPulseParams(pErtg1, 4, 1, 0, 10000);
  EvrSetPulseParams(pErtg1, 5, 1, 0, 10000);
  EvrSetPulseParams(pErtg1, 6, 1, 0, 10000);
  EvrSetPulseParams(pErtg1, 7, 1, 0, 1);

  EvrSetPulseProperties(pErtg1, 0, 0, 0, 0, 1, 1);
  EvrSetPulseProperties(pErtg1, 1, 0, 0, 0, 1, 1);
  EvrSetPulseProperties(pErtg1, 2, 0, 0, 0, 1, 1);
  EvrSetPulseProperties(pErtg1, 3, 0, 0, 0, 1, 1);
  EvrSetPulseProperties(pErtg1, 4, 0, 0, 0, 1, 1);
  EvrSetPulseProperties(pErtg1, 5, 0, 0, 0, 1, 1);
  EvrSetPulseProperties(pErtg1, 6, 0, 0, 0, 1, 1);
  EvrSetPulseProperties(pErtg1, 7, 0, 0, 0, 1, 1);

  /*
  EvrSetUnivOutMap(pEr, 0, C_EVR_SIGNAL_MAP_PRESC);
  EvrSetUnivOutMap(pEr, 1, C_EVR_SIGNAL_MAP_PRESC+1);
  EvrSetUnivOutMap(pEr, 2, C_EVR_SIGNAL_MAP_PRESC+2);
  EvrSetUnivOutMap(pEr, 3, C_EVR_SIGNAL_MAP_PRESC+3);
  EvrSetUnivOutMap(pEr, 4, C_EVR_SIGNAL_MAP_PRESC+4);
  EvrSetUnivOutMap(pEr, 5, C_EVR_SIGNAL_MAP_PRESC+5);
  EvrSetUnivOutMap(pEr, 6, C_EVR_SIGNAL_MAP_PRESC+6);
  EvrSetUnivOutMap(pEr, 7, C_EVR_SIGNAL_MAP_PRESC+7);
  */


  /* These are for the EVRTG */
  EvrCMLEnable(pErtg1, 0, 1);
  EvrCMLEnable(pErtg1, 1, 1);
  EvrCMLEnable(pErtg1, 2, 1);
  EvrCMLEnable(pErtg1, 3, 1);
  EvrCMLEnable(pErtg1, 4, 1);
  EvrCMLEnable(pErtg1, 5, 1);
  EvrCMLEnable(pErtg1, 6, 1);
  EvrCMLEnable(pErtg1, 7, 1);
  EvrOutputEnable(pErtg1, 1);
  EvrSetFPOutMap(pErtg1, 0, 0);
  EvrSetFPOutMap(pErtg1, 1, 1);
  EvrSetFPOutMap(pErtg1, 2, 2);
  EvrSetFPOutMap(pErtg1, 3, 3);
  EvrSetFPOutMap(pErtg1, 4, 4);
  EvrSetFPOutMap(pErtg1, 5, 5);
  EvrSetFPOutMap(pErtg1, 6, 6);
  EvrSetFPOutMap(pErtg1, 7, 7);

  EvrSetLedEvent(pErtg1, 1, 0x7B, 1);
#endif



  printf("%d\n", cnt++);

  for (i = 0; i < 5; i++)
    {
      sleep(1);
      printf("Timestamp %d, %08x\n", EvgTimestampGet(pEg1),
	     be32_to_cpu(pEg1->Status));
      printf("Evr1 Timestamp %d\n", EvrGetSecondsCounter(pEr1));
      printf("Evr2 Timestamp %d\n", EvrGetSecondsCounter(pEr2));
#ifdef EVRTG
      printf("Evrtg1 Timestamp %d\n", EvrGetSecondsCounter(pErtg1));
#endif
    }
  EvgTimestampLoad(pEg1, 999999);
  for (i = 0; i < 5; i++)
    {
      sleep(1);
      printf("Timestamp %d, %08x\n", EvgTimestampGet(pEg1),
	     be32_to_cpu(pEg1->Status));
      printf("Evr1 Timestamp %d\n", EvrGetSecondsCounter(pEr1));
      printf("Evr2 Timestamp %d\n", EvrGetSecondsCounter(pEr2));
#ifdef EVRTG
      printf("Evrtg1 Timestamp %d\n", EvrGetSecondsCounter(pErtg1));
#endif
    }

  EvgTimestampEnable(pEg1, 0);
  if (EvgGetTimestampEnable(pEg1) != 0)
    printf("Could not disable Timestamp generator\n");

  /*
  EvgEnable(pEg1, 0);
  printf("Disabling Evg1: %08x\n", EvgGetEnable(pEg1));
  EvrEnable(pEr1, 0);
  printf("Disabling Evr1: %08x\n", EvrGetEnable(pEr1));
  */

  EvgClose(fdEg1);
  EvrClose(fdEr1);
  EvrClose(fdEr2);
#ifdef EVRTG
  EvrClose(fdErtg1);
#endif
}

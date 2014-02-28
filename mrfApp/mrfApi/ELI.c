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
  struct MrfEgRegs *pEg;
  struct MrfErRegs *pEr;
  int              fdEg;
  int              fdEr;
  int              i;
  char             c;

  if (argc < 1)
    {
      printf("Usage: %s /dev/ega3 /dev/era3\n", argv[0]);
      printf("Assuming: /dev/ega3 /dev/era3\n");
      argv[1] = "/dev/ega3";
    }

  if (argc < 2)
    {
      argv[2] = "/dev/era3";
    }

  fdEg = EvgOpen(&pEg, argv[1]);
  if (fdEg == -1)
    return errno;

  fdEr = EvrOpen(&pEr, argv[2]);
  if (fdEr == -1)
    {
      EvgClose(fdEg);
      return errno;
    }

  int cnt = 1;

  printf("%d\n", cnt++);

  /*
  EvgSetRFInput(pEg, 1, C_EVG_RFDIV_6);
  */
  EvgSetRFInput(pEg, 0, C_EVG_RFDIV_6);
  EvgSetFracDiv(pEg, 0x0106822d);
  EvrSetFracDiv(pEr, 0x0106822d);
  sleep(2);

  printf("%d\n", cnt++);

  EvgEnable(pEg, 1);
  printf("Enabling Evg: %08x\n", EvgGetEnable(pEg));

  EvrEnable(pEr, 1);
  printf("Enabling Evr: %08x\n", EvrGetEnable(pEr));
  EvrGetViolation(pEr, 1);
  if (EvrGetViolation(pEr, 0))
    {
      printf("Could not clear Evr violation flag.\n");
    }
  EvrMapRamEnable(pEr, 1, 1);
  EvrSetTimestampDivider(pEr, 0);
  pEr->Control |= be32_to_cpu(1 << C_EVR_CTRL_TS_CLOCK_DBUS);

  printf("%d\n", cnt++);

  EvgEvanResetCount(pEg);
  printf("Setting up multiplexed couters\n");
  /* 1 kHz on MXC0 */
  EvgSetMXCPrescaler(pEg, 0, 80000);
  /* 1 Hz on MXC1 */
  EvgSetMXCPrescaler(pEg, 1, 80000000);
  /* 0.5 Hz on MXC2 */
  EvgSetMXCPrescaler(pEg, 2, 160000000);
  EvgSyncMxc(pEg);
  EvgSetTriggerEvent(pEg, 2, 0x7b, 1);
  EvgSetMxcTrigMap(pEg, 2, 2);
  EvgMXCDump(pEg);
  EvgSetDBusEvent(pEg, 1 << 5);
  EvgSetDBusMap(pEg, 0, C_EVG_DBUS_SEL_MXC);
  EvgSetDBusMap(pEg, 1, C_EVG_DBUS_SEL_MXC);
  EvgSetDBusMap(pEg, 4, C_EVG_DBUS_SEL_EXT);
  EvgSetDBusMap(pEg, 5, C_EVG_DBUS_SEL_EXT);
  printf("DBus Event Enable %08x\n", EvgGetDBusEvent(pEg));
  /* Map 1 kHz to UnivOut0 */
  EvgSetUnivOutMap(pEg, 0, C_SIGNAL_MAP_PRESC);
  /* Map 1 Hz to UnivOut1 */
  EvgSetUnivOutMap(pEg, 1, C_SIGNAL_MAP_PRESC+1);
  /* Map 1 kHz to UnivOut2 */
  EvgSetUnivOutMap(pEg, 2, C_SIGNAL_MAP_PRESC);
  /* Map 1 Hz to UnivOut3 */
  EvgSetUnivOutMap(pEg, 3, C_SIGNAL_MAP_PRESC+1);
  /* Input UnivIn1 (1 Hz) to Ext. DBUS5 */
  EvgSetUnivinMap(pEg, 1, -1, 5, -1, -1);
  /* Input UnivIn0 (1 kHz) to Ext. DBUS4 */
  EvgSetUnivinMap(pEg, 0, -1, 4, -1, -1);
  EvgTimestampLoad(pEg, 0);
  EvgTimestampEnable(pEg, 1);
  if (EvgGetTimestampEnable(pEg) != 1)
    printf("Could not enable Timestamp generator\n");

  printf("%d\n", cnt++);

  EvrSetPrescalerPolarity(pEr, 1);

  EvrSetPrescaler(pEr, 0, 160000000);
  EvrSetPrescaler(pEr, 1,  80000000);
  EvrSetPrescaler(pEr, 2,   8000000);
  EvrSetPrescaler(pEr, 3,    800000);
  EvrSetPrescaler(pEr, 4,     80000);
  EvrSetPrescaler(pEr, 5,     40000);
  EvrSetPrescaler(pEr, 6,       800);
  EvrSetPrescaler(pEr, 7,       400);

  EvrSetPrescalerTrig(pEr, 0, 1);
  EvrSetPrescalerTrig(pEr, 1, 2);
  EvrSetPrescalerTrig(pEr, 2, 4);
  EvrSetPrescalerTrig(pEr, 3, 8);
  EvrSetPrescalerTrig(pEr, 4, 16);
  EvrSetPrescalerTrig(pEr, 5, 32);
  EvrSetPrescalerTrig(pEr, 6, 64);
  EvrSetPrescalerTrig(pEr, 7, 128);

  EvrSetPulseParams(pEr, 0, 0, 0, 1000);
  EvrSetPulseParams(pEr, 1, 0, 0, 1000);
  EvrSetPulseParams(pEr, 2, 0, 0, 1000);
  EvrSetPulseParams(pEr, 3, 0, 0, 1000);
  EvrSetPulseParams(pEr, 4, 0, 0, 1000);
  EvrSetPulseParams(pEr, 5, 0, 0, 1000);
  EvrSetPulseParams(pEr, 6, 0, 0, 100);
  EvrSetPulseParams(pEr, 7, 0, 0, 100);

  EvrSetPulseProperties(pEr, 0, 0, 0, 0, 1, 1);
  EvrSetPulseProperties(pEr, 1, 0, 0, 0, 1, 1);
  EvrSetPulseProperties(pEr, 2, 0, 0, 0, 1, 1);
  EvrSetPulseProperties(pEr, 3, 0, 0, 0, 1, 1);
  EvrSetPulseProperties(pEr, 4, 0, 0, 0, 1, 1);
  EvrSetPulseProperties(pEr, 5, 0, 0, 0, 1, 1);
  EvrSetPulseProperties(pEr, 6, 0, 0, 0, 1, 1);
  EvrSetPulseProperties(pEr, 7, 0, 0, 0, 1, 1);

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

  EvrSetUnivOutMap(pEr, 0, 0);
  EvrSetUnivOutMap(pEr, 1, 1);
  EvrSetUnivOutMap(pEr, 2, 2);
  EvrSetUnivOutMap(pEr, 3, 3);
  EvrSetUnivOutMap(pEr, 4, 4);
  EvrSetUnivOutMap(pEr, 5, 5);
  EvrSetUnivOutMap(pEr, 6, 6);
  EvrSetUnivOutMap(pEr, 7, 7);
  EvrSetUnivOutMap(pEr, 8, C_EVR_SIGNAL_MAP_PRESC+3);
  EvrSetUnivOutMap(pEr, 9, C_EVR_SIGNAL_MAP_PRESC+3);
  EvrSetUnivOutMap(pEr, 10, C_EVR_SIGNAL_MAP_PRESC+3);
  EvrSetUnivOutMap(pEr, 11, C_EVR_SIGNAL_MAP_PRESC+3);

  EvrUnivDlyEnable(pEr, 0, 1);
  EvrUnivDlyEnable(pEr, 1, 1);
  EvrUnivDlySetDelay(pEr, 0, 0, 0);
  EvrUnivDlySetDelay(pEr, 1, 0, 0);
 
  /* These are for the EVRTG */
  EvrCMLEnable(pEr, 0, 1);
  EvrCMLEnable(pEr, 1, 1);
  EvrCMLEnable(pEr, 2, 1);
  EvrCMLEnable(pEr, 3, 1);
  EvrCMLEnable(pEr, 4, 1);
  EvrCMLEnable(pEr, 5, 1);
  EvrCMLEnable(pEr, 6, 1);
  EvrCMLEnable(pEr, 7, 1);
  EvrOutputEnable(pEr, 1);
  EvrSetFPOutMap(pEr, 0, 0);
  EvrSetFPOutMap(pEr, 1, 1);
  EvrSetFPOutMap(pEr, 2, 2);
  EvrSetFPOutMap(pEr, 3, 3);
  EvrSetFPOutMap(pEr, 4, 4);
  EvrSetFPOutMap(pEr, 5, 5);
  EvrSetFPOutMap(pEr, 6, 6);
  EvrSetFPOutMap(pEr, 7, 7);

  for (i = 0; i < 5; i++)
    {
      sleep(1);
      printf("Timestamp %d, %08x\n", EvgTimestampGet(pEg),
	     be32_to_cpu(pEg->Status));
      printf("Evr Timestamp %d\n", EvrGetSecondsCounter(pEr));
    }
  EvgTimestampLoad(pEg, 999999);
  for (i = 0; i < 5; i++)
    {
      sleep(1);
      printf("Timestamp %d, %08x\n", EvgTimestampGet(pEg),
	     be32_to_cpu(pEg->Status));
      printf("Evr Timestamp %d\n", EvrGetSecondsCounter(pEr));
    }

  EvgTimestampEnable(pEg, 0);
  if (EvgGetTimestampEnable(pEg) != 0)
    printf("Could not disable Timestamp generator\n");

  /*
  EvgEnable(pEg, 0);
  printf("Disabling Evg: %08x\n", EvgGetEnable(pEg));
  EvrEnable(pEr, 0);
  printf("Disabling Evr: %08x\n", EvrGetEnable(pEr));
  */

  i = 1;
  do
    {
      EvrUnivDlySetDelay(pEr, 0, i, i);
      EvrUnivDlySetDelay(pEr, 1, i, i);
      printf("%d\n", i);
      c = getchar();
      if (c == '+')
	i <<= 1;
      if (c == '-')
	i >>= 1;
      if (i > 512)
	i = 512;
      if (i <= 0)
	i = 1;
    }
  while (c != '.');

  while (1)
    for (i = 0; i < 1024; i++)
      {
	EvrUnivDlySetDelay(pEr, 0, i, i);
	EvrUnivDlySetDelay(pEr, 1, i, i);
	printf("%d\n", i);	
	usleep(10000);
      }

  EvgClose(fdEg);
  EvrClose(fdEr);
}

/*
  timestampgen.c -- Micro-Research Event Generator
  Application Programming Interface Test Application

  Author: Jukka Pietarinen (MRF)
  Date:   05.12.2006

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

  EvgSetRFInput(pEg, 0, C_EVG_RFDIV_8);

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

  EvrSetUnivOutMap(pEr, 0, C_EVR_SIGNAL_MAP_DBUS);
  EvrSetUnivOutMap(pEr, 1, C_EVR_SIGNAL_MAP_DBUS+1);

  EvgEvanResetCount(pEg);
  printf("Setting up multiplexed couters\n");
  /* 1 MHz on MXC0 */
  EvgSetMXCPrescaler(pEg, 0, 125);
  /* 1 Hz on MXC1 */
  EvgSetMXCPrescaler(pEg, 1, 125000000);
  EvgSyncMxc(pEg);
  EvgMXCDump(pEg);
  EvgSetDBusEvent(pEg, 1 << 5);
  EvgSetDBusMap(pEg, 0, C_EVG_DBUS_SEL_MXC);
  EvgSetDBusMap(pEg, 1, C_EVG_DBUS_SEL_MXC);
  EvgSetDBusMap(pEg, 4, C_EVG_DBUS_SEL_EXT);
  EvgSetDBusMap(pEg, 5, C_EVG_DBUS_SEL_EXT);
  printf("DBus Event Enable %08x\n", EvgGetDBusEvent(pEg));
  /* Map 1 MHz to UnivOut0 */
  EvgSetUnivOutMap(pEg, 0, C_SIGNAL_MAP_PRESC);
  /* Map 1 Hz to UnivOut1 */
  EvgSetUnivOutMap(pEg, 1, C_SIGNAL_MAP_PRESC+1);
  /* Map 1 MHz to UnivOut2 */
  EvgSetUnivOutMap(pEg, 2, C_SIGNAL_MAP_PRESC);
  /* Map 1 Hz to UnivOut3 */
  EvgSetUnivOutMap(pEg, 3, C_SIGNAL_MAP_PRESC+1);
  /* Input UnivIn1 (1 Hz) to Ext. DBUS5 */
  EvgSetUnivinMap(pEg, 1, -1, 5, -1, -1);
  /* Input UnivIn0 (1 MHz) to Ext. DBUS4 */
  EvgSetUnivinMap(pEg, 0, -1, 4, -1, -1);
  EvgTimestampLoad(pEg, 0);
  EvgTimestampEnable(pEg, 1);
  if (EvgGetTimestampEnable(pEg) != 1)
    printf("Could not enable Timestamp generator\n");
  for (i = 0; i < 5; i++)
    {
      sleep(1);
      printf("Timestamp %d, %08x\n", EvgTimestampGet(pEg),
	     be32_to_cpu(pEg->Status));
      printf("Evr Timestamp %08x\n", EvrGetSecondsCounter(pEr));
    }
  EvgTimestampLoad(pEg, 999999);
  for (i = 0; i < 5; i++)
    {
      sleep(1);
      printf("Timestamp %d, %08x\n", EvgTimestampGet(pEg),
	     be32_to_cpu(pEg->Status));
      printf("Evr Timestamp %08x\n", EvrGetSecondsCounter(pEr));
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
  EvgClose(fdEg);
  EvrClose(fdEr);
}

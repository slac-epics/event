/*
  apitest.c -- Micro-Research Event Generator
               Application Programming Interface Test Application

  Author: Jukka Pietarinen (MRF)
  Date:   05.12.2006

*/

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

  if (argc < 2)
    {
      printf("Usage: %s /dev/ega3 /dev/era3\n", argv[0]);
      printf("Assuming: /dev/ega3 and /dev/era3\n");
      argv[1] = "/dev/ega3";
      argv[2] = "/dev/era3";
    }

  fdEg = EvgOpen(&pEg, argv[1]);
  if (fdEg == -1)
    return errno;

  EvgSetRFInput(pEg, 0, C_EVG_RFDIV_8);

  fdEr = EvrOpen(&pEr, argv[2]);
  if (fdEr == -1)
    {
      EvgClose(fdEg);
      return errno;
    }

  EvrSetPulseParams(pEr, 0, 1, 0, 10000000);
  EvrEnable(pEr, 1);
  printf("Enabling Evr: %08x\n", EvrGetEnable(pEr));
  printf("Violation: %08x\n", EvrGetViolation(pEr, 1));
  printf("Violation after clearing: %08x\n", EvrGetViolation(pEr, 0));

  for (i = 1; i < 11; i++)
    {
      EvrSetLedEvent(pEr, 0, i, 1);
      EvrSetPulseMap(pEr, 0, i, i-1, -1, -1);
      EvrSetPulseParams(pEr, i-1, 1, 0, 5000000);
      EvrSetPulseProperties(pEr, i-1, 0, 0, 0, 1, 1);
      EvrSetUnivOutMap(pEr, i-1, i-1);
    }

  EvrDumpMapRam(pEr, 0);
  EvrMapRamEnable(pEr, 0, 1);
  EvrDumpStatus(pEr);
  EvrDumpPulses(pEr, 10);
  EvrDumpUnivOutMap(pEr, 10);

  EvgEnable(pEg, 1);
  printf("Enabling Evg: %08x\n", EvgGetEnable(pEg));

  EvgEvanResetCount(pEg);
  printf("Setting up multiplexed couters\n");
  EvgSetMXCPrescaler(pEg, 3, 0x08000000);
  EvgSyncMxc(pEg);
  EvgMXCDump(pEg);
  EvgSetDBusMap(pEg, 3, C_EVG_DBUS_SEL_MXC);
  EvgDBusDump(pEg);
  
  EvrSetUnivOutMap(pEr, 3, C_EVR_SIGNAL_MAP_DBUS+3);

  /* Write some RAM events */
  EvgSetSeqRamEvent(pEg, 0, 0, 0, 60);
  EvgSetSeqRamEvent(pEg, 0, 1, 0x03dfffff, 61);
  EvgSetSeqRamEvent(pEg, 0, 2, 0x03efffff, 62);
  EvgSetSeqRamEvent(pEg, 0, 3, 0x03ffffff, 63);
  EvgSetSeqRamEvent(pEg, 0, 4, 0x04000000, 0x7f);
  EvgSeqRamStatus(pEg, 0);
  EvgSeqRamDump(pEg, 0);
  EvgSeqRamControl(pEg, 0, 1, 0, 0, 0, C_EVG_SEQTRIG_MXC_BASE+3);
  EvgSeqRamStatus(pEg, 0);

  /*  EvgEvanReset(pEg);
      EvgEvanEnable(pEg, 1); */

  printf("Press\nR - disable & reset sequence\n"
	 "E - Enable sequence\nD - dump log\n. Exit\n");
  do
    {
      c = getchar();

      switch (toupper(c))
	{
	case 'R':
	  EvgSeqRamControl(pEg, 0, 0, 0, 0, 1, C_EVG_SEQTRIG_MXC_BASE+3);
	  break;
	case 'E':
	  EvgSeqRamControl(pEg, 0, 1, 0, 0, 0, C_EVG_SEQTRIG_MXC_BASE+3);
	  break;
	case 'D':
	  /*	  EvgEvanDump(pEg); */
	  break;
	}
    }    
  while (c != '.');

  /*  EvgEvanEnable(pEg, 0); */
  EvgSeqRamControl(pEg, 0, 0, 0, 0, 1, 0);

  EvrEnable(pEr, 0);
  printf("Disabling Evr: %08x\n", EvrGetEnable(pEr));
  EvgEnable(pEg, 0);
  printf("Disabling Evg: %08x\n", EvgGetEnable(pEg));
  EvrClose(fdEr);
  EvgClose(fdEg);
}

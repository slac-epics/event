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

  printf("Press any key.\n");
  getchar();

  for (i = 1; i < 11; i++)
    {
      printf("Press any key. %d\n", i);
      getchar();
      EvrSetLedEvent(pEr, 0, i, 1);
      EvrSetPulseMap(pEr, 0, i, i-1, -1, -1);
      printf("Press any key.\n");
      getchar();
      EvrSetPulseParams(pEr, i-1, 1, 0, 5000000);
      printf("Press any key.\n");
      getchar();
      EvrSetPulseProperties(pEr, i-1, 0, 0, 0, 1, 1);
      printf("Press any key.\n");
      getchar();
      EvrSetUnivOutMap(pEr, i-1, i-1);
      printf("Press any key.\n");
      getchar();
    }

  printf("Press any key. EvrDumpMapRam next.\n");
  getchar();

  EvrDumpMapRam(pEr, 0);
  printf("Press any key.\n");
  getchar();

  EvrMapRamEnable(pEr, 0, 1);
  printf("Press any key.\n");
  getchar();
  EvrDumpStatus(pEr);
  printf("Press any key.\n");
  getchar();
  EvrDumpPulses(pEr, 10);
  printf("Press any key.\n");
  getchar();
  EvrDumpUnivOutMap(pEr, 10);

  EvgEnable(pEg, 1);
  printf("Enabling Evg: %08x\n", EvgGetEnable(pEg));

  printf("Press any key.\n");
  getchar();

  EvgSWEventEnable(pEg, 1);
  printf("Enabling Evg SW Event: %d\n", EvgGetSWEventEnable(pEg));
  printf("Sending out 10 software events\n");
  for (i = 1; i < 11; i++)
    {
      EvgSendSWEvent(pEg, i);
      sleep(1);
    }
  EvgSWEventEnable(pEg, 0);
  printf("Disabling Evg SW Event: %d\n", EvgGetSWEventEnable(pEg));

  printf("Press ENTER to continue...");
  getchar();

  printf("Passing through EVG Univ0-3 -> DBUS -> EVR Univ0-3\n");

  EvgSetUnivinMap(pEg, 0, -1, 0, 0, 0);
  EvgSetUnivinMap(pEg, 1, -1, 1, 0, 0);
  EvgSetUnivinMap(pEg, 2, -1, 2, 0, 0);
  EvgSetUnivinMap(pEg, 3, -1, 3, 0, 0);
  EvgSetDBusMap(pEg, 0, C_EVG_DBUS_SEL_EXT);
  EvgSetDBusMap(pEg, 1, C_EVG_DBUS_SEL_EXT);
  EvgSetDBusMap(pEg, 2, C_EVG_DBUS_SEL_EXT);
  EvgSetDBusMap(pEg, 3, C_EVG_DBUS_SEL_EXT);

  EvrSetUnivOutMap(pEr, 0, C_EVR_SIGNAL_MAP_DBUS);
  EvrSetUnivOutMap(pEr, 1, C_EVR_SIGNAL_MAP_DBUS+1);
  EvrSetUnivOutMap(pEr, 2, C_EVR_SIGNAL_MAP_DBUS+2);
  EvrSetUnivOutMap(pEr, 3, C_EVR_SIGNAL_MAP_DBUS+3);

  printf("Press ENTER to continue...");
  getchar();

  printf("Setting up multiplexed couters\n");
  EvgSetMXCPrescaler(pEg, 0, 448/8); /* Storage Ring */
  EvgSetMXCPrescaler(pEg, 1, 416/8); /* Booster Ring */
  EvgSetMXCPrescaler(pEg, 7, 13*448/8); /* Coincidence Clock */
  EvgSyncMxc(pEg);
  EvgMXCDump(pEg);
  EvgSetDBusMap(pEg, 0, C_EVG_DBUS_SEL_MXC);
  EvgSetDBusMap(pEg, 1, C_EVG_DBUS_SEL_MXC);
  EvgSetDBusMap(pEg, 7, C_EVG_DBUS_SEL_MXC);
  EvgDBusDump(pEg);
  
  EvrSetUnivOutMap(pEr, 0, C_EVR_SIGNAL_MAP_DBUS);
  EvrSetUnivOutMap(pEr, 1, C_EVR_SIGNAL_MAP_DBUS+1);
  EvrSetUnivOutMap(pEr, 2, C_EVR_SIGNAL_MAP_DBUS+7);

  printf("Press ENTER to continue...");
  getchar();

  printf("Enabling AC input\n");
  /* Set AC trigger for 1 Hz, sync. to COIC, trigger 3 on AC trigger */
  EvgSetACInput(pEg, 0, 1, 50, 0);
  EvgSetACMap(pEg, C_EVG_ACMAP_TRIG_BASE+3);
  EvgACDump(pEg);
  EvgSetTriggerEvent(pEg, 3, 0x10, 1);
  EvgTriggerEventDump(pEg);

  EvrSetLedEvent(pEr, 0, 0x10, 1);
  EvrSetPulseMap(pEr, 0, 0x10, 3, -1, -1);
  EvrSetPulseParams(pEr, 3, 1, 0, 500);
  EvrSetPulseProperties(pEr, 3, 0, 0, 0, 1, 1);
  EvrSetUnivOutMap(pEr, 3, 3);

  printf("Press ENTER to continue...");
  getchar();

  printf("Setting up sequence ram 0 to send out events at rate trig/50.\n");
  printf("Apply 50 Hz TTL signal to EVG TRIG input.\n");

  EvgSetMXCPrescaler(pEg, 2, 499654000/(8*50)); /* 50 Hz for testing */
  EvgSetDBusMap(pEg, 2, C_EVG_DBUS_SEL_MXC);    /* Put on DBUS */
  EvrSetUnivOutMap(pEr, 2, C_EVR_SIGNAL_MAP_DBUS+2); /* Map to EVR UNIVOUT2 */
  EvgSyncMxc(pEg);

  /* Write some RAM events */
  EvgSetSeqRamEvent(pEg, 0, 0, 10, 1);
  EvgSetSeqRamEvent(pEg, 0, 1, 20, 2);
  EvgSetSeqRamEvent(pEg, 0, 2, 30, 3);
  EvgSetSeqRamEvent(pEg, 0, 3, 31, 4);
  EvgSetSeqRamEvent(pEg, 0, 4, 32, 5);
  EvgSetSeqRamEvent(pEg, 0, 5, 33, 6);
  EvgSetSeqRamEvent(pEg, 0, 6, 34, 7);
  EvgSetSeqRamEvent(pEg, 0, 7, 40, 0x7f);
  EvgSeqRamStatus(pEg, 0);
  EvgSeqRamDump(pEg, 0);
  EvgSeqRamControl(pEg, 0, 1, 0, 0, 0, C_EVG_SEQTRIG_ACINPUT);
  /*  EvgSeqRamControl(pEg, 0, 1, 1, 0, 0, C_EVG_SEQTRIG_SWTRIGGER0); */
  EvgSeqRamStatus(pEg, 0);
  /*  EvgSeqRamSWTrig(pEg, 0);
      EvgSeqRamStatus(pEg, 0); */

  printf("Press ENTER to continue...");
  getchar();

  EvgSeqRamControl(pEg, 0, 0, 0, 0, 1, 0);

#ifdef USE_EXTRF
  printf("Changing over to ext. RF\n");
  EvgSetRFInput(pEg, 1, C_EVG_RFDIV_8);

  sleep(1);
  EvrGetViolation(pEr, 1);
  printf("Violation after clearing: %08x\n", EvrGetViolation(pEr, 0));

  printf("Press ENTER to continue...");
  getchar();
#endif

  EvrEnable(pEr, 0);
  printf("Disabling Evr: %08x\n", EvrGetEnable(pEr));
  EvgEnable(pEg, 0);
  printf("Disabling Evg: %08x\n", EvgGetEnable(pEg));
  EvrClose(fdEr);
  EvgClose(fdEg);
}

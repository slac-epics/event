/*
  dbusclk.c -- Micro-Research Event Generator
               Application Programming Interface Test Application

  Author: Jukka Pietarinen (MRF)
  Date:   05.08.2013

*/

#include <errno.h>
#include <stdio.h>
#include <sys/ioctl.h>
#include <signal.h>
#include "egapi.h"

#define USE_EXTERNAL_REFERENCE 0

int main(int argc, char *argv[])
{
  struct MrfEgRegs *pEg;
  int              fdEg;
  int              i;
  char             c;

  if (argc < 2)
    {
      printf("Usage: %s /dev/ega3\n", argv[0]);
      printf("Assuming: /dev/ega3\n");
      argv[1] = "/dev/ega3";
    }

  fdEg = EvgOpen(&pEg, argv[1]);
  if (fdEg == -1)
    return errno;

  /* Set reference to 499.654 MHz/4 */
  EvgSetFracDiv(pEg, 0x0C928166);
  /* Allow clock to lock */
  sleep(1);
	      
#if USE_EXTERNAL_REFERENCE
  /* Select external reference/4 */
  EvgSetRFInput(pEg, 1, C_EVG_RFDIV_4);
#else
  /* Select internal reference */
  EvgSetRFInput(pEg, 0, C_EVG_RFDIV_4);
#endif

  EvgEnable(pEg, 1);
  printf("Enabling Evg: %08x\n", EvgGetEnable(pEg));

  /* Set up Booster revolution clock on Multiplexed Counter 0 */
  EvgSetMXCPrescaler(pEg, 0, 264/4);

  /* Set up Storage ring revolution clock on Multiplexed Counter 1 */
  EvgSetMXCPrescaler(pEg, 1, 936/4);

  /* Set up Coincidence clock on Multiplexed Counter 7 */
  EvgSetMXCPrescaler(pEg, 7, 39*66);

  /* Sync multiplexed counters */
  EvgSyncMxc(pEg);

  EvgMXCDump(pEg);

  EvgSetDBusMap(pEg, 0, C_EVG_DBUS_SEL_MXC);
  EvgSetDBusMap(pEg, 1, C_EVG_DBUS_SEL_MXC);
  EvgSetDBusMap(pEg, 7, C_EVG_DBUS_SEL_MXC);
  EvgDBusDump(pEg);
  
  EvgClose(fdEg);
}

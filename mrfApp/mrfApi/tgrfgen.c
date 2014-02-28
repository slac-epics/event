/*
  tgrfgen.c -- Micro-Research Event Generator
  Application Programming Interface Test Application

  Author: Jukka Pietarinen (MRF)
  Date:   1.12.2011

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
  struct MrfErRegs *pEr;
  int              fdEr;
  int              i;
  char             c;

  if (argc < 2)
    {
      printf("Usage: %s /dev/evrtga3\n", argv[0]);
      printf("Assuming: /dev/evrtga3\n");
      argv[1] = "/dev/evrtga3";
    }

  fdEr = EvrTgOpen(&pEr, argv[1]);
  if (fdEr == -1)
    {
      printf("Cannot open device.\n");
      return errno;
    }

  EvrSetFracDiv(pEr, 0x03358185);
  sleep(1);

  /* Clear violation flag */
  EvrGetViolation(pEr, 1);

  usleep(1000);

  if (EvrGetViolation(pEr, 1))
    {
      printf("Could not clear violation flag!\n");
    }

  for(i = 0; i < 6; i++)
    {
      pEr->GTXMem[i][3] = 0xff;
      pEr->GTXMem[i][4] = 0xc0;
      pEr->GTXMem[i][5] = 0x0f;
      pEr->GTXMem[i][6] = 0xfc;
      pEr->GTXMem[i][7] = 0x00;
      EvrSetCMLMode(pEr, i, 0);
      EvrCMLEnable(pEr, i, 1);
    }

  EvrEnable(pEr, 1);
  printf("Enabling Evr: %08x\n", EvrGetEnable(pEr));

  EvrOutputEnable(pEr, 1);

  EvrClose(fdEr);
}

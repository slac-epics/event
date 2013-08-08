/*
  evrpresc.c -- Micro-Research Event Receiver
                Enable Prescaler output on UNIVOUT 0-3

  Author: Jukka Pietarinen (MRF)
  Date:   11.12.2006

*/

#include <errno.h>
#include <stdio.h>
#include "erapi.h"

int main(int argc, char *argv[])
{
  struct MrfErRegs *pEr;
  int              fdEr;

  if (argc < 2)
    {
      printf("Usage: %s /dev/era3\n", argv[0]);
      printf("Assuming: /dev/era3\n");
      argv[1] = "/dev/era3";
    }

  fdEr = EvrOpen(&pEr, argv[1]);
  if (fdEr == -1)
    {
      return errno;
    }

  EvrEnable(pEr, 1);
  printf("Enabling Evr: %08x\n", EvrGetEnable(pEr));
  
  EvrSetUnivOutMap(pEr, 0, C_EVR_SIGNAL_MAP_PRESC);
  EvrSetUnivOutMap(pEr, 1, C_EVR_SIGNAL_MAP_PRESC+1);
  EvrSetUnivOutMap(pEr, 2, C_EVR_SIGNAL_MAP_PRESC+2);
  EvrSetUnivOutMap(pEr, 3, C_EVR_SIGNAL_MAP_PRESC+3);

  EvrClose(fdEr);
}

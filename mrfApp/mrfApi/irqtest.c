/*
  apitest.c -- Micro-Research Event Receiver 
               Interrupt Test Application

  Author: Jukka Pietarinen (MRF)
  Date:   11.01.2007

*/

#include <errno.h>
#include <stdio.h>
#include "egcpci.h"
#include "egapi.h"
#include "erapi.h"

struct MrfEgRegs *pEg;
struct MrfErRegs *pEr;
int              fdEg;
int              fdEr;

void user_sig_handler(int parm)
{
  if (EvrGetIrqFlags(pEr) & EVR_IRQFLAG_HEARTBEAT)
    {
      printf("Heartbeat Irq\n");
      EvrClearIrqFlags(pEr, EVR_IRQFLAG_HEARTBEAT);
      EvrIrqHandled(fdEr);
    }
}

int main(int argc, char *argv[])
{
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

  EvrIrqAssignHandler(pEr, fdEr, &user_sig_handler);
  EvrIrqEnable(pEr, EVR_IRQ_MASTER_ENABLE | EVR_IRQFLAG_HEARTBEAT);
  /* Here we call sleep a number of times because sleep is interrupted
     when signal (user mode interrupt) is received. */
  sleep(10);
  sleep(10);
  sleep(10);
  sleep(10);
  sleep(10);
  sleep(10);
  sleep(10);
  sleep(10);
  sleep(10);
  sleep(10);
  EvrIrqEnable(pEr, 0);

  EvrEnable(pEr, 0);
  printf("Disabling Evr: %08x\n", EvrGetEnable(pEr));
  EvgEnable(pEg, 0);
  printf("Disabling Evg: %08x\n", EvgGetEnable(pEg));
  EvrClose(fdEr);
  EvgClose(fdEg);
}

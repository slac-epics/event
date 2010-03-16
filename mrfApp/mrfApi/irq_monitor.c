/*
  irq_monitor.c -- Micro-Research Event Receiver 
                   Interrupt Monitor Application

  Author: Jukka Pietarinen (MRF)
  Date:   11.01.2007

*/

#include <errno.h>
#include <stdio.h>
#include <time.h>
#include "erapi.h"

struct MrfErRegs *pEr;
int              fdEr;

void user_sig_handler(int parm)
{
  time_t t;

  t = time(NULL);
  printf("%s", asctime(localtime(&t)));

  if (EvrGetIrqFlags(pEr) & EVR_IRQFLAG_PULSE)
    {
      printf("Pulse Irq\n");
      EvrClearIrqFlags(pEr, EVR_IRQFLAG_PULSE);
    }
  if (EvrGetIrqFlags(pEr) & EVR_IRQFLAG_HEARTBEAT)
    {
      printf("Heartbeat Irq\n");
      EvrClearIrqFlags(pEr, EVR_IRQFLAG_HEARTBEAT);
    }
  if (EvrGetIrqFlags(pEr) & EVR_IRQFLAG_VIOLATION)
    {
      printf("Violation Irq\n");
      EvrClearIrqFlags(pEr, EVR_IRQFLAG_VIOLATION);
    }
  EvrIrqHandled(fdEr);
}

int main(int argc, char *argv[])
{
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

  EvrIrqAssignHandler(pEr, fdEr, &user_sig_handler);
  EvrIrqEnable(pEr, EVR_IRQ_MASTER_ENABLE | EVR_IRQFLAG_PULSE |
	       EVR_IRQFLAG_HEARTBEAT | EVR_IRQFLAG_VIOLATION);
  printf("Press 'q' + ENTER to exit...\n");
  while(getchar() != 'q')
  	;

  EvrIrqEnable(pEr, 0);

  EvrClose(fdEr);

  return 0;
}

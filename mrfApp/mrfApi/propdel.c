/*
  factest.c -- Micro-Research Event Generator and Event Receiver
               Fiber propagation delay test

  Author: Jukka Pietarinen (MRF)
  Date:   01.02.2008

  Module Setup:

  PXI-EVG-230: TTLOUT modules in UNIV slots
  PXI-EVR-230: TTLOUT modules in UNIV slots
  Fibers:      EVG TX -> EVR RX, EVR TX -> EVG RX
  Lemos:       EVR UNIV0 -> scope
               EVR UNIV2 -> EVR IN0
               EVR UNIV1 -> scope
*/

#include <errno.h>
#include <stdio.h>
#include <time.h>
#include <sys/ioctl.h>
#include <signal.h>
#include <unistd.h>
#include "egcpci.h"
#include "egapi.h"
#include "erapi.h"

#define EVENTS 100000
#define ERROR_TEXT "*** :"

int measloop(struct MrfEgRegs *pEg, struct MrfErRegs *pEr)
{
  int i, j;

  EvrEnable(pEr, 1);
  if (!EvrGetEnable(pEr))
    {
      printf(ERROR_TEXT "Could not enable EVR!\n");
      return -1;
    }

  EvgEnable(pEg, 1);
  if (!EvgGetEnable(pEg))
    {
      printf(ERROR_TEXT "Could not enable EVG!\n");
      return -1;
    }

  EvgSetDBusMap(pEg, 0, 0);

  /* Set event clock to 499.654/4 MHz */
  /*
  EvgSetFracDiv(pEg, 0x0C928166);
  EvrSetFracDiv(pEr, 0x0C928166);

  sleep(1);
  */

  /* Clear violation flag */
  EvrGetViolation(pEr, 1);
  EvgRxEnable(pEg, 1);
  EvgGetViolation(pEg, 1);

  usleep(1000);

  if (EvrGetViolation(pEr, 1))
    {
      printf(ERROR_TEXT "Could not clear EVR violation flag!\n");
    }

  if (EvgGetViolation(pEg, 1))
    {
      /*      printf(ERROR_TEXT "Could not clear EVG violation flag!\n"); */
    }

  /* Setup EVG MXC0 to output 5 kHz, output on UNIV0 and UNIV1 */

  EvgSetMXCPrescaler(pEg, 0, 499654/4/5);
  EvgSetUnivOutMap(pEg, 0, C_SIGNAL_MAP_PRESC);
  EvgSetUnivOutMap(pEg, 1, C_SIGNAL_MAP_PRESC);
  EvgSyncMxc(pEg);
  EvgSWEventEnable(pEg, 1);
  EvgSetMxcTrigMap(pEg, 0, 0);
  EvgSetTriggerEvent(pEg, 0, 2, 1);

  EvrSetUnivOutMap(pEr, 2, C_EVR_SIGNAL_MAP_LOW);

  /* Setup EVR map RAM */

  EvrSetPrescaler(pEr, 0, 499654/(4*5*2));
  EvrSetUnivOutMap(pEr, 3, C_EVR_SIGNAL_MAP_PRESC);
  EvrSetPulseMap(pEr, 0, 1, 1, -1, -1);
  EvrSetPulseParams(pEr, 1, 1, 0, 1000);
  EvrSetPulseProperties(pEr, 1, 0, 0, 0, 1, 1);
  EvrSetUnivOutMap(pEr, 1, 1);
  EvrSetLedEvent(pEr, 0, 2, 1);
  EvrSetPulseMap(pEr, 0, 2, 0, -1, -1);
  EvrSetPulseParams(pEr, 0, 1, 0, 1000);
  EvrSetPulseProperties(pEr, 0, 0, 0, 0, 1, 1);
  EvrSetUnivOutMap(pEr, 0, 0);
  EvrSetFPOutMap(pEr, 0, 0);
  EvrSetExtEvent(pEr, 0, 1, 1, 0);
  EvrSetBackEvent(pEr, 0, 2, 1, 0);
  EvrSetFIFOEvent(pEr, 0, 1, 1);
  EvrSetFIFOEvent(pEr, 0, 2, 1);
  EvrSetTimestampDivider(pEr, 1);

  /* Reset Prescalers */
  EvgSendSWEvent(pEg, 0x07b);

  EvrMapRamEnable(pEr, 0, 1);

  EvrClearFIFO(pEr);

  /* Reset event counter */
  EvgSendSWEvent(pEg, 0x7d);
  EvgSendSWEvent(pEg, 0x7c);

  EvrSetUnivOutMap(pEr, 2, C_EVR_SIGNAL_MAP_PRESC);

  {
    struct FIFOEvent fe[EVENTS*2];
    double diff, min, max, ave;

    for (i = 0; i < EVENTS*2; i++)
      {
        while (EvrGetFIFOEvent(pEr, &fe[i]));
      }
    
    ave = 0.0;
    min = 9999999;
    max = 0;
    for (i = 0; i < EVENTS*2; i+=2)
      {
	/*
	printf("%d\t%08x\t%d\t%08x\t%d\n",
	       fe[i].EventCode, fe[i].TimestampLow,
	       fe[i+1].EventCode, fe[i+1].TimestampLow,
	       fe[i+1].TimestampLow - fe[i].TimestampLow);
	*/
	diff = fe[i+1].TimestampLow - fe[i].TimestampLow;
	ave += diff;
	if (diff < min) min = diff;
	if (diff > max) max = diff;
      }
    printf("%f\t%f\t%f\t%f ", min, max,
	   ave/EVENTS, (ave/EVENTS)*(1E9/(499654000.0/4.0)));
  }

  return 0;
}

int main(int argc, char *argv[])
{
  struct MrfEgRegs *pEg;
  struct MrfErRegs *pEr;
  int              fdEg;
  int              fdEr;
  int              i,j;

  if (argc < 3)
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

  /*  for (i = 0; i < 20; i++) */
    measloop(pEg, pEr);

  EvrClose(fdEr);
  EvgClose(fdEg);
}

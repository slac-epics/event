/*
  apitest.c -- Micro-Research Event Generator
			   Application Programming Interface Test Application

  Author: Jukka Pietarinen (MRF)
  Date:   05.12.2006

*/

#include <errno.h>
#include <stdio.h>
#include <unistd.h>
#include "egcpci.h"
#include "egapi.h"

#define EVG_RF_FREQ 499.654E6L
#define EVG_RF_DIVIDER 8.0L

int main(int argc, char *argv[])
{
  struct MrfEgRegs *pEg;
  struct EvanStruct evan;
  int			   fdEg;

  if (argc < 2)
	{
	  printf("Usage: %s /dev/era3\n", argv[0]);
	  return -1;
	}

  fdEg = EvgOpen(&pEg, argv[1]);
  if (fdEg < 0)
	{
	  printf("EvgOpen returned %d, errno %d\n", fdEg, errno);
	  return errno;
	}

  EvgEvanReset(pEg);
  EvgEvanResetCount(pEg);
  EvgEvanEnable(pEg, 1);
  while (1)
	{
	  while (!EvgEvanGetEvent(pEg, &evan))
		{
		  unsigned long long ts;
		  long double sec;

		  ts = ((long long) evan.TimestampHigh << 32) +
			(long long) evan.TimestampLow;
		  sec = ts / (EVG_RF_FREQ/EVG_RF_DIVIDER);
		  printf("Timestamp %08lx%08lx, %16.9Lf, event %02lx, dbus %02lx\n",
				 evan.TimestampHigh, evan.TimestampLow, sec, 
				 evan.EventCode & 0x00ff, evan.EventCode >> 8);
		}
	  /* Sleep for a while if there are no events in the event analyzer fifo */
	  sleep(1);
	}

  EvgEvanEnable(pEg, 0);
  EvgClose(fdEg);
}

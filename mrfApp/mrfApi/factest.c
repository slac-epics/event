/*
  factest.c -- Micro-Research Event Generator and Event Receiver
               Factory Test

  Author: Jukka Pietarinen (MRF)
  Date:   08.10.2007

  Module Setup:

  PXI-EVG-230: UNIVIO loopback on SBS connector
               TTLOUT loopback modules in UNIV slots
  PXI-EVR-230: UNIVIO loopback on SBS connector
               LVPECL-DLY loopback modules in UNIV slots
  Fibers:      EVG TX -> EVR RX, EVR TX -> 2nd EVR RX
  Lemos:       EVG UNIV0 -> EVG TRIG
               EVG UNIV2 -> EVR IN0
               EVG UNIV3 -> EVR IN1
               RF -> EVG RF

  PMC-EVR-230: EVG UNIV2 -> EVR EXT.IN
*/

#include <errno.h>
#include <stdio.h>
#include <time.h>
#include <sys/ioctl.h>
#include <signal.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include "egcpci.h"
#include "egapi.h"
#include "erapi.h"

int err_gen;
int err_vio;
int err_pulse;
int err_evan;
int err_mxc;
int err_dbus;
int err_acin;
int err_seq;
int err_dbuf;
int err_irq;
int err_fifo;
int err_extev;
int err_bev;
int err_bdbus;
int err_bdbuf;
int err_fwd;

void clear_errors()
{
  err_gen = 0;
  err_vio = 0;
  err_pulse = 0;
  err_evan = 0;
  err_mxc = 0;
  err_dbus = 0;
  err_acin = 0;
  err_seq = 0;
  err_dbuf = 0;
  err_irq = 0;
  err_fifo = 0;
  err_extev = 0;
  err_bev = 0;
  err_bdbus = 0;
  err_bdbuf = 0;
  err_fwd = 0;
}

void dump_errors()
{
  printf("General errors   %d\n", err_gen);
  printf("Violation errors %d\n", err_vio);
  printf("Pulse errors     %d\n", err_pulse);
  printf("Evan errors      %d\n", err_evan);
  printf("MXC errors       %d\n", err_mxc);
  printf("DBUS errors      %d\n", err_dbus);
  printf("ACIN errors      %d\n", err_acin);
  printf("SeqRAM errors    %d\n", err_seq);
  printf("Databuf errors   %d\n", err_dbuf);
  printf("IRQ errors       %d\n", err_irq);
  printf("FIFO errors      %d\n", err_fifo);
  printf("Ext. errors      %d\n", err_extev);
  printf("B. event errors  %d\n", err_bev);
  printf("B. DBUS errors   %d\n", err_bdbus);
  printf("BD Buf errors    %d\n", err_bdbuf);
  printf("Fwd event errors %d\n", err_fwd);
}

int evr_irq_cnt[C_EVR_NUM_IRQ];

void evr_clr_irq()
{
  int i;

  for (i = 0; i < C_EVR_NUM_IRQ; i++)
    evr_irq_cnt[i] = 0;
}

struct MrfErRegs *pIrqEr;
int              fdIrqEr;

void evr_irq_handler(int param)
{
  int flags, i;

  flags = EvrGetIrqFlags(pIrqEr);

  /*
  time_t t;
  t = time(NULL);
  printf("IRQ %08x %s", flags, asctime(localtime(&t)));
  */

  for (i = 0; i < C_EVR_NUM_IRQ; i++)
    {
      if (flags & (1 << i))
	{
	  evr_irq_cnt[i]++;
	  EvrClearIrqFlags(pIrqEr, (1 << i));
	}
    }
  EvrIrqHandled(fdIrqEr);
}

#define ERROR_TEXT "*** "

int testloop(struct MrfEgRegs *pEg, struct MrfErRegs *pEr, struct MrfErRegs *pBEr)
{
  int i, j, k, ff;

  EvrEnable(pEr, 1);
  if (!EvrGetEnable(pEr))
    {
      printf(ERROR_TEXT "Could not enable EVR!\n");
      err_gen++;
      return -1;
    }

  EvgEnable(pEg, 1);
  if (!EvgGetEnable(pEg))
    {
      printf(ERROR_TEXT "Could not enable EVG!\n");
      err_gen++;
      return -1;
    }

  EvrEnable(pBEr, 1);
  if (!EvrGetEnable(pBEr))
    {
      printf(ERROR_TEXT "Could not enable backward EVR!\n");
      err_gen++;
      return -1;
    }

  ff = EvrGetFormFactor(pEr);
  switch (ff)
    {
    case 0: printf("Testing cPCI-EVR-230\n");
      break;
    case 1: printf("Testing PMC-EVR-230\n");
      break;
    }

  EvgSetDBusMap(pEg, 0, 0);

  /* Set event clock to 499.654/4 MHz */
  EvgSetFracDiv(pEg, 0x0C928166);
  EvrSetFracDiv(pEr, 0x0C928166);
  EvrSetFracDiv(pBEr, 0x0C928166);
  /* Set event clock to 499.654/8 MHz */
  /*
  EvgSetFracDiv(pEg, 0x0C9282A6);
  EvrSetFracDiv(pEr, 0x0C9282A6);
  EvrSetFracDiv(pBEr, 0x0C9282A6);
  */

  sleep(1);

  /* Clear violation flag */
  EvrGetViolation(pEr, 1);
  EvrGetViolation(pBEr, 1);

  usleep(1000);

  if (EvrGetViolation(pEr, 1))
    {
      err_vio++;
      printf(ERROR_TEXT "Could not clear violation flag!\n");
    }

  if (EvrGetViolation(pBEr, 1))
    {
      err_vio++;
      printf(ERROR_TEXT "Could not clear violation flag on backward EVR!\n");
    }

  /* Setup EVR map RAM */

  for (i = 1; i < 11; i++)
    {
      EvrSetLedEvent(pEr, 0, i, 1);
      EvrSetPulseMap(pEr, 0, i, i-1, -1, -1);
      EvrSetPulseParams(pEr, i-1, 1, 0, 100);
      EvrSetPulseProperties(pEr, i-1, 0, 0, 0, 1, 1);
      EvrSetUnivOutMap(pEr, i-1, i-1);
      EvrSetTBOutMap(pEr, i-1, i-1);
    }

  EvrMapRamEnable(pEr, 0, 1);
  EvrUnivDlyEnable(pEr, 0, 1);
  EvrUnivDlyEnable(pEr, 1, 1);
  EvrUnivDlySetDelay(pEr, 0, 0, 0);
  EvrUnivDlySetDelay(pEr, 1, 0, 0);
  /*
  EvrDumpPulses(pEr, 10);
  EvrDumpMapRam(pEr, 0);
  EvrDumpUnivOutMap(pEr, 10);
  */

  for (i = 1; i < 11; i++)
    {
      EvrSetLedEvent(pBEr, 0, i, 1);
      EvrSetPulseMap(pBEr, 0, i, i-1, -1, -1);
      EvrSetPulseParams(pBEr, i-1, 1, 0, 100);
      EvrSetPulseProperties(pBEr, i-1, 0, 0, 0, 1, 1);
      EvrSetUnivOutMap(pBEr, i-1, i-1);
      EvrSetTBOutMap(pBEr, i-1, i-1);
    }

  EvrMapRamEnable(pBEr, 0, 1);
  EvrUnivDlyEnable(pBEr, 0, 1);
  EvrUnivDlyEnable(pBEr, 1, 1);
  EvrUnivDlySetDelay(pBEr, 0, 0, 0);
  EvrUnivDlySetDelay(pBEr, 1, 0, 0);
  /*  EvrDumpTBOutMap(pBEr, 10); */

  /*
   * Test SW event -> EVR UNIVOUT
   */

  printf("Test SW event -> EVR UNIVOUT\n");

  EvgSWEventEnable(pEg, 1);
  if (!EvgGetSWEventEnable(pEg))
    {
      printf(ERROR_TEXT "Could not enable EVG SWEvent!\n");      
      err_gen++;
    }

  for (i = 1; i < 11; i++)
    {
      EvrClearDiagCounters(pEr);
      EvrEnableDiagCounters(pEr, 1);

#define PULSES 10000

      for (j = 0; j < PULSES; j++)
	EvgSendSWEvent(pEg, i);

      usleep(1000);

      for (j = 1; j < 11; j++)
	{
	  if (j != i)
	    {
	      if (EvrGetDiagCounter(pEr, j-1))
		{
		  printf(ERROR_TEXT "1 Spurious pulses on UNIVOUT%d: %d, i = %d, j = %d\n", j-1,
			 EvrGetDiagCounter(pEr, j-1), i, j);
		  err_pulse++;
		}
	    }
	  else if (EvrGetDiagCounter(pEr, j-1) != PULSES)
	    {
	      printf(ERROR_TEXT "1 Wrong number of pulses on UNIVOUT%d: %d (%d), i = %d, j = %d\n",
		     j-1, EvrGetDiagCounter(pEr, j-1), PULSES, i, j);
	      err_pulse++;
	    }
	}
    }

  EvgSWEventEnable(pEg, 0);
  if (EvgGetSWEventEnable(pEg))
    {
      printf(ERROR_TEXT "Could not disable EVG SWEvent!\n");      
      err_gen++;
    }

  /*
   * Event Analyzer test
   */

  printf("Event Analyzer test\n");

  {
    struct EvanStruct ev1, ev2;

    EvgEvanReset(pEg);
    EvgEvanResetCount(pEg);

    EvgEvanEnable(pEg, 1);
    if (!EvgEvanGetEnable(pEg))
      {
	printf(ERROR_TEXT "Could not enable Event Analyzer!\n");      
	err_gen++;
      }

    EvgSWEventEnable(pEg, 1);
    if (!EvgGetSWEventEnable(pEg))
      {
	printf(ERROR_TEXT "Could not enable EVG SWEvent!\n");      
	err_gen++;
      }

    for (j = 1; j < 256; j++)
      EvgSendSWEvent(pEg, j);

    ev1.TimestampHigh = 0;
    ev1.TimestampLow = 0;
    ev1.EventCode = 0;
    
    for (j = 1; j < 256; j++)
      {
	if (EvgEvanGetEvent(pEg, &ev2))
	  {
	    printf(ERROR_TEXT "Event analyzer empty after %d reads!\n", j);
	    err_evan++;
	    break;
	  }
	if ((ev2.EventCode & 0x00ff) != j)
	  {
	    printf(ERROR_TEXT "Evan: wrong event code %02x/%02x\n", ev2.EventCode, j);
	    err_evan++;
	  }
	if (ev2.TimestampLow <= ev1.TimestampLow)
	  {
	    printf(ERROR_TEXT "Evan: Timestamp error %d - %d\n", ev1.TimestampLow, ev2.TimestampLow);
	    err_evan++;
	  }

	ev1.TimestampHigh = ev2.TimestampHigh;
	ev1.TimestampLow = ev2.TimestampLow;
	ev1.EventCode = ev2.EventCode;
      }

    EvgSWEventEnable(pEg, 0);
    if (EvgGetSWEventEnable(pEg))
      {
	printf(ERROR_TEXT "Could not disable EVG SWEvent!\n");      
	err_gen++;
      }

    EvgEvanEnable(pEg, 0);
    if (EvgEvanGetEnable(pEg))
      {
	printf(ERROR_TEXT "Could not disable Event Analyzer!\n");      
	err_gen++;
      }
  }

  /*
   * MXC tests
   */

  printf("MXC tests\n");

  {
    int cnt[8];

    for (i = 1; i <= 8; i++)
      {
	EvgSetMXCPrescaler(pEg, i-1, 0x0ffff);
      }
    EvgSyncMxc(pEg);
    
    EvrClearDiagCounters(pEr);
    EvrEnableDiagCounters(pEr, 1);

    for (i = 1; i <= 8; i++)
      {
	/* Map MXC0 to TRIG0, etc. */
	EvgSetMxcTrigMap(pEg, i-1, i-1);
	EvgSetTriggerEvent(pEg, i-1, i, 1);
	EvgSetMXCPrescaler(pEg, i-1, 64 << i);
      }

    /*    
    EvgMXCDump(pEg);
    */

    usleep(1000);
    
    EvrEnableDiagCounters(pEr, 0);
    
    for (i = 0; i < 8; i++)
      {
	cnt[i] = EvrGetDiagCounter(pEr, i);
	/*
	printf("MXC%d: %d pulses\n", i, cnt[i]);
	*/
	if (i > 0)
	  {
	    if (cnt[i] < (cnt[i-1] >> 1) - (cnt[i-1] >> 4) || cnt[i] > (cnt[i-1] >> 1) + (cnt[i-1] >> 4))
	      {
		printf(ERROR_TEXT "MXC%d: %d pulses, MXC%d: %d pulses\n", i-1, cnt[i-1], i, cnt[i]);
		err_mxc++;
	      }
	  }
      }

    for (i = 1; i <= 8; i++)
      {
	/* Unmap, reset prescalers */
	EvgSetMxcTrigMap(pEg, i-1, -1);
	EvgSetMXCPrescaler(pEg, i-1, 0);
      }
  }

  /*
   * DBUS tests
   */

  printf("DBUS tests\n");

  {
    int cnt[8];

    for (i = 1; i <= 8; i++)
      {
	EvgSetMXCPrescaler(pEg, i-1, 0x0ffff);
      }
    
    for (i = 1; i <= 8; i++)
      {
	/* Map MXC0 to DBUS0, etc. */
	EvgSetDBusMap(pEg, i-1, C_EVG_DBUS_SEL_MXC);
	EvgSetMXCPrescaler(pEg, i-1, 16 << i);
	EvrSetUnivOutMap(pEr, i-1, i-1+C_EVR_SIGNAL_MAP_DBUS);
	EvrSetTBOutMap(pEr, i-1, i-1+C_EVR_SIGNAL_MAP_DBUS);
      }
    
    EvgSyncMxc(pEg);
    
    EvrClearDiagCounters(pEr);
    EvrEnableDiagCounters(pEr, 1);

    usleep(10000);
    
    EvrEnableDiagCounters(pEr, 0);
    
    for (i = 0; i < 8; i++)
      {
	cnt[i] = EvrGetDiagCounter(pEr, i);
	if (i > 0)
	  {
	    if (cnt[i] < (cnt[i-1] >> 1) - (cnt[i-1] >> 4) || cnt[i] > (cnt[i-1] >> 1) + (cnt[i-1] >> 4))
	      {
		printf(ERROR_TEXT "MXC/DBUS%d: %d pulses, MXC%d: %d pulses\n", i-1, cnt[i-1], i, cnt[i]);
		err_mxc++;
	      }
	  }
      }

    for (i = 1; i <= 8; i++)
      {
	EvgSetDBusMap(pEg, i-1, C_EVG_DBUS_SEL_EXT);
	EvgSetUnivinMap(pEg, i-1, -1, i-1);
      }

    EvrClearDiagCounters(pEr);
    EvrEnableDiagCounters(pEr, 1);

    /* Generate pulses */
    for (i = 1; i <= 8; i++)
      for (j = 0; j < (1024 >> i); j++)
	{
	  EvgSetUnivOutMap(pEg, i-1, 0);
	  EvgSetUnivOutMap(pEg, i-1, (1 << EVG_SIGNAL_MAP_BITS) - 2);
	  EvgSetUnivOutMap(pEg, i-1, 0);
	}

    usleep(1000);
    
    EvrEnableDiagCounters(pEr, 0);
    
    for (i = 0; i < 8; i++)
      {
	cnt[i] = EvrGetDiagCounter(pEr, i);
	if (i > 0)
	  {
	    if (cnt[i] != cnt[i-1] >> 1)
	      {
		printf(ERROR_TEXT "DBUS%d: %d pulses, DBUS%d: %d pulses\n", i-1, cnt[i-1], i, cnt[i]);
		err_dbus++;
	      }
	  }
      }


    for (i = 1; i <= 8; i++)
      {
	/* Unmap, reset prescalers */
	EvgSetDBusMap(pEg, i-1, 0);
	EvgSetMXCPrescaler(pEg, i-1, 0);
	/* Restore EVR UnivOut mapping */
	EvrSetUnivOutMap(pEr, i-1, i-1);
	EvrSetTBOutMap(pEr, i-1, i-1);
	EvgSetUnivinMap(pEg, i-1, -1, -1);
      }
  }

  /*
   * AC Input tests
   */

  printf("AC Input tests\n");

  {
    int cnt;

    /* Lets put 50 Hz on MXC3, output on UNIVOUT0 -> LEMO Loopback to ACIN */
    EvgSetMXCPrescaler(pEg, 3, 499.654E6/4/50);
    EvgSyncMxc(pEg);
    EvgSetUnivOutMap(pEg, 0, C_SIGNAL_MAP_PRESC+3);

    /* Setup AC input for 10 Hz */
    EvgSetACInput(pEg, 0, 0, 5, 0);
    /* Map AC trigger to trigger event 3 */
    EvgSetACMap(pEg, C_EVG_ACMAP_TRIG_BASE+3);

    EvrClearDiagCounters(pEr);
    EvrEnableDiagCounters(pEr, 1);

    EvgSetTriggerEvent(pEg, 3, 4, 1);
    sleep(1);

    cnt = EvrGetDiagCounter(pEr, 3);
    if (cnt < 9 || cnt > 11)
      {
	printf(ERROR_TEXT "AC input 50 Hz, div/5, count %d\n", cnt);
	err_acin++;
      }
    
    /* Restore registers */
    EvgSetTriggerEvent(pEg, 3, 4, 0);
    EvgSetACMap(pEg, -1);
    EvgSetMXCPrescaler(pEg, 3, 0);
    EvgSetUnivOutMap(pEg, 0, C_SIGNAL_MAP_LOW);
  }


  /*
   * RF Input test
   */

#ifdef USE_EXT_RF
  printf("Changing to external /4 reference.\n");
  EvgSetRFInput(pEg, 1, C_EVG_RFDIV_4);
  usleep(10000);
  /* Clear violation flag */
  EvrGetViolation(pEr, 1);
  EvrGetViolation(pBEr, 1);
#endif

  /*
   * Sequence RAM tests
   */

  printf("Sequence RAM tests\n");

  {
    int ram;
    int cnt[7];

    for (ram = 0; ram < 2; ram++)
      {
	/* Lets put 50 Hz on MXC3, output on UNIVOUT0 -> LEMO Loopback to ACIN */
	EvgSetMXCPrescaler(pEg, 3, 499.654E6/4/50);
	EvgSyncMxc(pEg);
	EvgSetUnivOutMap(pEg, 0, C_SIGNAL_MAP_PRESC+3);

	/* Setup AC input for 10 Hz */
	EvgSetACInput(pEg, 0, 0, 5, 0);

	EvrClearDiagCounters(pEr);
	EvrEnableDiagCounters(pEr, 1);

	/* Write some RAM events */
	EvgSetSeqRamEvent(pEg, ram, 0, 10, 1);
	EvgSetSeqRamEvent(pEg, ram, 1, 20, 2);
	EvgSetSeqRamEvent(pEg, ram, 2, 30, 3);
	EvgSetSeqRamEvent(pEg, ram, 3, 31, 4);
	EvgSetSeqRamEvent(pEg, ram, 4, 32, 5);
	EvgSetSeqRamEvent(pEg, ram, 5, 33, 6);
	EvgSetSeqRamEvent(pEg, ram, 6, 34, 7);
	EvgSetSeqRamEvent(pEg, ram, 7, 40, 0x7f);
	/*
	EvgSeqRamStatus(pEg, ram);
	EvgSeqRamDump(pEg, ram);
	*/
	EvgSeqRamControl(pEg, ram, 1, 0, 0, 0, C_EVG_SEQTRIG_ACINPUT);

	sleep(1);

	EvgSeqRamControl(pEg, ram, 1, 0, 0, 0, 0);

	for (i = 0; i < 7; i++)
	  {
	    cnt[i] = EvrGetDiagCounter(pEr, i);
	    if (cnt[i] < 9 || cnt[i] > 11)
	      {
		printf(ERROR_TEXT "SEQ%d OUT%d %d cycles out of 10\n", ram, i, cnt[i]);
		err_seq++;
	      }
	  }
      }

    EvgSetMXCPrescaler(pEg, 3, 0);
    EvgSetUnivOutMap(pEg, 0, C_SIGNAL_MAP_LOW);
  }

  /*
   * Data transfer test
   */
  
  printf("Data transfer tests\n");

  {
    int buf1[EVG_MAX_BUFFER/4], buf2[EVG_MAX_BUFFER/4];
    int size;

    /*
    printf("buf1 %08x, buf2 %08x\n", (int) buf1, (int) buf2);
    */
    if (!(EvrSetDBufMode(pEr, 1) & (1 << C_EVR_DATABUF_MODE)))
      {
	printf(ERROR_TEXT "Could not enable EVR databuffer mode!\n");
	err_gen++;
      }
    if (!(EvgSetDBufMode(pEg, 1) & (1 << C_EVG_DATABUF_MODE)))
      {
	printf(ERROR_TEXT "Could not enable EVG databuffer mode!\n");
	err_gen++;
      }

    for (i = 4; i < EVG_MAX_BUFFER; i += 4)
      {
	for (size = 0; size < i/4; size++)
	  buf1[size] = rand() + (rand() << 15) + (rand() << 30);
 
	EvrReceiveDBuf(pEr, 1);
	if (EvgSendDBuf(pEg, (char *) buf1, i) < 0)
	  {
	    printf(ERROR_TEXT "Error sending out data buffer %x.\n", EvgGetDBufStatus(pEg));
	    err_dbuf++;
	  }
	usleep(100);
	if (!(EvgGetDBufStatus(pEg) & (1 << C_EVG_DATABUF_COMPLETE)))
	  {
	    printf(ERROR_TEXT "Data buffer transmit not complete.\n");
	    err_dbuf++;
	  }
	size = EvrGetDBuf(pEr, (char *) buf2, EVG_MAX_BUFFER) & (EVG_MAX_BUFFER-1);
	if (size != i)
	  {
	    printf(ERROR_TEXT "Error receiving data buffer size %d/%d stat %x\n", size, i, EvrGetDBufStatus(pEr));
	    err_dbuf++;
	  }
	else
	  if ((size = memcmp((void *) buf1, (void *) buf2, i)) != 0)
	    {
	      printf(ERROR_TEXT "Data buffer %d data error %d\n", i, size);
	      err_dbuf++;
	      for (size = 0; size < i/4; size++)
		if (buf1[size] != buf2[size])
		printf(ERROR_TEXT "TX %08x, %08x, RX %08x, %08x\n", buf1[size], pEg->Databuf[size], buf2[size], pEr->Databuf[size]);
	      //		printf("TX %08lx, RX %08lx\n", pEg->Databuf[size], pEr->Databuf[size]);
	    }
      }

    if ((EvgSetDBufMode(pEg, 0) & (1 << C_EVG_DATABUF_MODE)))
      {
	printf(ERROR_TEXT "Could not disable EVG databuffer mode!\n");
	err_gen++;
      }
    if ((EvrSetDBufMode(pEr, 0) & (1 << C_EVR_DATABUF_MODE)))
      {
	printf(ERROR_TEXT "Could not disable EVR databuffer mode!\n");
	err_gen++;
      }
  }

  /*
   * Test EVG UNIVIN -> Trigger event -> EVR UNIVOUT
   */

  printf("Test EVG UNIVIN -> Trigger event -> EVR UNIVOUT\n");

  for (i = 1; i <= 8; i++)
    {
      EvgSetUnivinMap(pEg, i-1, i-1, -1);
      EvgSetTriggerEvent(pEg, i-1, i, 1);
    }

  for (i = 1; i <= 8; i++)
    {
      EvrClearDiagCounters(pEr);
      EvrEnableDiagCounters(pEr, 1);

      /* Generate pulses */
      for (j = 0; j < PULSES; j++)
	{
	  EvgSetUnivOutMap(pEg, i-1, 0);
	  EvgSetUnivOutMap(pEg, i-1, (1 << EVG_SIGNAL_MAP_BITS) - 2);
	  EvgSetUnivOutMap(pEg, i-1, (1 << EVG_SIGNAL_MAP_BITS) - 2);
	  EvgSetUnivOutMap(pEg, i-1, (1 << EVG_SIGNAL_MAP_BITS) - 2);
	  EvgSetUnivOutMap(pEg, i-1, (1 << EVG_SIGNAL_MAP_BITS) - 2);
	  EvgSetUnivOutMap(pEg, i-1, 0);
	  EvgSetUnivOutMap(pEg, i-1, 0);
	  EvgSetUnivOutMap(pEg, i-1, 0);
	  EvgSetUnivOutMap(pEg, i-1, 0);
	}

      usleep(1000);

      for (j = 1; j <= 8; j++)
	{
	  if (j != i)
	    {
	      if (EvrGetDiagCounter(pEr, j-1))
		{
		  printf(ERROR_TEXT "2 Spurious pulses on UNIVOUT%d: %d\n", j-1,
			 EvrGetDiagCounter(pEr, j-1));
		  err_pulse++;
		}
	    }
	  else if (EvrGetDiagCounter(pEr, j-1) != PULSES)
	    {
	      printf(ERROR_TEXT "2 Wrong number of pulses on UNIVOUT%d: %d (%d)\n",
		     j-1, EvrGetDiagCounter(pEr, j-1), PULSES);
	      err_pulse++;
	    }
	}
    }

  for (i = 1; i <= 8; i++)
    {
      EvgSetUnivinMap(pEg, i-1, -1, -1);
      EvgSetTriggerEvent(pEg, i-1, i, 0);
    }

  /*
   * Irq tests
   */

  printf("EVR Irq tests\n");
  {
    evr_clr_irq();
    EvrClearIrqFlags(pEr, -1);

    EvrIrqEnable(pEr, EVR_IRQ_MASTER_ENABLE | EVR_IRQFLAG_VIOLATION);
    
    /* Generate violation by reloading fractional synth */
    EvrSetFracDiv(pEr, i = EvrGetFracDiv(pEr));

    /*
    printf("Frac. div. word %08x, %08x\n", i, EvrGetFracDiv(pEr));
    */

    /* This sleep will be interrupted by IRQ */
    sleep(1);
    
    if (!evr_irq_cnt[C_EVR_IRQFLAG_VIOLATION])
      {
	printf(ERROR_TEXT "Violation IRQ failed %d/1\n", evr_irq_cnt[C_EVR_IRQFLAG_VIOLATION]);
	err_irq++;
      }

    /* We have to disable IRQs to be able to sleep for a while and wait link is up */
    EvrIrqEnable(pEr, 0);

    sleep(1);
    
    EvrClearIrqFlags(pEr, -1);

    if (EvrGetViolation(pEr, 1))
    {
      err_vio++;
      printf(ERROR_TEXT "Could not clear violation flag!\n");
      EvrDumpStatus(pEr);
    }
    EvrGetViolation(pBEr, 1);
    
    evr_clr_irq();
    EvrClearFIFO(pEr);
    EvrClearIrqFlags(pEr, -1);
    EvrIrqEnable(pEr, EVR_IRQ_MASTER_ENABLE | EVR_IRQFLAG_FIFOFULL);
    EvrSetFIFOEvent(pEr, 0, 1, 1);
    EvgSWEventEnable(pEg, 1);
    for (j = 0; j < 511; j++)
      EvgSendSWEvent(pEg, 1);
    EvgSWEventEnable(pEg, 0);
    EvrSetFIFOEvent(pEr, 0, 1, 0);

    usleep(100);
    if (!evr_irq_cnt[C_EVR_IRQFLAG_FIFOFULL])
      {
	printf(ERROR_TEXT "FIFO full IRQ failed %d/1\n", evr_irq_cnt[C_EVR_IRQFLAG_FIFOFULL]);
	err_irq++;
      }
    EvrClearFIFO(pEr);

    evr_clr_irq();
    EvrClearIrqFlags(pEr, -1);
    EvrIrqEnable(pEr, EVR_IRQ_MASTER_ENABLE | EVR_IRQFLAG_HEARTBEAT);
    EvgSWEventEnable(pEg, 1);
    EvgSendSWEvent(pEg, 0x7a);
    sleep(1);
    EvgSendSWEvent(pEg, 0x7a);
    sleep(1);
    EvgSendSWEvent(pEg, 0x7a);
    sleep(1);
    if (evr_irq_cnt[C_EVR_IRQFLAG_HEARTBEAT])
      {
	printf(ERROR_TEXT "Received heartbeat interrupt %d/0!\n", evr_irq_cnt[C_EVR_IRQFLAG_HEARTBEAT]);
	err_irq++;
      }
    sleep(2);
    if (!evr_irq_cnt[C_EVR_IRQFLAG_HEARTBEAT])
      {
	printf(ERROR_TEXT "Heartbeat interrupt failed %d/1!\n", evr_irq_cnt[C_EVR_IRQFLAG_HEARTBEAT]);
	err_irq++;
      }
    EvgSWEventEnable(pEg, 0);

    evr_clr_irq();
    EvrClearFIFO(pEr);
    EvrClearIrqFlags(pEr, -1);
    EvrIrqEnable(pEr, EVR_IRQ_MASTER_ENABLE | EVR_IRQFLAG_EVENT);
    EvrSetFIFOEvent(pEr, 0, 1, 1);
    EvgSWEventEnable(pEg, 1);
    EvgSendSWEvent(pEg, 1);
    EvgSWEventEnable(pEg, 0);
    EvrSetFIFOEvent(pEr, 0, 1, 0);

    usleep(100);
    if (!evr_irq_cnt[C_EVR_IRQFLAG_EVENT])
      {
	printf(ERROR_TEXT "Event IRQ failed %d/1\n", evr_irq_cnt[C_EVR_IRQFLAG_EVENT]);
	err_irq++;
      }
    EvrClearFIFO(pEr);

    evr_clr_irq();
    EvrClearIrqFlags(pEr, -1);
    EvrIrqEnable(pEr, EVR_IRQ_MASTER_ENABLE | EVR_IRQFLAG_PULSE);
    
    EvrSetPulseIrqMap(pEr, C_EVR_SIGNAL_MAP_LOW);
    EvrSetPulseIrqMap(pEr, C_EVR_SIGNAL_MAP_HIGH);
    EvrSetPulseIrqMap(pEr, C_EVR_SIGNAL_MAP_LOW);

    usleep(100);

    if (evr_irq_cnt[C_EVR_IRQFLAG_PULSE] != 1)
      {
	printf(ERROR_TEXT "Pulse IRQ failed %d/1\n", evr_irq_cnt[C_EVR_IRQFLAG_PULSE]);
	err_irq++;
      }

    evr_clr_irq();
    EvrSetDBufMode(pEr, 1);
    EvgSetDBufMode(pEg, 1);
    EvrClearIrqFlags(pEr, -1);
    EvrIrqEnable(pEr, EVR_IRQ_MASTER_ENABLE | EVR_IRQFLAG_DATABUF);
    EvrReceiveDBuf(pEr, 1);
    EvgSendDBuf(pEg, (char *) &i, 4);
    usleep(100);
    if (!evr_irq_cnt[C_EVR_IRQFLAG_DATABUF])
      {
	printf(ERROR_TEXT "Databuf IRQ failed %d/1\n", evr_irq_cnt[C_EVR_IRQFLAG_DATABUF]);
	err_irq++;
      }
    EvgSetDBufMode(pEg, 0);
    EvrSetDBufMode(pEr, 0);

    evr_clr_irq();
    EvrClearIrqFlags(pEr, -1);
    EvrIrqEnable(pEr, 0);
  }

  /*
   * Event FIFO test
   */

  if (EvrGetViolation(pEr, 1))
    {
      err_vio++;
      printf(ERROR_TEXT "Violation flag!\n");
    }

  printf("Event FIFO test\n");
  {
    int cnt[2];
    struct FIFOEvent fe[10];

    EvrClearFIFO(pEr);
    EvgSWEventEnable(pEg, 1);
    EvrSetFIFOEvent(pEr, 0, 1, 1);
    EvrSetLatchEvent(pEr, 0, 2, 1);
    EvrSetFIFOEvent(pEr, 0, 3, 1);
    EvrSetLatchEvent(pEr, 0, 3, 1);

    /* Timestamp event counter tests */
    EvrSetTimestampDBus(pEr, 0);
    EvrSetTimestampDivider(pEr, 0);
    /* Reset event counter */
    EvgSendSWEvent(pEg, 0x7d);
    EvgSendSWEvent(pEg, 0x7c);
    usleep(100);
    cnt[0] = EvrGetTimestampCounter(pEr);
    if (cnt[0])
      {
	printf(ERROR_TEXT "Event counter not reset %d/0\n", cnt[0]);
	err_fifo++;
      }

    EvgSendSWEvent(pEg, 2);
    usleep(100);
    cnt[0] = EvrGetTimestampLatch(pEr);
    if (cnt[0])
      {
	printf(ERROR_TEXT "Timestamp latch not reset %d/0\n", cnt[0]);
	err_fifo++;
      }

    EvgSendSWEvent(pEg, 0x7c);
    usleep(200);
    cnt[0] = EvrGetTimestampCounter(pEr);
    if (cnt[0] != 1)
      {
	printf(ERROR_TEXT "Event counter not incremented by TS event %d/1\n", cnt[0]);
	err_fifo++;
      }

    EvgSendSWEvent(pEg, 0x7d);
    EvrSetTimestampDBus(pEr, 1);
    EvgSetMXCPrescaler(pEg, 4, 0x2);
    EvgSyncMxc(pEg);
    EvgSetDBusMap(pEg, 4, C_EVG_DBUS_SEL_MXC);
    usleep(100);
    cnt[0] = EvrGetTimestampCounter(pEr);
    if (cnt[0] < 2)
      {
	printf(ERROR_TEXT "Event counter not counting (DBUS)\n");
	err_fifo++;
      }
    EvrSetTimestampDBus(pEr, 0);
    EvgSendSWEvent(pEg, 0x7d);
    EvgSendSWEvent(pEg, 0x7c);
    usleep(100);
    cnt[0] = EvrGetTimestampCounter(pEr);
    if (cnt[0])
      {
	printf(ERROR_TEXT "2 Event counter not reset %d/0\n", cnt[0]);
	err_fifo++;
      }
    EvgSetDBusMap(pEg, 4, 0);
    EvgSetMXCPrescaler(pEg, 4, 0x0000ffff);

    EvrSetTimestampDivider(pEr, 1);
    usleep(100);
    cnt[0] = EvrGetTimestampCounter(pEr);
    if (!cnt[0])
      {
	printf(ERROR_TEXT "Event counter not counting (int)\n");
	err_fifo++;
      }
    EvgSendSWEvent(pEg, 2);
    usleep(100);
    cnt[0] = EvrGetTimestampLatch(pEr);
    if (!cnt[0])
      {
	printf(ERROR_TEXT "Timestamp latch not updated %d/0\n", cnt[0]);
	err_fifo++;
      }

    for (i = 0; i < 32; i++)
      EvgSendSWEvent(pEg, 0x70);
    /* Reset event counter & load seconds counter */
    EvgSendSWEvent(pEg, 0x7d);
    usleep(100);
    cnt[0] = EvrGetSecondsCounter(pEr);
    if (cnt[0])
      {
	printf(ERROR_TEXT "Seconds counter not reset %d/0\n", cnt[0]);
	err_fifo++;
      }
    EvgSendSWEvent(pEg, 0x71);
    EvgSendSWEvent(pEg, 0x70);
    EvgSendSWEvent(pEg, 0x71);
    EvgSendSWEvent(pEg, 0x70);
    EvgSendSWEvent(pEg, 0x71);
    EvgSendSWEvent(pEg, 0x70);
    EvgSendSWEvent(pEg, 0x71);
    EvgSendSWEvent(pEg, 0x70);
    EvgSendSWEvent(pEg, 0x70);
    EvgSendSWEvent(pEg, 0x71);
    EvgSendSWEvent(pEg, 0x70);
    EvgSendSWEvent(pEg, 0x71);
    EvgSendSWEvent(pEg, 0x70);
    EvgSendSWEvent(pEg, 0x71);
    EvgSendSWEvent(pEg, 0x70);
    EvgSendSWEvent(pEg, 0x71);
    EvgSendSWEvent(pEg, 0x70);
    EvgSendSWEvent(pEg, 0x71);
    EvgSendSWEvent(pEg, 0x71);
    EvgSendSWEvent(pEg, 0x70);
    EvgSendSWEvent(pEg, 0x70);
    EvgSendSWEvent(pEg, 0x71);
    EvgSendSWEvent(pEg, 0x71);
    EvgSendSWEvent(pEg, 0x70);
    EvgSendSWEvent(pEg, 0x71);
    EvgSendSWEvent(pEg, 0x70);
    EvgSendSWEvent(pEg, 0x70);
    EvgSendSWEvent(pEg, 0x71);
    EvgSendSWEvent(pEg, 0x71);
    EvgSendSWEvent(pEg, 0x70);
    EvgSendSWEvent(pEg, 0x70);
    EvgSendSWEvent(pEg, 0x71);
    /* Reset event counter & load seconds counter */
    EvgSendSWEvent(pEg, 0x7d);
    usleep(100);
    cnt[0] = EvrGetSecondsCounter(pEr);
    cnt[1] = 0xaa556699;
    if (cnt[0] != cnt[1])
      {
	printf(ERROR_TEXT "Seconds counter not loaded %08x/%08x\n", cnt[0], cnt[1]);
	err_fifo++;
      }
    
    EvgSendSWEvent(pEg, 2);
    usleep(100);
    cnt[0] = EvrGetSecondsLatch(pEr);
    if (cnt[0] != cnt[1])
      {
	printf(ERROR_TEXT "Seconds latch not updated %08x/%08x\n", cnt[0], cnt[1]);
	err_fifo++;
      }

    EvrClearFIFO(pEr);

    for (i = 0; i < 8; i++)
      {
	EvrSetFIFOEvent(pEr, 0, (1 << i), 1);
	EvgSendSWEvent(pEg, (1 << i));
      }
    
    for (i = 0; i < 9; i++)
      {
	j = EvrGetFIFOEvent(pEr, &fe[i]);
	if (!j)
	  {
	    if (fe[i].EventCode != (1 << i))
	      {
		printf(ERROR_TEXT "Wrong event code in FIFO %d/%d\n", fe[i].EventCode, (1 << i));
		err_fifo++;
	      }
	    if (fe[i].TimestampHigh != cnt[1])
	      {
		printf(ERROR_TEXT "Seconds error in FIFO %08x/%08x\n", fe[i].TimestampHigh, cnt[1]);
		err_fifo++;
	      }
	    if (i > 0)
	      if (fe[i].TimestampLow < fe[i-1].TimestampLow)
		{
		  printf(ERROR_TEXT "Timestamp error in FIFO %08x/%08x\n",  fe[i-1].TimestampLow,
			 fe[i].TimestampLow);
		  err_fifo++;
		}
	  }
      }

    if (!j)
      {
	printf( ERROR_TEXT "FIFO not empty after reading %i events.\n", i );
	err_fifo++;
      }

    
    if (!EvrEnableFIFOStopEvent(pEr, 1))
      {
	printf(ERROR_TEXT "Could not enable FIFO stop event.\n");
	err_fifo++;
      }

    for (i = 0x70; i < 0x7f; i++)
      {
	EvrSetFIFOEvent(pEr, 0, i, 1);
	EvgSendSWEvent(pEg, i);
      }

    if (!EvrGetFIFOState(pEr))
      {
	printf(ERROR_TEXT "Event FIFO not stopped.\n");
	err_fifo++;
      }

    EvrDumpFIFO(pEr);

    EvrEnableFIFO(pEr, 1);
    if (EvrGetFIFOState(pEr))
      {
	printf(ERROR_TEXT "Event FIFO not enabled.\n");
	err_fifo++;
      }

    EvrEnableFIFO(pEr, 0);
    if (!EvrGetFIFOState(pEr))
      {
	printf(ERROR_TEXT "Event FIFO not stopped.\n");
	err_fifo++;
      }

    if (EvrEnableFIFOStopEvent(pEr, 0))
      {
	printf(ERROR_TEXT "Could not disable FIFO stop event.\n");
	err_fifo++;
      }

    EvrEnableFIFO(pEr, 1);
    if (EvrGetFIFOState(pEr))
      {
	printf(ERROR_TEXT "Event FIFO not enabled.\n");
	err_fifo++;
      }

    for (i = 0; i < 256; i++)
      {
	EvrSetFIFOEvent(pEr, 0, i, 0);
	EvrSetLatchEvent(pEr, 0, i, 0);
      }

    EvgSWEventEnable(pEg, 0);
  }

  /*
   * External Event test
   */

  if (EvrGetViolation(pEr, 1))
    {
      err_vio++;
      printf(ERROR_TEXT "Violation flag!\n");
    }

  printf("EVR external event test.\n");

  {
    EvrClearDiagCounters(pEr);
    EvrEnableDiagCounters(pEr, 1);
    EvrSetExtEvent(pEr, 0, 1, 1);
    EvrSetExtEvent(pEr, 1, 2, 1);
    for (i = 0; i < 8; i++)
      {
	EvgSetUnivOutMap(pEg, 2, C_SIGNAL_MAP_LOW);
	EvgSetUnivOutMap(pEg, 2, C_SIGNAL_MAP_HIGH);
	EvgSetUnivOutMap(pEg, 2, C_SIGNAL_MAP_LOW);
	usleep(10);
	EvgSetUnivOutMap(pEg, 3, C_SIGNAL_MAP_LOW);
	EvgSetUnivOutMap(pEg, 3, C_SIGNAL_MAP_HIGH);
	EvgSetUnivOutMap(pEg, 3, C_SIGNAL_MAP_LOW);
	usleep(10);
	EvgSetUnivOutMap(pEg, 2, C_SIGNAL_MAP_LOW);
	EvgSetUnivOutMap(pEg, 2, C_SIGNAL_MAP_HIGH);
	EvgSetUnivOutMap(pEg, 2, C_SIGNAL_MAP_LOW);
	usleep(10);
      }
    if ((i = EvrGetDiagCounter(pEr, 0)) != 16)
      {
	printf(ERROR_TEXT "Wrong number of ext0 events %d/16\n", i);
	err_extev++;
      }

    if (ff == 0)
      {
	if ((i = EvrGetDiagCounter(pEr, 1)) != 8)
	  {
	    printf(ERROR_TEXT "Wrong number of ext1 events %d/8\n", i);
	    err_extev++;
	  }
      }
    
    EvrSetExtEvent(pEr, 0, 0, 0);
    EvrSetExtEvent(pEr, 1, 0, 0);
  }

  /*
   * Backward event test
   */

  printf("Backward event test.\n");

  for (i = 0; i < 2; i++)
    {
      EvrClearDiagCounters(pBEr);
      EvrEnableDiagCounters(pBEr, 1);

      EvrSetBackEvent(pEr, i, i+1, 1);

      for (j = 0; j < PULSES; j++)
	{
	  EvgSetUnivOutMap(pEg, i+2, 0);
	  EvgSetUnivOutMap(pEg, i+2, (1 << EVG_SIGNAL_MAP_BITS) - 2);
	  EvgSetUnivOutMap(pEg, i+2, 0);
	  EvgSetUnivOutMap(pEg, i+2, 0);
	  EvgSetUnivOutMap(pEg, i+2, 0);
	  EvgSetUnivOutMap(pEg, i+2, 0);
	  EvgSetUnivOutMap(pEg, i+2, 0);
	}

      /* Only one input on PMC-EVR-230 */
      k = 2;
      if (ff == 1)
	k = 1;

      for (j = 0; j < 1; j++)
	{
	  if (j != i)
	    {
	      if (EvrGetDiagCounter(pBEr, j))
		{
		  printf(ERROR_TEXT "3 Spurious pulses on UNIVOUT%d: %d\n", j,
			 EvrGetDiagCounter(pBEr, j));
		  err_bev++;
		}
	    }
	  else if (EvrGetDiagCounter(pBEr, j) != PULSES)
	    {
	      printf(ERROR_TEXT "3 Wrong number of pulses on UNIVOUT%d: %d (%d)\n",
		     j, EvrGetDiagCounter(pBEr, j), PULSES);
	      err_bev++;
	    }
	}

      EvrSetBackEvent(pEr, i, i+1, 0);
    }

  /*
   * Backward DBUS test
   */

  printf("Backward DBUS test\n");
  
  for (i = 0; i < 8; i++)
    {
      EvrSetUnivOutMap(pBEr, i, i+C_EVR_SIGNAL_MAP_DBUS);
      EvrSetTBOutMap(pBEr, i, i+C_EVR_SIGNAL_MAP_DBUS);
    }

  /* Only one input on PMC-EVR-230 */
  k = 2;
  if (ff == 1)
    k = 1;
  
  for (i = 0; i < k; i++)
    {
      EvrClearDiagCounters(pBEr);
      EvrEnableDiagCounters(pBEr, 1);

      EvrSetBackDBus(pEr, i, 4 << i);

      for (j = 0; j < PULSES; j++)
	{
	  EvgSetUnivOutMap(pEg, i+2, 0);
	  EvgSetUnivOutMap(pEg, i+2, (1 << EVG_SIGNAL_MAP_BITS) - 2);
	  EvgSetUnivOutMap(pEg, i+2, 0);
	  EvgSetUnivOutMap(pEg, i+2, 0);
	  EvgSetUnivOutMap(pEg, i+2, 0);
	  EvgSetUnivOutMap(pEg, i+2, 0);
	  EvgSetUnivOutMap(pEg, i+2, 0);
	}

      for (j = 0; j < 2; j++)
	{
	  if (j != i)
	    {
	      if (EvrGetDiagCounter(pBEr, j+2))
		{
		  printf(ERROR_TEXT "4 Spurious pulses on UNIVOUT%d: %d\n", j+2,
			 EvrGetDiagCounter(pBEr, j+2));
		  err_bdbus++;
		}
	    }
	  else if (EvrGetDiagCounter(pBEr, j+2) != PULSES)
	    {
	      printf(ERROR_TEXT "4 Wrong number of pulses on UNIVOUT%d: %d (%d)\n",
		     j+2, EvrGetDiagCounter(pBEr, j+2), PULSES);
	      err_bdbus++;
	    }
	}

      EvrSetBackDBus(pEr, i, 0);
    }
  
  for (i = 0; i < 8; i++)
    {
      EvrSetUnivOutMap(pBEr, i, i);
      EvrSetTBOutMap(pBEr, i, i);
    }

  /*
   * Backward data transfer test
   */

  printf("Back Data transfer tests\n");

  {
    int buf1[EVG_MAX_BUFFER/4], buf2[EVG_MAX_BUFFER/4];
    int size;

    /*
    printf("buf1 %08x, buf2 %08x\n", (int) buf1, (int) buf2);
    */
    if (!(EvrSetDBufMode(pBEr, 1) & (1 << C_EVR_DATABUF_MODE)))
      {
	printf(ERROR_TEXT "Could not enable BEVR databuffer mode!\n");
	err_gen++;
      }
    if (!((i = EvrSetTxDBufMode(pEr, 1)) & (1 << C_EVR_TXDATABUF_MODE)))
      {
	printf(ERROR_TEXT "Could not enable EVR tx databuffer mode (%08x)!\n", i);
	err_gen++;
      }

    for (i = 4; i < EVR_MAX_BUFFER; i += 4)
      {
	for (size = 0; size < i/4; size++)
	  buf1[size] = rand() + (rand() << 15) + (rand() << 30);
 
	EvrReceiveDBuf(pBEr, 1);
	if (EvrSendTxDBuf(pEr, (char *) buf1, i) < 0)
	  {
	    printf(ERROR_TEXT "Error sending out data buffer %x.\n", EvrGetTxDBufStatus(pEr));
	    err_bdbuf++;
	  }
	usleep(100);
	if (!(EvrGetTxDBufStatus(pEr) & (1 << C_EVR_TXDATABUF_COMPLETE)))
	  {
	    printf(ERROR_TEXT "Data buffer transmit not complete.\n");
	    err_bdbuf++;
	  }
	size = EvrGetDBuf(pBEr, (char *) buf2, EVG_MAX_BUFFER) & (EVG_MAX_BUFFER-1);
	if (size != i)
	  {
	    printf(ERROR_TEXT "Error receiving data buffer size %d/%d stat %x\n", size, i, EvrGetDBufStatus(pBEr));
	    err_bdbuf++;
	  }
	else
	  if ((size = memcmp((void *) buf1, (void *) buf2, i)) != 0)
	    {
	      printf(ERROR_TEXT "Data buffer %d data error %d\n", i, size);
	      err_bdbuf++;
	      for (size = 0; size < i/4; size++)
		if (buf1[size] != buf2[size])
		printf("TX %08x, %08x, RX %08x, %08x\n", buf1[size], pEr->Databuf[size], buf2[size], pBEr->Databuf[size]);
	      //		printf("TX %08x, RX %08x\n", pEr->Databuf[size], pBEr->Databuf[size]);
	    }
      }

    if ((EvrSetTxDBufMode(pEr, 0) & (1 << C_EVR_TXDATABUF_MODE)))
      {
	printf(ERROR_TEXT "Could not disable EVR TX databuffer mode!\n");
	err_gen++;
      }
    if ((EvrSetDBufMode(pBEr, 0) & (1 << C_EVR_DATABUF_MODE)))
      {
	printf(ERROR_TEXT "Could not disable BEVR databuffer mode!\n");
	err_gen++;
      }
  }

  /*
   * Event forwarding test
   */

  printf("Event forwarding test\n");

  EvrClearFIFO(pEr);
  EvrClearFIFO(pBEr);
  EvrSetTimestampDivider(pBEr, 1);

  for (i = 1; i < 10; i++)
    {
      if (i < 5)
	EvrSetForwardEvent(pEr, 0, i, 1);
      EvrSetFIFOEvent(pEr, 0, i, 1);
      EvrSetFIFOEvent(pBEr, 0, i, 1);
    }
  EvrSetForwardEvent(pEr, 0, 1, 1);

  EvgSWEventEnable(pEg, 1);

  EvrClearDiagCounters(pBEr);
  EvrEnableDiagCounters(pBEr, 1);

  EvgSendSWEvent(pEg, 1);
  
  usleep(1000);

  if ( (i = EvrGetDiagCounter(pBEr, 1)) != 0 )
    {
      printf(ERROR_TEXT "Spurios events %d (fowarding disabled)\n", i);
      err_fwd++;
    }
  
  EvrEnableEventForwarding(pEr, 1);

  /*
  EvrDumpStatus(pEr);
  EvrDumpMapRam(pEr, 0);
  */

  EvrClearDiagCounters(pBEr);
  EvrEnableDiagCounters(pBEr, 1);

  /* Write some RAM events */
  EvgSetSeqRamEvent(pEg, 0, 0, 10, 9);
  EvgSetSeqRamEvent(pEg, 0, 1, 11, 1);
  EvgSetSeqRamEvent(pEg, 0, 2, 12, 2);
  EvgSetSeqRamEvent(pEg, 0, 3, 13, 3);
  EvgSetSeqRamEvent(pEg, 0, 4, 14, 4);
  EvgSetSeqRamEvent(pEg, 0, 5, 15, 5);
  EvgSetSeqRamEvent(pEg, 0, 6, 16, 6);
  EvgSetSeqRamEvent(pEg, 0, 7, 17, 7);
  EvgSetSeqRamEvent(pEg, 0, 8, 18, 8);
  EvgSetSeqRamEvent(pEg, 0, 9, 19, 2);
  EvgSetSeqRamEvent(pEg, 0,10, 20, 1);
  EvgSetSeqRamEvent(pEg, 0,11, 21, 4);
  EvgSetSeqRamEvent(pEg, 0,12, 22, 3);
  EvgSetSeqRamEvent(pEg, 0,13, 40, 0x7f);

  EvgSeqRamControl(pEg, 0, 1, 0, 0, 0, C_EVG_SEQTRIG_SWTRIGGER0);
  EvgSeqRamSWTrig(pEg, 0);

  sleep(1);

  {
    struct FIFOEvent fe;
    int eventseq[9] = {1, 2, 3, 4, 2, 1, 4, 3, -1};
    
    for (i = 0; eventseq[i] > 0; i++)
      {
	if (EvrGetFIFOEvent(pBEr, &fe))
	  {
	    printf(ERROR_TEXT "Too few events.\n");
	    err_fwd++;
	    break;
	  }
	if (fe.EventCode != eventseq[i])
	  {
	    printf(ERROR_TEXT "Received wrong event %02x (%02x)\n",
		   fe.EventCode, eventseq[i]);
	    err_fwd++;
	  }
	printf("Forwarded event Code %08x, %08x:%08x\n",
	       fe.EventCode, fe.TimestampHigh, fe.TimestampLow);
      }
  }

  /*
  printf("--- Er ---\n");
  EvrDumpFIFO(pEr);
  printf("--- BEr ---\n");
  EvrDumpFIFO(pBEr);
  */

  EvrEnableEventForwarding(pEr, 0);

  EvgSWEventEnable(pEg, 0);

  /* Turn off forwarding */

  EvrSetForwardEvent(pEr, 0, 1, 0);

  EvrEnable(pBEr, 0);
  if (EvrGetEnable(pBEr))
    {
      printf(ERROR_TEXT "Could not disable BEVR!\n");
      err_gen++;
      return -1;
    }
  EvrEnable(pEr, 0);
  if (EvrGetEnable(pEr))
    {
      printf(ERROR_TEXT "Could not disable EVR!\n");
      err_gen++;
      return -1;
    }
  EvgEnable(pEg, 0);
  if (EvgGetEnable(pEg))
    {
      printf(ERROR_TEXT "Could not disable EVG!\n");
      err_gen++;
      return -1;
    }

  return 0;
}

int dlytest(struct MrfEgRegs *pEg, struct MrfErRegs *pEr)
{
  /*
   * Test DLYs
   */

  int i;

  EvrEnable(pEr, 1);
  if (!EvrGetEnable(pEr))
    {
      printf("Could not enable EVR!\n");
      err_gen++;
      return -1;
    }

  EvgEnable(pEg, 1);
  if (!EvgGetEnable(pEg))
    {
      printf("Could not enable EVG!\n");
      err_gen++;
      return -1;
    }

  printf("Test DLYs\n");

  /* Output DBUS0 on UNIV0 to UNIV3 */
  EvgSetUnivOutMap(pEg, 0, 32);
  EvgSetUnivOutMap(pEg, 1, 32);
  EvgSetUnivOutMap(pEg, 2, 32);
  EvgSetUnivOutMap(pEg, 3, 32);
  EvgSetMXCPrescaler(pEg, 0, 448/4); /* Storage Ring */
  EvgSyncMxc(pEg);
  EvgSetDBusMap(pEg, 0, C_EVG_DBUS_SEL_MXC);

  /*  EvrSetUnivOutMap(pEr, 0, C_EVR_SIGNAL_MAP_DBUS); */

  for (i = 0; i < 1024; i++)
    {
      EvrUnivDlySetDelay(pEr, 0, i, 1023-i);
      EvrUnivDlySetDelay(pEr, 1, 1023-i, 1023-i);
      usleep(1000);
    }
   for (i = 0; i < 1024; i++)
    {
      EvrUnivDlySetDelay(pEr, 0, 1023-i, i);
      EvrUnivDlySetDelay(pEr, 1, 1023-i, 1023-i);
      usleep(1000);
    }
  for (i = 0; i < 1024; i++)
    {
      EvrUnivDlySetDelay(pEr, 0, 1023-i, 1023-i);
      EvrUnivDlySetDelay(pEr, 1, i, 1023-i);
      usleep(1000);
    }
   for (i = 0; i < 1024; i++)
    {
      EvrUnivDlySetDelay(pEr, 0, 1023-i, 1023-i);
      EvrUnivDlySetDelay(pEr, 1, 1023-i, i);
      usleep(1000);
    }

   /*
  EvrEnable(pEr, 0);
  if (EvrGetEnable(pEr))
    {
      printf("Could not disable EVR!\n");
      err_gen++;
      return -1;
    }
  EvgEnable(pEg, 0);
  if (EvgGetEnable(pEg))
    {
      printf("Could not disable EVG!\n");
      err_gen++;
      return -1;
    }
   */

  return 0;
}

int toggle_fp(struct MrfErRegs *pEr)
{
  /*
   * Toggle PMC-EVR-230 front panel outputs
   */

  int i;

  EvrEnable(pEr, 1);
  if (!EvrGetEnable(pEr))
    {
      printf("Could not enable EVR!\n");
      err_gen++;
      return -1;
    }

  for (i = 0; i < 10; i++)
    {
      EvrSetFPOutMap(pEr, 0, C_EVR_SIGNAL_MAP_HIGH);
      EvrSetFPOutMap(pEr, 1, C_EVR_SIGNAL_MAP_LOW);
      EvrSetFPOutMap(pEr, 2, C_EVR_SIGNAL_MAP_LOW);
      usleep(50000);
      EvrSetFPOutMap(pEr, 1, C_EVR_SIGNAL_MAP_HIGH);
      EvrSetFPOutMap(pEr, 2, C_EVR_SIGNAL_MAP_LOW);
      EvrSetFPOutMap(pEr, 0, C_EVR_SIGNAL_MAP_LOW);
      usleep(50000);
      EvrSetFPOutMap(pEr, 2, C_EVR_SIGNAL_MAP_HIGH);
      EvrSetFPOutMap(pEr, 0, C_EVR_SIGNAL_MAP_LOW);
      EvrSetFPOutMap(pEr, 1, C_EVR_SIGNAL_MAP_LOW);
      usleep(50000);
    }
  return 0;
}

int main(int argc, char *argv[])
{
  struct MrfEgRegs *pEg;
  struct MrfErRegs *pEr;
  struct MrfErRegs *pBEr;
  int              fdEg;
  int              fdEr;
  int              fdBEr;
  int              i,j;

  if (argc < 3)
    {
      printf("Usage: %s /dev/ega3 /dev/era3 /dev/erd3\n", argv[0]);
      printf("Assuming: /dev/ega3, /dev/era3 and /dev/erd3\n");
      argv[1] = "/dev/ega3";
      argv[2] = "/dev/era3";
      argv[3] = "/dev/erd3";
    }

  fdEg = EvgOpen(&pEg, argv[1]);
  if (fdEg == -1)
    return errno;

  /*
  EvgSetRFInput(pEg, 0, C_EVG_RFDIV_4);
  */

  fdEr = EvrOpen(&pEr, argv[2]);
  if (fdEr == -1)
    {
      EvgClose(fdEg);
      return errno;
    }

  fdBEr = EvrOpen(&pBEr, argv[3]);
  if (fdBEr == -1)
    {
      EvgClose(fdEg);
      EvgClose(fdEr);
      return errno;
    }

  pIrqEr = pEr;
  fdIrqEr = fdEr;
  EvrIrqAssignHandler(pIrqEr, fdIrqEr, &evr_irq_handler);

  do
    {
      printf("cPCI-EVG-230 and cPCI-EVR-230 test\n\n");
      printf("1. run testloop\n");
      printf("2. run 10 testloops\n");
      printf("3. run dlytest for UNIV-LVPECL-DLY modules\n");
      printf("4. toggle PMC-EVR-230 front panel outputs\n");
      printf("5. exit\n");

      do {
	i = getchar();
      }
      while (i == '\n');

      switch(i)
	{
	case '1':
	  clear_errors();
	  testloop(pEg, pEr, pBEr);
	  dump_errors();
	  break;
	case '2':
	  clear_errors();
	  for (j = 0; j < 10; j++)
	    testloop(pEg, pEr, pBEr);
	  dump_errors();
	  break;
	case '3':
	  dlytest(pEg, pEr);
	  break;
	case '4':
	  toggle_fp(pEr);
	  break;
	}
    }
  while(i != '5');

#if 0
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

  EvgSetUnivinMap(pEg, 0, -1, 0);
  EvgSetUnivinMap(pEg, 1, -1, 1);
  EvgSetUnivinMap(pEg, 2, -1, 2);
  EvgSetUnivinMap(pEg, 3, -1, 3);
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

#endif
  EvrClose(fdEr);
  EvgClose(fdEg);
}

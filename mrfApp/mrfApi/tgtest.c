/*
  tgtest.c -- Micro-Research Event Receiver TG
              Factory Test

  Author: Jukka Pietarinen (MRF)
  Date:   11.5.2010

  Module Setup:

*/

#include <byteswap.h>
#include <errno.h>
#include <stdio.h>
#include <time.h>
#include <sys/ioctl.h>
#include <signal.h>
#include <unistd.h>
#include "egcpci.h"
#include "egapi.h"
#include "erapi.h"

#define FRAC_DIV_WORD 0x0C928166
/*
#define FRAC_DIV_WORD 0x00DE816D
*/

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
int err_log;
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
  err_log = 0;
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
  printf("Log errors       %d\n", err_log);
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
  time_t t;

  flags = EvrGetIrqFlags(pIrqEr);

  t = time(NULL);
  printf("IRQ %08x %s", flags, asctime(localtime(&t)));

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

int mytest(struct MrfErRegs *pEr)
{
  int i, j, k, ff;

  EvrEnable(pEr, 1);
  if (!EvrGetEnable(pEr))
    {
      printf(ERROR_TEXT "Could not enable EVR!\n");
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
    default:
      printf("Testing form factor %d\n", ff);
    }

  printf("1\n");

  EvrSetFracDiv(pEr, FRAC_DIV_WORD);

  sleep(1);

  printf("2\n");

  /* Clear violation flag */
  EvrGetViolation(pEr, 1);

  usleep(1000);

  printf("3\n");

  if (EvrGetViolation(pEr, 1))
    {
      err_vio++;
      printf(ERROR_TEXT "Could not clear violation flag!\n");
    }

  /* Setup EVR map RAM */

  printf("4\n");

  //EvrEnableRam(pEr, 1);
  EvrSetFineDelay(pEr,0, 0);
  EvrOutputEnable(pEr, 1);
  for (i = 1; i < 11; i++)
    {
      EvrSetLedEvent(pEr, 0, i, 1);
      EvrSetLedEvent(pEr, 1, i, 1);
      EvrSetPulseMap(pEr, 0, i, i-1, -1, -1);
      EvrSetPulseMap(pEr, 1, i, i-1, -1, -1);
      EvrSetPulseParams(pEr, i-1, 1, 0, 1);
      EvrSetPulseProperties(pEr, i-1, 0, 0, 0, 1, 1);
      EvrSetUnivOutMap(pEr, i-1, i-1);
      EvrSetFPOutMap(pEr, i-1, i-1);
      EvrSetTBOutMap(pEr, i-1, i-1);
    }

  printf("5\n");
///////////////
  EvrMapRamEnable(pEr, 0, 1);
  EvrEnable(pEr, 1);
  EvrOutputEnable(pEr, 1);
  for (i=0 ; i<8 ; i++) {
      EvrCMLEnable(pEr, i, 1);
      EvrSetFPOutMap(pEr, i, 0);
  }

  EvrEnable(pEr, 1);
  EvrSetFineDelay(pEr, 0, 0);
/////////////////////

  EvrDumpPulses(pEr, 10);
  EvrDumpMapRam(pEr, 0);

  return 0;
}


int testloop(struct MrfErRegs *pEr)
{
  int i, j, k, ff;

  EvrEnable(pEr, 1);
  if (!EvrGetEnable(pEr))
    {
      printf(ERROR_TEXT "Could not enable EVR!\n");
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
    default:
      printf("Testing form factor %d\n", ff);
    }

  printf("1\n");
  getchar();

  EvrSetFracDiv(pEr, FRAC_DIV_WORD);

  sleep(1);

  printf("2\n");
  getchar();

  /* Clear violation flag */
  EvrGetViolation(pEr, 1);

  usleep(1000);

  printf("3\n");
  getchar();

  if (EvrGetViolation(pEr, 1))
    {
      err_vio++;
      printf(ERROR_TEXT "Could not clear violation flag!\n");
    }

  /* Setup EVR map RAM */

  printf("4\n");
  getchar();

  EvrSetFineDelay(pEr,0, 0);
  EvrOutputEnable(pEr, 1);
  for (i = 1; i < 11; i++)
    {
      EvrSetLedEvent(pEr, 0, i, 1);
      EvrSetPulseMap(pEr, 0, i, i-1, -1, -1);
      EvrSetPulseMap(pEr, 1, i, i-1, -1, -1);
      EvrSetPulseParams(pEr, i-1, 1, 0, 100);
      EvrSetPulseProperties(pEr, i-1, 0, 0, 0, 1, 1);
      EvrSetUnivOutMap(pEr, i-1, i-1);
      EvrSetFPOutMap(pEr, i-1, i-1);
      EvrSetTBOutMap(pEr, i-1, i-1);
    }

  printf("5\n");
  getchar();

  EvrMapRamEnable(pEr, 0, 1);

  printf("6\n");
  getchar();

  EvrUnivDlyEnable(pEr, 0, 1);

  printf("7\n");
  getchar();
  EvrUnivDlyEnable(pEr, 1, 1);
  printf("8\n");
  getchar();
  EvrUnivDlySetDelay(pEr, 0, 0, 0);
  printf("9\n");
  getchar();
  EvrUnivDlySetDelay(pEr, 1, 0, 0);

  printf("10\n");
  getchar();

  EvrDumpPulses(pEr, 10);
  EvrDumpMapRam(pEr, 0);

  return 0;
}

int dlytest(struct MrfErRegs *pEr)
{
  int i, j, c;

  EvrEnable(pEr, 1);
  EvrOutputEnable(pEr, 1);
  if (!EvrGetEnable(pEr))
    {
      printf("Could not enable EVR!\n");
      err_gen++;
      return -1;
    }

  for (i = 0; i < 8; i++)
    {
      EvrCMLEnable(pEr, i, 1);
    }

  /* Output 0UT0 on FP0 to FP7 */
  EvrSetFPOutMap(pEr, 0, 0);
  EvrSetFPOutMap(pEr, 1, 0);
  EvrSetFPOutMap(pEr, 2, 0);
  EvrSetFPOutMap(pEr, 3, 0);
  EvrSetFPOutMap(pEr, 4, 0);
  EvrSetFPOutMap(pEr, 5, 0);
  EvrSetFPOutMap(pEr, 6, 0);
  EvrSetFPOutMap(pEr, 7, 0);

  /*  EvrSetUnivOutMap(pEr, 0, C_EVR_SIGNAL_MAP_DBUS); */

  do
    {
      printf("Delay adjust test\n\n");
      printf("Select channel 1-8 (0 - exit) ?");
      
      do {
	c = getchar();
      }
      while (c == '\n');

      if (c >= '1' && c <= '8')
	{
	  j = c - '1';
	  for (i = 0; i <= 1024; i++)
	    {
	      EvrSetFineDelay(pEr, j, i % 1024);
	      usleep(1000);
	    }
	}
    }
  while (c != '0');

  EvrEnable(pEr, 0);
  if (EvrGetEnable(pEr))
    {
      printf("Could not disable EVR!\n");
      err_gen++;
      return -1;
    }

  return 0;
}

int irqtest(struct MrfErRegs *pEr, int fdEr)
{
  evr_clr_irq();
  EvrIrqEnable(pEr, 0);

  printf("Enabling Evr: %08x\n", EvrGetEnable(pEr));
  printf("Violation: %08x\n", EvrGetViolation(pEr, 1));
  printf("Violation after clearing: %08x\n", EvrGetViolation(pEr, 0));

  printf("EvrIrqEnable\n");
  EvrIrqEnable(pEr, EVR_IRQ_MASTER_ENABLE | EVR_IRQFLAG_HEARTBEAT);
  /* Here we call sleep a number of times because sleep is interrupted
     when signal (user mode interrupt) is received. */
  sleep(3);
  printf("Sleep 3...\n");
  sleep(3);
  printf("Sleep 3...\n");
  sleep(3);
  printf("Sleep 3...\n");
  sleep(3);
  printf("Sleep 3...\n");
  sleep(3);
  printf("Sleep 3...\n");
  sleep(3);
  printf("Sleep 3...\n");
  sleep(3);
  printf("Sleep 3...\n");
  sleep(3);
  printf("Sleep 3...\n");
  sleep(3);
  printf("Sleep 3...\n");
  sleep(3);
  printf("Sleep 3...\n");
  EvrIrqEnable(pEr, 0);

  return 0;
}

int guntest(struct MrfErRegs *pEr)
{
  int i, j, k, ff;

  EvrEnable(pEr, 1);
  if (!EvrGetEnable(pEr))
    {
      printf(ERROR_TEXT "Could not enable EVR!\n");
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
    case 4: printf("Testing cPCI-EVRTG-300\n");
      break;
    default:
      printf("Testing form factor %d\n", ff);
    }

  EvrSetFracDiv(pEr, FRAC_DIV_WORD);

  sleep(1);

  /* Clear violation flag */
  EvrGetViolation(pEr, 1);

  usleep(1000);

  if (EvrGetViolation(pEr, 1))
    {
      err_vio++;
      printf(ERROR_TEXT "Could not clear violation flag!\n");
    }

  /* Setup EVR map RAM */

  //EvrEnableRam(pEr, 1);
  EvrSetFineDelay(pEr,0, 0);
  EvrOutputEnable(pEr, 1);
  for (i = 1; i < 11; i++)
    {
      EvrSetLedEvent(pEr, 0, i, 1);
      EvrSetLedEvent(pEr, 1, i, 1);
      EvrSetPulseParams(pEr, i-1, 0, 0, 100);
      EvrSetPulseProperties(pEr, i-1, 0, 0, 0, 1, 1);
    }

  EvrSetFPOutMap(pEr, 0, 0x20);
  EvrSetFPOutMap(pEr, 1, 0x21);
  EvrSetFPOutMap(pEr, 2, 0x00);
  EvrSetFPOutMap(pEr, 3, 0x21);
  EvrSetFPOutMap(pEr, 4, 0x00);
  EvrSetFPOutMap(pEr, 5, 0x21);
  EvrSetFPOutMap(pEr, 6, 0x00);
  EvrSetFPOutMap(pEr, 7, 0x01);

  EvrSetPulseMap(pEr, 0, 1, 0, -1, -1);
  EvrSetPulseMap(pEr, 0, 1, 1, -1, -1);
  EvrMapRamEnable(pEr, 0, 1);
  EvrEnable(pEr, 1);
  EvrOutputEnable(pEr, 1);
  for (i=0 ; i<8 ; i++) {
      EvrCMLEnable(pEr, i, 1);
  }
  EvrSetCMLMode(pEr, 6, C_EVR_CMLCTRL_MODE_GUNTX300);
  EvrSetCMLMode(pEr, 7, C_EVR_CMLCTRL_MODE_GUNTX300);

  EvrEnable(pEr, 1);
  EvrSetFineDelay(pEr, 0, 0);
/////////////////////

  EvrDumpPulses(pEr, 10);
  EvrDumpMapRam(pEr, 0);

  return 0;
}

int gunpattern(struct MrfErRegs *pEr)
{
  pEr->CML[6].SamplePos = be32_to_cpu(0x100);
  pEr->CML[7].SamplePos = be32_to_cpu(0x100);

  EvrSetCMLMode(pEr, 6, C_EVR_CMLCTRL_MODE_GUNTX300 | C_EVR_CMLCTRL_MODE_PATTERN);
  EvrSetCMLMode(pEr, 7, C_EVR_CMLCTRL_MODE_GUNTX300 | C_EVR_CMLCTRL_MODE_PATTERN);

  return 0;
}

int main(int argc, char *argv[])
{
  struct MrfErRegs *pEr;
  int              fdEr;
  int              i,j,k;

  if (argc < 2)
    {
      printf("Usage: %s dev/evrtga3\n", argv[0]);
      printf("Assuming: /dev/evrtga3\n");
      argv[1] = "/dev/evrtga3";
    }

  fdEr = EvrOpen(&pEr, (char *) argv[1]);
  if (fdEr == -1)
    {
      return errno;
    }

  pIrqEr = pEr;
  fdIrqEr = fdEr;
  EvrIrqAssignHandler(pIrqEr, fdIrqEr, &evr_irq_handler);

  do
    {
      printf("cPCI-EVRTG-300 test\n\n");
      printf("0. my test\n");
      printf("1. run testloop\n");
      printf("2. run 10 testloops\n");
      printf("3. run dlytest for UNIV-LVPECL-DLY modules\n");
      printf("5. HB irqtest\n");
      printf("6. Gun-TX-300 test\n");
      printf("7. Gun-TX-300 pattern test\n");
      printf("8. exit\n");

      do {
	i = getchar();
      }
      while (i == '\n');

      switch(i)
	{
	case '0':
	  mytest(pEr);
	  break;
	case '1':
	  clear_errors();
	  testloop(pEr);
	  dump_errors();
	  break;
	case '2':
	  clear_errors();
	  for (j = 0; j < 10; j++)
	    testloop(pEr);
	  dump_errors();
	  break;
	case '3':
	  dlytest(pEr);
	  break;
	case '4':
	  j = pEr->CML[0].LowPeriod;
	  k = pEr->CML[0].HighPeriod;
	  k = pEr->CML[0].Control = 0x0100;
	  k = pEr->CML[1].Control = 0x0100;
	  k = pEr->CML[2].Control = 0x0100;
	  k = pEr->CML[3].Control = 0x0100;
	  k = pEr->CML[4].Control = 0x0100;
	  k = pEr->CML[5].Control = 0x0100;
	  k = pEr->CML[6].Control = 0x0100;
	  k = pEr->CML[7].Control = 0x0100;
	  pEr->CML[0].LowPeriod = be16_to_cpu(0x1234);
	  pEr->CML[0].HighPeriod = be16_to_cpu(0x5678);
	  printf("0 : %08x, 1: %08x\n", j, k);
	  break;
	case '5':
	  irqtest(pEr, fdEr);
	  break;
	case '6':
	  guntest(pEr);
	  break;
	case '7':
	  gunpattern(pEr);
	  break;
	}
    }
  while(i != '8');

  EvrClose(fdEr);
}

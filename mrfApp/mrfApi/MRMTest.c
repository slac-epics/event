/*
  MRMTest.c -- Micro-Research Event Generator and Event Receiver
               Factory Test for Modular Register Mapping

  Author: Jukka Pietarinen (MRF)
  Date:   24.08.2009

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

  VME-EVG-230:
  VME-EVR-230:
*/

#ifdef __linux__
#include <errno.h>
#include <stdio.h>
#include <time.h>
#include <sys/ioctl.h>
#include <signal.h>
#include <unistd.h>
#include <endian.h>
#include <byteswap.h>
#include "egcpci.h"
#else
#include <sysLib.h>
#include "egvme.h"
#endif
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include "egapi.h"
#include "erapi.h"

#ifndef __linux__
#define usleep(x) sysUsDelay(x)
void sysUsDelay(int);
#endif

#define DEBUG_PRINTF printf

#if 1
#define USE_EXT_RF 1
#endif

#define FRAC_DIV_WORD 0x0C928166
#ifndef __linux__
#define VME 1
#endif

#ifdef VME
/* this is from target/config/mv2306/sl82565IntrCtl.h 
 * this should be incorporated into the build structure!
 * (but I just copied it here )
*/
typedef struct intHandlerDesc           /* interrupt handler desciption */
    {
    VOIDFUNCPTR                 vec;    /* interrupt vector */
    int                         arg;    /* interrupt handler argument */
    struct intHandlerDesc *     next;   /* next interrupt handler & argument */
    } INT_HANDLER_DESC;

/*#include <ravenMpic.h>*/

extern INT_HANDLER_DESC * sysIntTbl [256];
INT_HANDLER_DESC * ppcShowIvecChain(INT_HANDLER_DESC *vector);

int ppcDisconnectVec(int vecnum, VOIDFUNCPTR func);
#endif

/* Capabilities */
int evg_sequence_rams = -1;
int evg_event_triggers = -1;
int evg_multiplexed_counters = -1;
int evg_ttl_inputs = -1;
int evg_ttl_outputs = -1;
int evg_univ_inputs = -1;
int evg_univ_outputs = -1;
int evg_tb_inputs = -1;
int evg_tb_outputs = -1;

int evr_pulse_gens = -1;
int evr_inputs = -1;
int evr_ttl_inputs = -1;
int evr_ttl_outputs = -1;
int evr_cml_outputs = -1;
int evr_univ_inputs = -1;
int evr_univ_outputs = -1;
int evr_gpios = -1;
int evr_tb_outputs = -1;
int evr_pulse_presc_range[EVR_MAX_PULSES];
int evr_pulse_delay_range[EVR_MAX_PULSES];
int evr_pulse_width_range[EVR_MAX_PULSES];
int evr_prescalers = -1;
int evr_presc_range[EVR_MAX_PRESCALERS];
int evr_diag_counters = -1;
int evr_diag_counter_bits[EVR_DIAG_MAX_COUNTERS];

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

int dump_errors()
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

  return (err_gen +
	  err_vio +
	  err_pulse +
	  err_evan +
	  err_mxc +
	  err_dbus +
	  err_acin +
	  err_seq +
	  err_dbuf +
	  err_irq +
	  err_fifo +
	  err_log +
	  err_extev +
	  err_bev +
	  err_bdbus +
	  err_bdbuf +
	  err_fwd);
}

int evg_irq_cnt[C_EVG_NUM_IRQ];

void evg_dump_irq()
{
  int i;

  for (i = 0; i < C_EVG_NUM_IRQ; i++)
    printf("EVG IRQ%d %d\n", i, evg_irq_cnt[i]);
}

void evg_clr_irq()
{
  int i;

  for (i = 0; i < C_EVG_NUM_IRQ; i++)
    evg_irq_cnt[i] = 0;
}

struct MrfEgRegs *pIrqEg;
int              fdIrqEg;

void evg_irq_handler(int param)
{
  int flags, i;
  time_t t;

  flags = EvgGetIrqFlags(pIrqEg);

  /*
  if (flags & ~1)
    {
      t = time(NULL);
      printf("EVG IRQ %08x %s", flags, asctime(localtime(&t)));
    }
  */

  for (i = 0; i < C_EVG_NUM_IRQ; i++)
    {
      if (flags & (1 << i))
	{
	  /*
	  if (i > 7)
	    {
	      t = time(NULL);
	      printf("EVG IRQ %d %08x %s", i, flags, asctime(localtime(&t)));
	    }
	  */
	  evg_irq_cnt[i]++;
	  /*
	  printf("EVG IRQ %d %d\n", i, evg_irq_cnt[i]);
	  */
	  EvgClearIrqFlags(pIrqEg, (1 << i));
	}
    }

  flags = EvgGetIrqFlags(pIrqEg);
  /*
  if (flags & ~1)
    printf("EVG IRQ flags %08x\n", flags);
  */

#ifdef __linux__
  EvgIrqHandled(fdIrqEg);
#endif
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
  struct FIFOEvent fe;

  flags = EvrGetIrqFlags(pIrqEr);

  /*
  t = time(NULL);
  printf("EVR IRQ %08x %s", flags, asctime(localtime(&t)));
  */

  for (i = 0; i < C_EVR_NUM_IRQ; i++)
    {
      if (flags & (1 << i))
	{
	  evr_irq_cnt[i]++;
	  switch(i)
	    {
	    case C_EVR_IRQFLAG_EVENT:
	      EvrGetFIFOEvent(pIrqEr, &fe);
	      break;
	    case C_EVR_IRQFLAG_PULSE:
	      /* If we get a pulse IRQ we need to disable the irq so that
		 we so not get interrupted again when re-enabling interrupts. */
	      EvrIrqEnable(pIrqEr, EvrGetIrqEnable(pIrqEr) & ~(1 << C_EVR_IRQFLAG_PULSE));
	      break;
	    case C_EVR_IRQFLAG_DATABUF:
	      EvrReceiveDBuf(pIrqEr, 1);
	      break;
	    default:
	      EvrClearIrqFlags(pIrqEr, (1 << i));
	    }
	}
    }

  /* We need to call EVG handler as well */
  evg_irq_handler(param);

#ifdef __linux__
  EvrIrqHandled(fdIrqEr);
#endif
}

#define ERROR_TEXT "*** "

int EvgProbe(struct MrfEgRegs *pEg)
{
  int i;

  for (i = 0; i < EVG_SEQRAMS; i++)
    {
      unsigned int probe_ts = 0x55aa6699;
      int probe_code = 0x11;

      EvgSetSeqRamEvent(pEg, i, 0, probe_ts, probe_code);
      if (EvgGetSeqRamTimestamp(pEg, i, 0) != probe_ts ||
	  EvgGetSeqRamEvent(pEg, i, 0) != probe_code)
	break;
    }

  evg_sequence_rams = i;
  DEBUG_PRINTF("EVG: probed %d Sequence Rams.\n", evg_sequence_rams);

  for (i = 0; i < EVG_MAX_TRIGGERS; i++)
    {
      EvgSetTriggerEvent(pEg, i, i+1, 1);
      if (EvgGetTriggerEventCode(pEg, i) != i+1 ||
	  EvgGetTriggerEventEnable(pEg, i) != 1)
	break;
    }

  evg_event_triggers = i;
  DEBUG_PRINTF("EVG: probed %d Event Triggers.\n", evg_event_triggers);

  for (i = EVG_MAX_MXCS-1; i >= 0; i--)
    {
      EvgSetMXCPrescaler(pEg, i, 1 << i);
    }

  for (i = 0; i < EVG_MAX_MXCS; i++)
    {
      if (EvgGetMXCPrescaler(pEg, i) != (1 << i))
	break;
    }

  evg_multiplexed_counters = i;
  DEBUG_PRINTF("EVG: probed %d Multiplexed Counters.\n", evg_multiplexed_counters);

  for (i = EVG_MAX_FPIN_MAP-1; i >= 0; i--)
    {
      EvgSetFPinMap(pEg, i, i, i, 1, (i % 2));
    }

  for (i = 0; i < EVG_MAX_FPIN_MAP; i++)
    {
      /*      DEBUG_PRINTF("EVG: %d Trig %d, DBUS %d, Irq %d, Seqtrig %d\n", i,
		   EvgGetFPinMapTrigger(pEg, i),
		   EvgGetFPinMapDBus(pEg, i),
		   EvgGetFPinMapIrq(pEg, i),
		   EvgGetFPinMapSeqtrig(pEg, i)); */
      if (EvgGetFPinMapTrigger(pEg, i) != i ||
	  EvgGetFPinMapDBus(pEg, i) != i ||
	  EvgGetFPinMapIrq(pEg, i) != 1 ||
	  EvgGetFPinMapSeqtrig(pEg, i) != (i % 2))
	break;
      EvgSetFPinMap(pEg, i, -1, -1, 0, -1);
    }

  evg_ttl_inputs = i;
  DEBUG_PRINTF("EVG: probed %d Front Panel TTL Inputs.\n", evg_ttl_inputs);

  for (i = EVG_MAX_UNIVIN_MAP-1; i >= 0; i--)
    {
      EvgSetUnivinMap(pEg, i, i, i, 1, (i % 2));
    }

  /*  EvgUnivinDump(pEg); */

  for (i = 0; i < EVG_MAX_UNIVIN_MAP; i++)
    {
      /*      DEBUG_PRINTF("EVG: %d Trig %d, DBUS %d, Irq %d, Seqtrig %d\n", i,
		   EvgGetUnivinMapTrigger(pEg, i),
		   EvgGetUnivinMapDBus(pEg, i),
		   EvgGetUnivinMapIrq(pEg, i),
		   EvgGetUnivinMapSeqtrig(pEg, i)); */
      if (EvgGetUnivinMapTrigger(pEg, i) != i ||
	  EvgGetUnivinMapDBus(pEg, i) != i ||
	  EvgGetUnivinMapIrq(pEg, i) != 1 ||
	  EvgGetUnivinMapSeqtrig(pEg, i) != (i % 2))
	break;
      EvgSetUnivinMap(pEg, i, -1, -1, 0, -1);
    }

  evg_univ_inputs = i;
  DEBUG_PRINTF("EVG: probed %d Front Panel Universal Inputs.\n", evg_univ_inputs);

  for (i = EVG_MAX_TBIN_MAP-1; i >= 0; i--)
    {
      pEg->TBInMap[i] = be32_to_cpu(i + (i << 16));
      EvgSetTBinMap(pEg, i, i & 7, i & 7, 1, (i % 2));
      /*
	printf("%2d wrote %08x, read %08x; ", i, i + (i << 16),
	be32_to_cpu(pEg->TBInMap[i]));  
	printf("%d, %04x %04x\n", i, EvgGetTBinMapTrigger(pEg, i),EvgGetTBinMapDBus(pEg, i) );
      */
    }

  /*
    for (i = EVG_MAX_TBIN_MAP-1; i >= 0; i--)
    {
    printf("%2d wrote %08x, read %08x\n", i, i + (i << 16),
    be32_to_cpu(pEg->TBInMap[i]));
    }
    
    EvgTBinDump(pEg);
  */

  /*  EvgTBinDump(pEg); */

  for (i = 0; i < EVG_MAX_TBIN_MAP; i++)
    {
      /*      DEBUG_PRINTF("EVG: %d Trig %d, DBUS %d, Irq %d, Seqtrig %d\n", i,
		   EvgGetTBinMapTrigger(pEg, i),
		   EvgGetTBinMapDBus(pEg, i),
		   EvgGetTBinMapIrq(pEg, i),
		   EvgGetTBinMapSeqtrig(pEg, i)); */
      if (EvgGetTBinMapTrigger(pEg, i) != (i & 7) ||
	  EvgGetTBinMapDBus(pEg, i) != (i & 7) ||
	  EvgGetTBinMapIrq(pEg, i) != 1 ||
	  EvgGetTBinMapSeqtrig(pEg, i) != (i % 2))
	break;
      EvgSetTBinMap(pEg, i, -1, -1, 0, -1);
    }

  evg_tb_inputs = i;
  DEBUG_PRINTF("EVG: probed %d Transition Board Inputs.\n", evg_tb_inputs);

  for (i = EVG_MAX_FPOUT_MAP-1; i >= 0; i--)
    {
      EvgSetFPOutMap(pEg, i, i+1);
    }

  for (i = 0; i < EVG_MAX_FPOUT_MAP; i++)
    {
      if (EvgGetFPOutMap(pEg, i) != i+1)
	break;
    }

  evg_ttl_outputs = i;
  DEBUG_PRINTF("EVG: probed %d Front Panel TTL Outputs.\n", evg_ttl_outputs);

  for (i = EVG_MAX_UNIVOUT_MAP-1; i >= 0; i--)
    {
      EvgSetUnivOutMap(pEg, i, i+1);
    }

  for (i = 0; i < EVG_MAX_UNIVOUT_MAP; i++)
    {
      if (EvgGetUnivOutMap(pEg, i) != i+1)
	break;
    }

  evg_univ_outputs = i;
  DEBUG_PRINTF("EVG: probed %d Front Panel Universal Outputs.\n", evg_univ_outputs);

  for (i = EVG_MAX_TBOUT_MAP-1; i >= 0; i--)
    {
      EvgSetTBOutMap(pEg, i, i+1);
    }

  for (i = 0; i < EVG_MAX_TBOUT_MAP; i++)
    {
      if (EvgGetTBOutMap(pEg, i) != i+1)
	break;
    }

  evg_tb_outputs = i;
  DEBUG_PRINTF("EVG: probed %d Transition Board Outputs.\n", evg_tb_outputs);
}

int EvrProbe(struct MrfErRegs *pEr)
{
  int i;

  evr_pulse_gens = 0;
  for (i = EVR_MAX_PULSES-1; i >= 0; i--)
    {
      int probe_presc = -1;
      int probe_delay = -1;
      int probe_width = -1;
      EvrSetPulseParams(pEr, i, probe_presc, probe_delay, probe_width);
    }

  for (i = 0; i < EVR_MAX_PULSES; i++)
    {
      evr_pulse_presc_range[i] = EvrGetPulsePresc(pEr, i);
      evr_pulse_delay_range[i] = EvrGetPulseDelay(pEr, i);
      evr_pulse_width_range[i] = EvrGetPulseWidth(pEr, i);
      /*
      printf("PULSE %2d: presc %08x, delay %08x, width %08x\n", i,
	     evr_pulse_presc_range[i], evr_pulse_delay_range[i],
	     evr_pulse_width_range[i]);
      */
      if (evr_pulse_presc_range[i] != 0 ||
	  /*	  evr_pulse_delay_range[i] != 0 || */
	  evr_pulse_width_range[i] != 0)
	evr_pulse_gens++;
      EvrSetPulseParams(pEr, i, 0, 0, 0);
   }

  DEBUG_PRINTF("EVR: Probed %d Pulse Generators.\n", evr_pulse_gens);

  for (i = EVR_MAX_FPIN_MAP-1; i >= 0; i--)
    {
      EvrSetExtEvent(pEr, i, i+1, 0, 0);
    }

  for (i = 0; i < EVR_MAX_FPIN_MAP; i++)
    {
      if (EvrGetExtEventCode(pEr, i) != i+1)
	break;
    }

  evr_inputs = i;
  DEBUG_PRINTF("EVR: probed %d Inputs.\n", evr_inputs);

  for (i = EVR_MAX_FPOUT_MAP-1; i >= 0; i--)
    {
      EvrSetFPOutMap(pEr, i, i+1);
    }

  for (i = 0; i < EVR_MAX_FPOUT_MAP; i++)
    {
      if (EvrGetFPOutMap(pEr, i) != i+1)
	break;
    }

  evr_ttl_outputs = i;
  DEBUG_PRINTF("EVR: probed %d Front Panel TTL Outputs.\n", evr_ttl_outputs);

  for (i = EVR_MAX_CML_OUTPUTS-1; i >= 0; i--)
    {
      pEr->CML[i].Pattern00 = be32_to_cpu(i+1);
      /*
      printf("CML %d, %08x, %08x\n", i, i+1, be32_to_cpu(pEr->CML[i].Pattern00));
      */
    }

  for (i = 0; i < EVR_MAX_CML_OUTPUTS; i++)
    {
      /*
      printf("CML %d, %08x, %08x\n", i, i+1, be32_to_cpu(pEr->CML[i].Pattern00) & 0x000fffff);
      */
      if ((be32_to_cpu(pEr->CML[i].Pattern00) & 0x000fffff) != i+1)
	break;
      pEr->CML[i].Pattern00 = be32_to_cpu(0);
    }

  evr_cml_outputs = i;
  DEBUG_PRINTF("EVR: probed %d Front Panel CML Outputs.\n", evr_cml_outputs);

  for (i = EVR_MAX_UNIVOUT_MAP-1; i >= 0; i--)
    {
      EvrSetUnivOutMap(pEr, i, i+1);
    }


  for (i = 0; i < EVR_MAX_UNIVOUT_MAP; i++)
    {
      if (EvrGetUnivOutMap(pEr, i) != i+1)
	break;
    }

  evr_univ_outputs = i;
  DEBUG_PRINTF("EVR: probed %d Front Panel Universal Outputs.\n", evr_univ_outputs);

  for (i = EVR_MAX_TBOUT_MAP-1; i >= 0; i--)
    {
      EvrSetTBOutMap(pEr, i, i+1);
    }

  for (i = 0; i < EVR_MAX_TBOUT_MAP; i++)
    {
      if (EvrGetTBOutMap(pEr, i) != i+1)
	break;
    }

  evr_tb_outputs = i;
  DEBUG_PRINTF("EVR: probed %d Transition Board Outputs.\n", evr_tb_outputs);

  {
    int gpio;
    /* Configure all GPIOS as inputs */
    EvrSetGPIODir(pEr, 0);
    gpio = EvrSetGPIOOut(pEr, 0xffffffff);
    
    for (i = 0; i < EVR_MAX_GPIOS; i++)
    {
      if (gpio == 0)
	break;
      gpio >>= 1;
    }

    evr_gpios = i;
    DEBUG_PRINTF("EVR: probed %d GPIOs.\n", evr_gpios);
  }

  evr_prescalers = 0;
  for (i = 0; i < EVR_MAX_PRESCALERS; i++)
    {
      EvrSetPrescaler(pEr, i, 0xffffffff);
    }

  for (i = 0; i < EVR_MAX_PRESCALERS; i++)
    {
      evr_presc_range[i] = EvrGetPrescaler(pEr, i);
      if (evr_presc_range[i])
	evr_prescalers++;
      else
	break;
      evr_presc_range[i] = EvrSetPrescaler(pEr, i, 0);
    }

  DEBUG_PRINTF("EVR: probed %d Prescalers.\n", evr_prescalers);

  {
    int diag;
    diag = EvrEnableDiagCounters(pEr, 1);
    
    for (i = 0; i < EVR_DIAG_MAX_COUNTERS; i++)
    {
      if (diag == 0)
	break;
      diag >>= 1;
    }

    evr_diag_counters = i;
    DEBUG_PRINTF("EVR: probed %d Diagnostics Counters.\n", evr_diag_counters);
  }

}

int MRMTest(struct MrfEgRegs *pEg, struct MrfErRegs *pEr,
	    struct MrfErRegs *pBEr)
{
  int i, j, k, evgff, evrff;

#ifndef __linux__
  EvgIrqAssignHandler(pEg, 0x61, evg_irq_handler);
  EvrIrqAssignHandler(pEr, 0x60, evr_irq_handler);
  pIrqEg = pEg;
  pIrqEr = pEr;
#endif

  EvgProbe((struct MrfEgRegs *) pEg);
  EvrProbe((struct MrfErRegs *) pEr);

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

  evgff = EvgGetFormFactor(pEg);
  switch (evgff)
    {
    case 0: printf("Testing cPCI-EVG-230\n");
      break;
    case 2: printf("Testing VME-EVG-230\n");
      break;
    case 4: printf("Testing cPCI-EVG-300\n");
      break;
    default:
      printf("Unknown form factor\n");
      err_gen++;
      break;
    }

  evrff = EvrGetFormFactor(pEr);
  switch (evrff)
    {
    case 0: printf("Testing cPCI-EVR-230\n");
      break;
    case 1: printf("Testing PMC-EVR-230\n");
      break;
    case 2: printf("Testing VME-EVR-230(RF)\n");
      break;
    case 4: printf("Testing cPCI-EVR-300\n");
      break;
    default:
      printf("Unknown form factor\n");
      err_gen++;
      break;
    }

  evg_clr_irq();
  EvgClearIrqFlags(pEg, -1);
  EvgIrqEnable(pEg, 0);

  EvgSetDBusMap(pEg, 0, 0);

  /*
  EvgSetFracDiv(pEg, FRAC_DIV_WORD);
  EvrSetFracDiv(pEr, FRAC_DIV_WORD);
  EvrSetFracDiv(pBEr, FRAC_DIV_WORD);

  sleep(1);
  */

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

  {
    /* Build configuration for EVR map RAMS */

    int ram,code;

    for (ram = 0; ram < EVR_MAPRAMS; ram++)
      {
	for (i = 0; i < evr_pulse_gens; i++)
	  {
	    code = ram * evr_pulse_gens + i + 1;
	    EvrSetLedEvent(pEr, ram, code, 1);
	    /* Pulse Triggers start at code 1 */
	    EvrSetPulseMap(pEr, ram, code, i, -1, -1);
	    /* Set Level start at code 0x81 */
	    EvrSetPulseMap(pEr, ram, code + 0x80, -1, i, -1);
	    /* Clear Level start at code 0xC1 */
	    EvrSetPulseMap(pEr, ram, code + 0xC0, -1, -1, i);
	  }

	for (i = 0; i <= EVR_MAX_EVENT_CODE; i++)
	  EvrSetLogEvent(pEr, ram, i, 1);
      }
    
    for (i = 0; i < evr_pulse_gens; i++)
      {
	EvrSetPulseParams(pEr, i, 1, 0, 100);
	EvrSetPulseProperties(pEr, i, 0, 0, 0, 1, 1);
	EvrSetUnivOutMap(pEr, i, i);
	EvrSetTBOutMap(pEr, i, i);
	EvrSetFPOutMap(pEr, i, i);
      }
    /*
    EvrDumpPulses(pEr, evr_pulse_gens);
    */
  }
  
  EvrMapRamEnable(pEr, 0, 1);
  if (evr_gpios)
    {
      EvrUnivDlyEnable(pEr, 0, 1);
      EvrUnivDlyEnable(pEr, 1, 1);
      EvrUnivDlySetDelay(pEr, 0, 0, 0);
      EvrUnivDlySetDelay(pEr, 1, 0, 0);
    }
  
  for (i = 1; i <= evr_pulse_gens; i++)
    {
      EvrSetLedEvent(pBEr, 0, i, 1);
      EvrSetPulseMap(pBEr, 0, i, i-1, -1, -1);
      EvrSetPulseParams(pBEr, i-1, 1, 0, 10);
      EvrSetPulseProperties(pBEr, i-1, 0, 0, 0, 1, 1);
      EvrSetUnivOutMap(pBEr, i-1, i-1);
      EvrSetTBOutMap(pBEr, i-1, i-1);
    }
  
  EvrMapRamEnable(pBEr, 0, 1);
  EvrUnivDlyEnable(pBEr, 0, 1);
  EvrUnivDlyEnable(pBEr, 1, 1);
  EvrUnivDlySetDelay(pBEr, 0, 0, 0);
  EvrUnivDlySetDelay(pBEr, 1, 0, 0);

  /* Test SW Event */
  
  printf("Test SW event -> EVR UNIV/TBOUT\n");
  
  EvgSWEventEnable(pEg, 1);
  if (!EvgGetSWEventEnable(pEg))
    {
      printf(ERROR_TEXT "Could not enable EVG SWEvent!\n");      
      err_gen++;
    }

  /*
  EvrClearLog(pEr);
  EvrEnableLog(pEr, 1);
  */

  /*  evr_pulse_gens = 1; */

  for (i = 1; i <= evr_pulse_gens; i++)
    {
      EvrClearDiagCounters(pEr);
      EvrEnableDiagCounters(pEr, 1);
      
#define PULSES 10000
      
      for (j = 0; j < PULSES; j++)
	EvgSendSWEvent(pEg, i);
      
      usleep(1000);
      
      for (j = 1; j <= evr_pulse_gens; j++)
	{
	  if (j != i)
	    {
	      if (EvrGetDiagCounter(pEr, j-1))
		{
		  printf(ERROR_TEXT "1 Spurious pulses on OUT%d: %ld, i = %d, j = %d\n", j-1,
			 EvrGetDiagCounter(pEr, j-1), i, j);
		  err_pulse++;
		}
	    }
	  else if (EvrGetDiagCounter(pEr, j-1) != PULSES)
	    {
	      printf(ERROR_TEXT "1 Wrong number of pulses on OUT%d: %ld (%d), i = %d, j = %d\n",
		     j-1, EvrGetDiagCounter(pEr, j-1), PULSES, i, j);
	      err_pulse++;
	    }
	}
    }

  /*
  EvrDumpLog(pEr);

  printf("******\n");

  EvrClearLog(pEr);

  for (i = 0; i <= 255; i++)
    EvgSendSWEvent(pEg, i);

  EvrDumpLog(pEr);
  */

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
	    printf(ERROR_TEXT "Evan: wrong event code %02x/%02x\n", (int) ev2.EventCode, j);
	    err_evan++;
	  }
	if (ev2.TimestampLow <= ev1.TimestampLow)
	  {
	    printf(ERROR_TEXT "Evan: Timestamp error %d - %d\n", (int) ev1.TimestampLow, (int) ev2.TimestampLow);
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

    usleep(2000);
    
    /*
    EvrEnableDiagCounters(pEr, 0);
    */    

    for (i = 0; i < 8; i++)
      {
	cnt[i] = EvrGetDiagCounter(pEr, i);

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
	if (evgff == 0 || evgff == 4)
	  EvgSetUnivinMap(pEg, i-1, -1, i-1, 0, -1);
	if (evgff == 2)
	  EvgSetTBinMap(pEg, i-1, -1, i-1, 0, -1);
      }

    for (i = 1; i <= 8; i++)
      {
	if (evgff == 0 || evgff == 4)
	  EvgSetUnivOutMap(pEg, i-1, C_SIGNAL_MAP_LOW);
	if (evgff == 2)
	  EvgSetTBOutMap(pEg, i-1, C_SIGNAL_MAP_LOW);
      }

    EvrClearDiagCounters(pEr);
    EvrEnableDiagCounters(pEr, 1);

    /* Generate pulses */
    for (i = 1; i <= 8; i++)
      for (j = 0; j < (1024 >> i); j++)
	{
	  if (evgff == 0 || evgff == 4)
	    {
	      EvgSetUnivOutMap(pEg, i-1, C_SIGNAL_MAP_LOW);
	      EvgSetUnivOutMap(pEg, i-1, C_SIGNAL_MAP_HIGH);
	      EvgSetUnivOutMap(pEg, i-1, C_SIGNAL_MAP_LOW);
	    }
	  if (evgff == 2)
	    {
	      EvgSetTBOutMap(pEg, i-1, C_SIGNAL_MAP_LOW);
	      EvgSetTBOutMap(pEg, i-1, C_SIGNAL_MAP_HIGH);
	      EvgSetTBOutMap(pEg, i-1, C_SIGNAL_MAP_LOW);
	    }
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
	EvgSetUnivinMap(pEg, i-1, -1, -1, 0, -1);
	EvgSetTBinMap(pEg, i-1, -1, -1, 0, -1);
      }
  }

  /*
   * AC Input tests
   */

  printf("AC Input tests\n");

  {
    int cnt;

    /* Lets put 50 Hz on MXC3 */
    EvgSetMXCPrescaler(pEg, 3, 499.654E6/4/50);
    EvgSyncMxc(pEg);
    if (evgff == 0 || evgff == 4)
      {
	/* cPCI: output on UNIVOUT0 -> LEMO Loopback to ACIN */
	EvgSetUnivOutMap(pEg, 0, C_SIGNAL_MAP_PRESC+3);
      }
    if (evgff == 2)
      {
	/* VME: output on TTLOUT0 -> LEMO Loopback to ACIN */
	EvgSetFPOutMap(pEg, 0, C_SIGNAL_MAP_PRESC+3);
      }

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
    EvgSetFPOutMap(pEg, 0, C_SIGNAL_MAP_LOW);
  }

  /*
   * RF Input test
   */

#ifdef USE_EXT_RF
  printf("Changing to external /4 reference.\n");
  EvgSetRFInput(pEg, 1, C_EVG_RFDIV_4);
  sleep(3);
  /* Clear violation flag */
  EvrGetViolation(pEr, 1);
  EvrGetViolation(pBEr, 1);
#endif

  /*
  printf("Sleep 5\n");
  sleep(5);
  */

  /*
   * Sequence RAM tests
   */

  printf("Sequence RAM tests\n");

  EvgClearIrqFlags(pEg, -1);
  evg_clr_irq();

  EvgIrqEnable(pEg, (1 << C_EVG_IRQ_MASTER_ENABLE) |
	       (0x0f << C_EVG_IRQFLAG_SEQSTART) |
	       (0x0f << C_EVG_IRQFLAG_SEQSTOP));

  {
    int ram;
    int cnt[7];

    for (ram = 0; ram < 2; ram++)
      {
	EvgClearIrqFlags(pEg, -1);
	evg_clr_irq();

	i = EvgGetIrqFlags(pEg);
	if (i & ~1)
	  {
	    printf("EVG IRQ flags %08x\n", i);
	    err_seq++;
	  }

	/* Reset Sequencer */
	EvgSeqRamControl(pEg, ram, 0, 0, 0, 1, 0);	

	/* Lets put 50 Hz on MXC3, output on UNIVOUT0 -> LEMO Loopback to ACIN */
	EvgSetMXCPrescaler(pEg, 3, 499.654E6/4/50);
	EvgSyncMxc(pEg);

	if (evgff == 0 || evgff == 4)
	  {
	    /* cPCI: output on UNIVOUT0 -> LEMO Loopback to ACIN */
	    EvgSetUnivOutMap(pEg, 0, C_SIGNAL_MAP_PRESC+3);
	  }
	if (evgff == 2)
	  {
	    /* VME: output on TTLOUT0 -> LEMO Loopback to ACIN */
	    EvgSetFPOutMap(pEg, 0, C_SIGNAL_MAP_PRESC+3);
	  }

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

	/*
	EvgSeqRamStatus(pEg, ram);
	*/

	for (i = 0; i < 10; i++)
	  sleep(2);

	/*
	EvgSeqRamStatus(pEg, ram);
	*/

	EvgSeqRamControl(pEg, ram, 0, 0, 0, 0, 0);

	for (i = 0; i < 7; i++)
	  {
	    cnt[i] = EvrGetDiagCounter(pEr, i);
	    if (cnt[i] < 9 || cnt[i] > 11)
	      {
		printf(ERROR_TEXT "SEQ%d OUT%d %d cycles out of 10\n", ram, i, cnt[i]);
		err_seq++;
	      }
	  }

	for (i = 0; i < EVG_MAX_SEQRAMS; i++)
	  {
	    if (i == ram && (evg_irq_cnt[C_EVG_IRQFLAG_SEQSTART+i] < 9 ||
			     evg_irq_cnt[C_EVG_IRQFLAG_SEQSTART+i] > 11))
	      {
		printf(ERROR_TEXT "SEQ%d %d start irqs out of 10\n", i, 
		       evg_irq_cnt[C_EVG_IRQFLAG_SEQSTART+i]);
		err_irq++;
	      }
	    if (i != ram && evg_irq_cnt[C_EVG_IRQFLAG_SEQSTART+i])
	      {
		printf(ERROR_TEXT "SEQ%d %d spurious start irqs\n", i, 
		       evg_irq_cnt[C_EVG_IRQFLAG_SEQSTART+i]);
		err_irq++;
	      }
	    if (i == ram && (evg_irq_cnt[C_EVG_IRQFLAG_SEQSTOP+i] < 9 ||
			     evg_irq_cnt[C_EVG_IRQFLAG_SEQSTOP+i] > 11))
	      {
		printf(ERROR_TEXT "SEQ%d %d stop irqs out of 10\n", i, 
		       evg_irq_cnt[C_EVG_IRQFLAG_SEQSTOP+i]);
		err_irq++;
	      }
	    if (i != ram && evg_irq_cnt[C_EVG_IRQFLAG_SEQSTOP+i])
	      {
		printf(ERROR_TEXT "SEQ%d %d spurious stop irqs\n", i, 
		       evg_irq_cnt[C_EVG_IRQFLAG_SEQSTOP+i]);
		err_irq++;
	      }
	  }
      }

    EvgIrqEnable(pEg, 0);

    EvgSetMXCPrescaler(pEg, 3, 0);
    EvgSetUnivOutMap(pEg, 0, C_SIGNAL_MAP_LOW);
    EvgSetFPOutMap(pEg, 0, C_SIGNAL_MAP_LOW);
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
		printf(ERROR_TEXT "TX %08x, %08x, RX %08x, %08x\n", buf1[size], (int) pEg->Databuf[size], buf2[size], (int) pEr->Databuf[size]);
	      /*		printf("TX %08x, RX %08x\n", pEg->Databuf[size], pEr->Databuf[size]); */
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
   * Test EVG UNIV/TBIN -> Trigger event -> EVR UNIV/TBOUT
   */

  {
    int triggers = evg_event_triggers;
    if (evrff == 0 || evrff == 4)
      {
	if (evr_univ_outputs < triggers)
	  triggers = evr_univ_outputs;
      }
    if (evrff == 1 || evrff == 2)
      {
	if (evr_tb_outputs < triggers)
	  triggers = evr_tb_outputs;
      }
    
    printf("Test EVG HWIN -> Trigger event -> EVR HWOUT\n");

    for (i = 1; i <= triggers; i++)
      {
	if (evgff == 0 || evgff == 4)
	  EvgSetUnivinMap(pEg, i-1, i-1, -1, 0, -1);
	if (evgff == 2)
	  EvgSetTBinMap(pEg, i-1, i-1, 1, 0, -1);
	EvgSetTriggerEvent(pEg, i-1, i, 1);
      }

    for (i = 1; i <= triggers; i++)
      {
	EvrClearDiagCounters(pEr);
	EvrEnableDiagCounters(pEr, 1);

	/* Generate pulses */
	for (j = 0; j < PULSES; j++)
	  {
	    if (evgff == 0 || evgff == 4)
	      {
		EvgSetUnivOutMap(pEg, i-1, C_SIGNAL_MAP_LOW);
		EvgSetUnivOutMap(pEg, i-1, C_SIGNAL_MAP_HIGH);
		EvgSetUnivOutMap(pEg, i-1, C_SIGNAL_MAP_HIGH);
		EvgSetUnivOutMap(pEg, i-1, C_SIGNAL_MAP_HIGH);
		EvgSetUnivOutMap(pEg, i-1, C_SIGNAL_MAP_HIGH);
		EvgSetUnivOutMap(pEg, i-1, C_SIGNAL_MAP_LOW);
		EvgSetUnivOutMap(pEg, i-1, C_SIGNAL_MAP_LOW);
		EvgSetUnivOutMap(pEg, i-1, C_SIGNAL_MAP_LOW);
		EvgSetUnivOutMap(pEg, i-1, C_SIGNAL_MAP_LOW);
	      }
	    if (evgff == 2)
	      {
		EvgSetTBOutMap(pEg, i-1, C_SIGNAL_MAP_LOW);
		EvgSetTBOutMap(pEg, i-1, C_SIGNAL_MAP_HIGH);
		EvgSetTBOutMap(pEg, i-1, C_SIGNAL_MAP_HIGH);
		EvgSetTBOutMap(pEg, i-1, C_SIGNAL_MAP_HIGH);
		EvgSetTBOutMap(pEg, i-1, C_SIGNAL_MAP_HIGH);
		EvgSetTBOutMap(pEg, i-1, C_SIGNAL_MAP_LOW);
		EvgSetTBOutMap(pEg, i-1, C_SIGNAL_MAP_LOW);
		EvgSetTBOutMap(pEg, i-1, C_SIGNAL_MAP_LOW);
		EvgSetTBOutMap(pEg, i-1, C_SIGNAL_MAP_LOW);
	      }
	}

      usleep(1000);

      for (j = 1; j <= triggers; j++)
	{
	  if (j != i)
	    {
	      if (EvrGetDiagCounter(pEr, j-1))
		{
		  printf(ERROR_TEXT "2 Spurious pulses on HWOUT%d: %d\n", j-1,
			 (int) EvrGetDiagCounter(pEr, j-1));
		  err_pulse++;
		}
	    }
	  else if (EvrGetDiagCounter(pEr, j-1) != PULSES)
	    {
	      printf(ERROR_TEXT "2 Wrong number of pulses on HWOUT%d: %d (%d)\n",
		     j-1, (int) EvrGetDiagCounter(pEr, j-1), PULSES);
	      err_pulse++;
	    }
	}
      }

    for (i = 1; i <= triggers; i++)
      {
	EvgSetUnivinMap(pEg, i-1, -1, -1, 0, -1);
	EvgSetTBinMap(pEg, i-1, -1, -1, 0, -1);
	EvgSetTriggerEvent(pEg, i-1, i, 0);
      }
  }

  /*
   * Irq tests
   */

  printf("EVG Irq tests\n");
  {
    printf("Test EVG HWIN -> External interrupt\n");

    EvgIrqEnable(pEg, (1 << C_EVG_IRQ_MASTER_ENABLE) |
	       (1 << C_EVG_IRQFLAG_EXTERNAL));

    EvgClearIrqFlags(pEg, -1);
    evg_clr_irq();

    if (evgff == 0 || evgff == 4)
      EvgSetUnivinMap(pEg, 0, -1, -1, 1, -1); 
    if (evgff == 2)
      EvgSetTBinMap(pEg, 0, -1, -1, 1, -1);

    if (evgff == 0 || evgff == 4)
      {
	EvgSetUnivOutMap(pEg, 0, C_SIGNAL_MAP_LOW);
	EvgSetUnivOutMap(pEg, 0, C_SIGNAL_MAP_HIGH);
	EvgSetUnivOutMap(pEg, 0, C_SIGNAL_MAP_HIGH);
	EvgSetUnivOutMap(pEg, 0, C_SIGNAL_MAP_HIGH);
	EvgSetUnivOutMap(pEg, 0, C_SIGNAL_MAP_HIGH);
	EvgSetUnivOutMap(pEg, 0, C_SIGNAL_MAP_LOW);
      }

    if (evgff == 2)
      {
	EvgSetTBOutMap(pEg, 0, C_SIGNAL_MAP_LOW);
	EvgSetTBOutMap(pEg, 0, C_SIGNAL_MAP_HIGH);
	EvgSetTBOutMap(pEg, 0, C_SIGNAL_MAP_HIGH);
	EvgSetTBOutMap(pEg, 0, C_SIGNAL_MAP_HIGH);
	EvgSetTBOutMap(pEg, 0, C_SIGNAL_MAP_HIGH);
	EvgSetTBOutMap(pEg, 0, C_SIGNAL_MAP_LOW);
      }

    sleep(1);

    if ((i = evg_irq_cnt[C_EVG_IRQFLAG_EXTERNAL]) != 1)
      {
	printf(ERROR_TEXT "External interrupts %d/1\n", i);
	err_irq++;
      }

    EvgIrqEnable(pEg, 0);
    EvgClearIrqFlags(pEg, -1);
    evg_clr_irq();

    EvgSetUnivinMap(pEg, 0, -1, -1, 0, -1); 
    EvgSetTBinMap(pEg, 0, -1, -1, 0, -1);
  }

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
    sleep(1);
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
    /* Fifo on ECP3 is deeper by one event */
    if (evrff == 4)
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

    printf("Heartbeat interrupt test\n");
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

    printf("Event interrupt test\n");
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

    printf("Pulse interrupt test\n");
    evr_clr_irq();
    EvrClearIrqFlags(pEr, -1);
    EvrIrqEnable(pEr, EVR_IRQ_MASTER_ENABLE | EVR_IRQFLAG_PULSE);
    
    EvrSetPulseIrqMap(pEr, C_EVR_SIGNAL_MAP_LOW);
    EvrSetPulseIrqMap(pEr, C_EVR_SIGNAL_MAP_HIGH);
    EvrSetPulseIrqMap(pEr, C_EVR_SIGNAL_MAP_LOW);

    usleep(100);

    if (evr_irq_cnt[C_EVR_IRQFLAG_PULSE] < 1)
      {
	printf(ERROR_TEXT "Pulse IRQ failed %d/1\n", evr_irq_cnt[C_EVR_IRQFLAG_PULSE]);
	err_irq++;
      }

    printf("Databuf interrupt test\n");
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

    printf("Link state interrupt test\n");
    evr_clr_irq();
    EvrClearIrqFlags(pEr, -1);

    EvrIrqEnable(pEr, EVR_IRQ_MASTER_ENABLE | EVR_IRQFLAG_LINKCHG);
    
    /* Generate link down by loading fractional synth with other reference */
    i = EvrGetFracDiv(pEr);
    EvrSetFracDiv(pEr, 0x0106822d);

    /* This sleep will be interrupted by IRQ */
    sleep(1);
    
    if (!evr_irq_cnt[C_EVR_IRQFLAG_LINKCHG])
      {
	printf(ERROR_TEXT "Link change (down) IRQ failed %d/1\n", evr_irq_cnt[C_EVR_IRQFLAG_LINKCHG]);
	err_irq++;
      }

    /* Reload fractional synth */
    EvrSetFracDiv(pEr, i);

    /* This sleep will be interrupted by IRQ */
    sleep(3);
    if (!evr_irq_cnt[C_EVR_IRQFLAG_LINKCHG])
      {
	printf(ERROR_TEXT "Link change (up) IRQ failed %d/1\n", evr_irq_cnt[C_EVR_IRQFLAG_LINKCHG]);
	err_irq++;
      }

    EvrClearIrqFlags(pEr, -1);

    sleep(1);
    sleep(1);
    sleep(1);
    EvrGetViolation(pBEr, 1);

    if (EvrGetViolation(pEr, 1))
    {
      err_vio++;
      printf(ERROR_TEXT "Could not clear violation flag!\n");
      EvrDumpStatus(pEr);
    }
    if (EvrGetViolation(pBEr, 1))
    {
      err_vio++;
      printf(ERROR_TEXT "Could not clear violation flag on BEvr!\n");
      EvrDumpStatus(pBEr);
    }


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
		printf(ERROR_TEXT "Wrong event code in FIFO %ld/%d\n", fe[i].EventCode, (1 << i));
		err_fifo++;
	      }
	    if (fe[i].TimestampHigh != cnt[1])
	      {
		printf(ERROR_TEXT "Seconds error in FIFO %08lx/%08x\n", fe[i].TimestampHigh, cnt[1]);
		err_fifo++;
	      }
	    if (i > 0)
	      if (fe[i].TimestampLow < fe[i-1].TimestampLow)
		{
		  printf(ERROR_TEXT "Timestamp error in FIFO %08lx/%08lx\n",  fe[i-1].TimestampLow,
			 fe[i].TimestampLow);
		  err_fifo++;
		}
	  }
      }

    if (!j)
      {
	printf(ERROR_TEXT "FIFO not empty after reading %i events.\n", i);
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
   * Event Log test
   */
  printf("Event Log test\n");

  {
    EvrClearLog(pEr);
    EvgSWEventEnable(pEg, 1);
    EvrEnableLog(pEr, 1);
    if (EvrGetLogState(pEr))
      {
	printf(ERROR_TEXT "Could not enable Event Log.\n");
	err_log++;
      }

    if ((i = EvrGetLogStart(pEr)) != 0)
      {
	printf(ERROR_TEXT "Log not reset. Position 0x%04x\n", i);
	err_log++;
      }

    if ((i = EvrGetLogEntries(pEr)) != 0)
      {
	printf(ERROR_TEXT "Log not reset. Number of entries 0x%04x\n", i);
	err_log++;
      }

    for (i = 0; i < 0x40; i++)
      {
	EvrSetLogEvent(pEr, 0, i + 1, 1);
	EvgSendSWEvent(pEg, i + 1);
      }

    usleep(100);

    if ((j = EvrGetLogStart(pEr)) != 0)
      {
	printf(ERROR_TEXT "Log start moved. Position 0x%04x\n", j);
	err_log++;
      }

    if ((j = EvrGetLogEntries(pEr)) != i)
      {
	printf(ERROR_TEXT "Number of entries 0x%04x != 0x%04x\n", j, i);
	err_log++;
      }

    /*
    EvrDumpLog(pEr);
    */

    for (i = 0; i < 0x200; i++)
      {
	EvrSetLogEvent(pEr, 0, (i & 0x3f) + 1, 1);
	EvgSendSWEvent(pEg, (i & 0x3f) + 1);
      }
    usleep(100);

    if ((j = EvrGetLogStart(pEr)) != 0x40)
      {
	printf(ERROR_TEXT "Log start not updated. Position 0x%04x\n", j);
	err_log++;
      }

    if ((j = EvrGetLogEntries(pEr)) != 0x200)
      {
	printf(ERROR_TEXT "Number of entries 0x%04x != 0x%04x\n", j, i);
	err_log++;
      }
    
    /*
    EvrDumpLog(pEr);
    */

    EvrClearLog(pEr);

    if (!EvrEnableLogStopEvent(pEr, 1))
      {
	printf(ERROR_TEXT "Could not enable Log stop event.\n");
	err_log++;
      }

    for (i = 0x70; i < 0x7f; i++)
      {
	EvrSetLogEvent(pEr, 0, i, 1);
	EvgSendSWEvent(pEg, i);
      }
    usleep(100);

    if (!EvrGetLogState(pEr))
      {
	printf(ERROR_TEXT "Event Log not stopped.\n");
	err_log++;
      }

    EvrDumpLog(pEr);

    EvrEnableLog(pEr, 1);
    if (EvrGetLogState(pEr))
      {
	printf(ERROR_TEXT "Event Log not enabled.\n");
	err_log++;
      }

    EvrEnableLog(pEr, 0);
    if (!EvrGetLogState(pEr))
      {
	printf(ERROR_TEXT "Event Log not stopped.\n");
	err_log++;
      }

    if (EvrEnableLogStopEvent(pEr, 0))
      {
	printf(ERROR_TEXT "Could not disable Log stop event.\n");
	err_log++;
      }

    EvrEnableLog(pEr, 1);
    if (EvrGetLogState(pEr))
      {
	printf(ERROR_TEXT "Event Log not enabled.\n");
	err_log++;
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
    EvrSetExtEvent(pEr, 0, 1, 1, 0);
    EvrSetExtEvent(pEr, 1, 2, 1, 0);
    EvrSetExtEdgeSensitivity(pEr, 0, 0);
    EvrSetExtEdgeSensitivity(pEr, 1, 0);
    EvrSetExtLevelSensitivity(pEr, 0, 0);
    EvrSetExtLevelSensitivity(pEr, 1, 0);

    for (i = 0; i < 8; i++)
      {
	if (evgff == 0 || evgff == 4)
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
	if (evrff == 2)
	  {
	    EvrSetFPOutMap(pEr, 0, C_SIGNAL_MAP_LOW);
	    EvrSetFPOutMap(pEr, 0, C_SIGNAL_MAP_HIGH);
	    EvrSetFPOutMap(pEr, 0, C_SIGNAL_MAP_LOW);
	    usleep(10);
	    EvrSetFPOutMap(pEr, 1, C_SIGNAL_MAP_LOW);
	    EvrSetFPOutMap(pEr, 1, C_SIGNAL_MAP_HIGH);
	    EvrSetFPOutMap(pEr, 1, C_SIGNAL_MAP_LOW);
	    usleep(10);
	    EvrSetFPOutMap(pEr, 0, C_SIGNAL_MAP_LOW);
	    EvrSetFPOutMap(pEr, 0, C_SIGNAL_MAP_HIGH);
	    EvrSetFPOutMap(pEr, 0, C_SIGNAL_MAP_LOW);
	    usleep(10);
	  }
      }
    if ((i = EvrGetDiagCounter(pEr, 0)) != 16)
      {
	printf(ERROR_TEXT "Wrong number of ext0 events %d/16\n", i);
	err_extev++;
      }

    if (evrff == 0 || evrff == 4 || evrff == 2)
      {
	if ((i = EvrGetDiagCounter(pEr, 1)) != 8)
	  {
	    printf(ERROR_TEXT "Wrong number of ext1 events %d/8\n", i);
	    err_extev++;
	  }
      }

    /*    
      Testing rising edge
    */
    EvrSetExtEdgeSensitivity(pEr, 0, 0);
    EvrSetExtEdgeSensitivity(pEr, 1, 0);
    if (evgff == 0 || evgff == 4)
      {
	EvgSetUnivOutMap(pEg, 2, C_SIGNAL_MAP_LOW);
	EvgSetUnivOutMap(pEg, 3, C_SIGNAL_MAP_LOW);
      }
    if (evrff == 2)
      {
	EvrSetFPOutMap(pEr, 0, C_SIGNAL_MAP_LOW);
	EvrSetFPOutMap(pEr, 1, C_SIGNAL_MAP_LOW);
      }
    usleep(100);
    EvrClearDiagCounters(pEr);
    if (evgff == 0 || evgff == 4)
      {
	EvgSetUnivOutMap(pEg, 2, C_SIGNAL_MAP_HIGH);
	EvgSetUnivOutMap(pEg, 3, C_SIGNAL_MAP_HIGH);
      }
    if (evrff == 2)
      {
	EvrSetFPOutMap(pEr, 0, C_SIGNAL_MAP_HIGH);
	EvrSetFPOutMap(pEr, 1, C_SIGNAL_MAP_HIGH);
      }
    usleep(100);
    if ((i = EvrGetDiagCounter(pEr, 0)) != 1)
      {
	printf(ERROR_TEXT "Rising edge test failed for IN0\n");
	err_extev++;
      }
    if (evrff == 0 || evrff == 4)
      if ((i = EvrGetDiagCounter(pEr, 1)) != 1)
	{
	  printf(ERROR_TEXT "Rising edge test failed for IN1\n");
	  err_extev++;
	}
    /*
      Testing falling edge
    */
    EvrSetExtEdgeSensitivity(pEr, 0, 1);
    EvrSetExtEdgeSensitivity(pEr, 1, 1);
    EvrClearDiagCounters(pEr);
    if (evgff == 0 || evgff == 4)
      {
	EvgSetUnivOutMap(pEg, 2, C_SIGNAL_MAP_LOW);
	EvgSetUnivOutMap(pEg, 3, C_SIGNAL_MAP_LOW);
      }
    if (evrff == 2)
      {
	EvrSetFPOutMap(pEr, 0, C_SIGNAL_MAP_LOW);
	EvrSetFPOutMap(pEr, 1, C_SIGNAL_MAP_LOW);
      }
    usleep(100);
    if ((i = EvrGetDiagCounter(pEr, 0)) != 1)
      {
	printf(ERROR_TEXT "Falling edge test failed for IN0\n");
	err_extev++;
      }
    if (evrff == 0 || evrff == 4)
      if ((i = EvrGetDiagCounter(pEr, 1)) != 1)
	{
	  printf(ERROR_TEXT "Falling edge test failed for IN1\n");
	  err_extev++;
	}
    /*
      Testing high level
    */
    EvrClearDiagCounters(pEr);
    EvrSetExtEvent(pEr, 0, 1, 0, 1);
    EvrSetExtEvent(pEr, 1, 2, 0, 1);
    EvrSetExtLevelSensitivity(pEr, 0, 0);
    EvrSetExtLevelSensitivity(pEr, 1, 0);
    usleep(100);
    if ((i = EvrGetDiagCounter(pEr, 0)))
      {
	printf(ERROR_TEXT "High sens. on IN0 produced %d events when input low\n", i);
	err_extev++;
      }
    if (evrff == 0 || evrff == 4)
      if ((i = EvrGetDiagCounter(pEr, 1)))
	{
	  printf(ERROR_TEXT "High sens. on IN1 produced %d events when input low\n", i);
	  err_extev++;
	}
    if (evgff == 0 || evgff == 4)
      {
	EvgSetUnivOutMap(pEg, 2, C_SIGNAL_MAP_HIGH);
	EvgSetUnivOutMap(pEg, 3, C_SIGNAL_MAP_HIGH);
      }
    if (evrff == 2)
      {
	EvrSetFPOutMap(pEr, 0, C_SIGNAL_MAP_HIGH);
	EvrSetFPOutMap(pEr, 1, C_SIGNAL_MAP_HIGH);
      }
    usleep(100);
    if (!(i = EvrGetDiagCounter(pEr, 0)))
      {
	printf(ERROR_TEXT "High level on IN0 did not produce events\n");
	err_extev++;
      }

    if (evrff == 0 || evrff == 4)
      if (!(i = EvrGetDiagCounter(pEr, 1)))
	{
	  printf(ERROR_TEXT "High level on IN1 did not produce events\n");
	  err_extev++;
	}

    /*
      Testing low level
    */
    EvrSetExtLevelSensitivity(pEr, 0, 1);
    EvrSetExtLevelSensitivity(pEr, 1, 1);
    usleep(100);
    EvrClearDiagCounters(pEr);
    usleep(100);
    if ((i = EvrGetDiagCounter(pEr, 0)))
      {
	printf(ERROR_TEXT "Low sens. on IN0 produced %d events when input low\n", i);
	err_extev++;
      }
    if (evrff == 0 || evrff == 4)
      if ((i = EvrGetDiagCounter(pEr, 1)))
	{
	  printf(ERROR_TEXT "High sens. on IN1 produced %d events when input low\n", i);
	  err_extev++;
	}

    if (evgff == 0 || evgff == 4)
      {
	EvgSetUnivOutMap(pEg, 2, C_SIGNAL_MAP_LOW);
	EvgSetUnivOutMap(pEg, 3, C_SIGNAL_MAP_LOW);
      }
    if (evrff == 2)
      {
	EvrSetFPOutMap(pEr, 0, C_SIGNAL_MAP_LOW);
	EvrSetFPOutMap(pEr, 1, C_SIGNAL_MAP_LOW);
      }
    usleep(100);
    if (!(i = EvrGetDiagCounter(pEr, 0)))
      {
	printf(ERROR_TEXT "Low level on IN0 did not produce events\n");
	err_extev++;
      }
    if (evrff == 0 || evrff == 4)
      if (!(i = EvrGetDiagCounter(pEr, 1)))
	{
	  printf(ERROR_TEXT "Low level on IN1 did not produce events\n");
	  err_extev++;
	}

    EvrSetExtEvent(pEr, 0, 0, 0, 0);
    EvrSetExtEvent(pEr, 1, 0, 0, 0);
    EvrEnableDiagCounters(pBEr, 0);
  }

  /*
   * Backward event test
   */

  printf("Backward event test.\n");

  for (i = 0; i < 2; i++)
    {
      EvrClearDiagCounters(pBEr);
      EvrEnableDiagCounters(pBEr, 1);

      EvrSetBackEvent(pEr, i, i+1, 1, 0);

      /*
      for (j = 0; j < 8; j++)
	printf("%d: %5ld ", j, EvrGetDiagCounter(pBEr, j));
      printf("\n");
      */

      for (j = 0; j < PULSES; j++)
	{
	  if (evgff == 0 || evgff == 4)
	    {
	      EvgSetUnivOutMap(pEg, i+2, C_SIGNAL_MAP_LOW);
	      EvgSetUnivOutMap(pEg, i+2, C_SIGNAL_MAP_HIGH);
	      EvgSetUnivOutMap(pEg, i+2, C_SIGNAL_MAP_LOW);
	      EvgSetUnivOutMap(pEg, i+2, C_SIGNAL_MAP_LOW);
	      EvgSetUnivOutMap(pEg, i+2, C_SIGNAL_MAP_LOW);
	      EvgSetUnivOutMap(pEg, i+2, C_SIGNAL_MAP_LOW);
	      EvgSetUnivOutMap(pEg, i+2, C_SIGNAL_MAP_LOW);
	    }
	  if (evrff == 2)
	    {
	      EvrSetFPOutMap(pEr, i, C_SIGNAL_MAP_LOW);
	      EvrSetFPOutMap(pEr, i, C_SIGNAL_MAP_HIGH);
	      EvrSetFPOutMap(pEr, i, C_SIGNAL_MAP_LOW);
	      EvrSetFPOutMap(pEr, i, C_SIGNAL_MAP_LOW);
	      EvrSetFPOutMap(pEr, i, C_SIGNAL_MAP_LOW);
	      EvrSetFPOutMap(pEr, i, C_SIGNAL_MAP_LOW);
	      EvrSetFPOutMap(pEr, i, C_SIGNAL_MAP_LOW);
	    }
	}

      /* Only one input on PMC-EVR-230 */
      k = 2;
      if (evrff == 1)
	k = 1;

      for (j = 0; j < 8; j++)
	printf("%d: %5ld ", j, EvrGetDiagCounter(pBEr, j));
      printf("\n");

      for (j = 0; j < k; j++)
	{
	  if (j != i)
	    {
	      if (EvrGetDiagCounter(pBEr, j))
		{
		  printf(ERROR_TEXT "3 Spurious pulses on UNIVOUT%d: %ld\n", j,
			 EvrGetDiagCounter(pBEr, j));
		  err_bev++;
		}
	    }
	  else if (EvrGetDiagCounter(pBEr, j) != PULSES)
	    {
	      printf(ERROR_TEXT "3 Wrong number of pulses on UNIVOUT%d: %ld (%d)\n",
		     j, EvrGetDiagCounter(pBEr, j), PULSES);
	      err_bev++;
	    }
	}

      EvrSetBackEvent(pEr, i, 0, 0, 0);
    }

  EvrSetBackEvent(pEr, 0, 1, 1, 0);
  EvrSetBackEvent(pEr, 1, 2, 1, 0);

  /*
    Testing rising edge
  */
  EvrSetExtEdgeSensitivity(pEr, 0, 0);
  EvrSetExtEdgeSensitivity(pEr, 1, 0);
  if (evgff == 0 || evgff == 4)
    {
      EvgSetUnivOutMap(pEg, 2, C_SIGNAL_MAP_LOW);
      EvgSetUnivOutMap(pEg, 3, C_SIGNAL_MAP_LOW);
    }
  if (evrff == 2)
    {
      EvrSetFPOutMap(pEr, 0, C_SIGNAL_MAP_LOW);
      EvrSetFPOutMap(pEr, 1, C_SIGNAL_MAP_LOW);
    }
  usleep(100);
  EvrClearDiagCounters(pBEr);
  if (evgff == 0 || evgff == 4)
    {
      EvgSetUnivOutMap(pEg, 2, C_SIGNAL_MAP_HIGH);
      EvgSetUnivOutMap(pEg, 3, C_SIGNAL_MAP_HIGH);
    }
  if (evrff == 2)
    {
      EvrSetFPOutMap(pEr, 0, C_SIGNAL_MAP_HIGH);
      EvrSetFPOutMap(pEr, 1, C_SIGNAL_MAP_HIGH);
    }
  usleep(100);
  if ((i = EvrGetDiagCounter(pBEr, 0)) != 1)
    {
      printf(ERROR_TEXT "Rising edge test failed for IN0\n");
      err_bev++;
    }
  if (evrff == 0 || evrff == 4)
    if ((i = EvrGetDiagCounter(pBEr, 1)) != 1)
      {
	printf(ERROR_TEXT "Rising edge test failed for IN1\n");
	err_bev++;
      }
  /*
    Testing falling edge
  */
  EvrSetExtEdgeSensitivity(pEr, 0, 1);
  EvrSetExtEdgeSensitivity(pEr, 1, 1);
  EvrClearDiagCounters(pBEr);
  if (evgff == 0 || evgff == 4)
    {
      EvgSetUnivOutMap(pEg, 2, C_SIGNAL_MAP_LOW);
      EvgSetUnivOutMap(pEg, 3, C_SIGNAL_MAP_LOW);
    }
  if (evrff == 2)
    {
      EvrSetFPOutMap(pEr, 0, C_SIGNAL_MAP_LOW);
      EvrSetFPOutMap(pEr, 1, C_SIGNAL_MAP_LOW);
    }
  usleep(100);
  if ((i = EvrGetDiagCounter(pBEr, 0)) != 1)
    {
      printf(ERROR_TEXT "Falling edge test failed for IN0\n");
      err_bev++;
    }
  if (evrff == 0 || evrff == 4 || evrff == 2)
    if ((i = EvrGetDiagCounter(pBEr, 1)) != 1)
      {
	printf(ERROR_TEXT "Falling edge test failed for IN1\n");
	err_bev++;
      }
  /*
    Testing high level
  */
  EvrSetBackEvent(pEr, 0, 1, 0, 1);
  EvrSetBackEvent(pEr, 1, 2, 0, 1);
  EvrSetExtLevelSensitivity(pEr, 0, 0);
  EvrSetExtLevelSensitivity(pEr, 1, 0);
  usleep(100);
  EvrClearDiagCounters(pBEr);
  usleep(100);
  if ((i = EvrGetDiagCounter(pBEr, 0)))
    {
      printf(ERROR_TEXT "High sens. on IN0 produced %d events when input low\n", i);
      err_bev++;
    }
  if (evrff == 0 || evrff == 4 || evrff == 2)
    if ((i = EvrGetDiagCounter(pBEr, 1)))
      {
	printf(ERROR_TEXT "High sens. on IN1 produced %d events when input low\n", i);
	err_bev++;
      }
  if (evgff == 0 || evgff == 4)
    {
      EvgSetUnivOutMap(pEg, 2, C_SIGNAL_MAP_HIGH);
      EvgSetUnivOutMap(pEg, 3, C_SIGNAL_MAP_HIGH);
    }
  if (evrff == 2)
    {
      EvrSetFPOutMap(pEr, 0, C_SIGNAL_MAP_HIGH);
      EvrSetFPOutMap(pEr, 1, C_SIGNAL_MAP_HIGH);
    }
  usleep(100);
  if (!(i = EvrGetDiagCounter(pBEr, 0)))
    {
      printf(ERROR_TEXT "High level on IN0 did not produce events\n");
      err_bev++;
    }
  if (evrff == 0 || evrff == 4 || evrff == 2)
    if (!(i = EvrGetDiagCounter(pBEr, 1)))
      {
	printf(ERROR_TEXT "High level on IN1 did not produce events\n");
	err_bev++;
      }
  /*
    Testing low level
  */
  EvrSetExtLevelSensitivity(pEr, 0, 1);
  EvrSetExtLevelSensitivity(pEr, 1, 1);
  usleep(100);
  EvrClearDiagCounters(pBEr);
  usleep(100);
  if ((i = EvrGetDiagCounter(pBEr, 0)))
    {
      printf(ERROR_TEXT "Low sens. on IN0 produced %d events when input low\n", i);
      err_bev++;
    }
  if (evrff == 0 || evrff == 4 || evrff == 2)
    if ((i = EvrGetDiagCounter(pBEr, 1)))
      {
	printf(ERROR_TEXT "High sens. on IN1 produced %d events when input low\n", i);
	err_bev++;
      }
  if (evgff == 0 || evgff == 4)
    {
      EvgSetUnivOutMap(pEg, 2, C_SIGNAL_MAP_LOW);
      EvgSetUnivOutMap(pEg, 3, C_SIGNAL_MAP_LOW);
    }
  if (evrff == 2)
    {
      EvrSetFPOutMap(pEr, 0, C_SIGNAL_MAP_LOW);
      EvrSetFPOutMap(pEr, 1, C_SIGNAL_MAP_LOW);
    }
  usleep(100);
  if (!(i = EvrGetDiagCounter(pBEr, 0)))
    {
      printf(ERROR_TEXT "Low level on IN0 did not produce events\n");
      err_bev++;
    }
  if (evrff == 0 || evrff == 4 || evrff == 2)
    if (!(i = EvrGetDiagCounter(pBEr, 1)))
      {
	printf(ERROR_TEXT "Low level on IN1 did not produce events\n");
	err_bev++;
      }

  EvrSetBackEvent(pEr, 0, 0, 0, 0);
  EvrSetBackEvent(pEr, 1, 0, 0, 0);

  /*
   * Backward DBUS test
   */

  printf("Backward DBUS test\n");

  /* Use data buffer mode!!! */

  if (!(EvrSetDBufMode(pBEr, 1) & (1 << C_EVR_DATABUF_MODE)))
    {
      printf(ERROR_TEXT "Could not enable BEVR databuffer mode!\n");
      err_gen++;
    }

  for (i = 0; i < 8; i++)
    {
      if (evrff == 0 || evrff == 4)
	EvrSetUnivOutMap(pBEr, i, i+C_EVR_SIGNAL_MAP_DBUS);
      else
	EvrSetTBOutMap(pBEr, i, i+C_EVR_SIGNAL_MAP_DBUS);
    }

  /* Only one input on PMC-EVR-230 */
  k = 2;
  if (evrff == 1)
    k = 1;
  
  for (i = 0; i < k; i++)
    {
      EvrClearDiagCounters(pBEr);
      EvrEnableDiagCounters(pBEr, 1);

      EvrSetBackDBus(pEr, i, 4 << i);

      for (j = 0; j < PULSES; j++)
	{
	  if (evgff == 0 || evgff == 4)
	    {
	      EvgSetUnivOutMap(pEg, i+2, C_SIGNAL_MAP_LOW);
	      EvgSetUnivOutMap(pEg, i+2, C_SIGNAL_MAP_HIGH);
	      EvgSetUnivOutMap(pEg, i+2, C_SIGNAL_MAP_LOW);
	      EvgSetUnivOutMap(pEg, i+2, C_SIGNAL_MAP_LOW);
	      EvgSetUnivOutMap(pEg, i+2, C_SIGNAL_MAP_LOW);
	      EvgSetUnivOutMap(pEg, i+2, C_SIGNAL_MAP_LOW);
	      EvgSetUnivOutMap(pEg, i+2, C_SIGNAL_MAP_LOW);
	    }
	  if (evrff == 2)
	    {
	      EvrSetFPOutMap(pEr, i, C_SIGNAL_MAP_LOW);
	      EvrSetFPOutMap(pEr, i, C_SIGNAL_MAP_HIGH);
	      EvrSetFPOutMap(pEr, i, C_SIGNAL_MAP_LOW);
	      EvrSetFPOutMap(pEr, i, C_SIGNAL_MAP_LOW);
	      EvrSetFPOutMap(pEr, i, C_SIGNAL_MAP_LOW);
	      EvrSetFPOutMap(pEr, i, C_SIGNAL_MAP_LOW);
	      EvrSetFPOutMap(pEr, i, C_SIGNAL_MAP_LOW);
	    }
	}

      for (j = 0; j < 2; j++)
	{
	  if (j != i)
	    {
	      if (EvrGetDiagCounter(pBEr, j+2))
		{
		  printf(ERROR_TEXT "4 Spurious pulses on UNIVOUT%d: %ld\n", j+2,
			 EvrGetDiagCounter(pBEr, j+2));
		  err_bdbus++;
		}
	    }
	  else if (EvrGetDiagCounter(pBEr, j+2) != PULSES)
	    {
	      printf(ERROR_TEXT "4 Wrong number of pulses on UNIVOUT%d: %ld (%d)\n",
		     j+2, EvrGetDiagCounter(pBEr, j+2), PULSES);
	      err_bdbus++;
	    }
	}

      EvrSetBackDBus(pEr, i, 0);
    }
  
  for (i = 0; i < 8; i++)
    {
      if (evrff == 0 || evrff == 4)
	EvrSetUnivOutMap(pBEr, i, i);
      else
	EvrSetTBOutMap(pBEr, i, i);
    }

  /* Disable data buffer mode!!! */

  if ((EvrSetDBufMode(pBEr, 0) & (1 << C_EVR_DATABUF_MODE)))
    {
      printf(ERROR_TEXT "Could not disable BEVR databuffer mode!\n");
      err_gen++;
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

    /*
    printf("DataBufControl %08x\n", pBEr->DataBufControl);
    printf("TxDataBufControl %08x\n", pEr->TxDataBufControl);
    */

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
		printf("TX %08x, %08x, RX %08x, %08x\n", buf1[size], (int) pEr->TxDatabuf[size], buf2[size], (int) pBEr->Databuf[size]);
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

  if ((i = EvrGetDiagCounter(pBEr, 1)))
    {
      printf(ERROR_TEXT "Spurios events %d (fowarding disabled)\n", i);
      err_fwd++;
    }
  
  EvrEnableEventForwarding(pEr, 1);

  /*
   * Test DLYs
   */
  
#ifndef __linux__
  EvgIrqUnassignHandler(0x61, evg_irq_handler);
  EvrIrqUnassignHandler(0x60, evr_irq_handler);
#endif

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

  /* Output DBUS0 on TTLOUT2 */
  EvrSetFPOutMap(pEr, 2, 32);
  EvrSetFPOutMap(pEr, 3, 32);
  EvrSetFPOutMap(pEr, 4, 32);
  EvrSetFPOutMap(pEr, 5, 32);
  EvrSetFPOutMap(pEr, 6, 32);
  EvrSetFPOutMap(pEr, 7, 32);
  pEr->CML[0].Control = be32_to_cpu(1 << C_EVR_CMLCTRL_ENABLE);
  pEr->CML[1].Control = be32_to_cpu(1 << C_EVR_CMLCTRL_ENABLE);
  pEr->CML[2].Control = be32_to_cpu(1 << C_EVR_CMLCTRL_ENABLE);
  EvrSetUnivOutMap(pEr, 0, 32);
  EvrSetUnivOutMap(pEr, 1, 32);
  EvrSetUnivOutMap(pEr, 2, 32);
  EvrSetUnivOutMap(pEr, 3, 32);
  EvrUnivDlyEnable(pEr, 0, 1);
  EvrUnivDlyEnable(pEr, 1, 1);
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

int MBLTTest(volatile struct MrfEgRegs *pEg)
{
  pEg->Databuf[0] = 0x11111111;
  pEg->Databuf[1] = 0x22222222;
  pEg->Databuf[2] = 0x33333333;
  pEg->Databuf[3] = 0x44444444;
  pEg->Databuf[4] = 0x55555555;
  pEg->Databuf[5] = 0x66666666;
  pEg->Databuf[6] = 0x77777777;
  pEg->Databuf[7] = 0x88888888;

  return 0;
}

void WriteTest(volatile struct MrfErRegs *pEr)
{
  int i;

  while(1)
    {
      pEr->Control = 0x80000600;
      i = pEr->Control;
    }
}

void TSRead(volatile struct MrfErRegs *pEr)
{
  int ts, sec;

  while(1)
    {
      sec = pEr->SecondsCounter;
      ts = pEr->TimestampEventCounter;
      if (sec != 0 || ts != 0)
	printf("%08x %08x\n", sec, ts);
    }
}

#ifdef __linux__
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

  pIrqEg = pEg;
  fdIrqEg = fdEg;
  EvgIrqEnable(pIrqEg, 0);
  /*
    We cannot assign two handler for one signal,
    evr and evg handlers are cascaded.
  */

  fdBEr = EvrOpen(&pBEr, argv[3]);
  if (fdBEr == -1)
    {
      EvgClose(fdEg);
      EvgClose(fdEr);
      return errno;
    }

  pIrqEr = pEr;
  fdIrqEr = fdEr;
  EvrIrqEnable(pIrqEr, 0);
  EvgIrqAssignHandler(pIrqEg, fdIrqEg, &evr_irq_handler);
  EvrIrqAssignHandler(pIrqEr, fdIrqEr, &evr_irq_handler);

  do
    {
      printf("cPCI-EVG-230 and cPCI-EVR-230 test\n\n");
      printf("1. run testloop\n");
      printf("2. run 10 testloops\n");
      printf("5. exit\n");

      do {
	i = getchar();
      }
      while (i == '\n');

      switch(i)
	{
	case '1':
	  clear_errors();
	  MRMTest(pEg, pEr, pBEr);
	  dump_errors();
	  break;
	case '2':
	  clear_errors();
	  for (j = 0; j < 10; j++)
	    MRMTest(pEg, pEr, pBEr);
	  dump_errors();
	  break;
	case '3':
	  clear_errors();
	  for (j = 0; j < 1000; j++)
	    {
	      MRMTest(pEg, pEr, pBEr);
	      if (dump_errors())
		break;
	    }
	  break;
	}
    }
  while(i != '5');

  EvrClose(fdEr);
  EvgClose(fdEg);
}
#endif

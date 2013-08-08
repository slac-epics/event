/*
  egapi.c -- Functions for Micro-Research Event Generator
             Application Programming Interface

  Author: Jukka Pietarinen (MRF)
  Date:   05.12.2006

*/

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <stdio.h>
#include <endian.h>
#include <byteswap.h>
#include <errno.h>
#include <string.h>
#include <unistd.h>

#include "egcpci.h"
#include "egapi.h"

/*
#define DEBUG 1
*/
#define DEBUG_PRINTF printf

int EvgOpen(struct MrfEgRegs **pEg, char *device_name)
{
  int fd;

  /* Open Event Generator device for read/write */
  fd = open(device_name, O_RDWR);
#ifdef DEBUG
  DEBUG_PRINTF("EvgOpen: open(\"%s\", O_RDWR) returned %d\n", device_name, fd);
#endif
  if (fd != -1)
    {
      /* Memory map Event Generator registers */
      *pEg = (struct MrfEgRegs *) mmap(0, EVG_MEM_WINDOW, PROT_READ | PROT_WRITE,
					MAP_SHARED, fd, 0);
#ifdef DEBUG
  DEBUG_PRINTF("EvgOpen: mmap returned %08x, errno %d\n", (int) *pEg,
	       errno);
#endif
      if (*pEg == MAP_FAILED)
	{
	  close(fd);
	  return -1;
	}
    }

  return fd;
}

int EvgClose(int fd)
{
  int result;

  result = munmap(0, EVG_MEM_WINDOW);
  return close(fd);
}

int EvgEnable(volatile struct MrfEgRegs *pEg, int state)
{
  if (state)
    pEg->Control |= be32_to_cpu(1 << C_EVG_CTRL_MASTER_ENABLE);
  else
    pEg->Control &= be32_to_cpu(~(1 << C_EVG_CTRL_MASTER_ENABLE));
  
  return EvgGetEnable(pEg);
}

int EvgGetEnable(volatile struct MrfEgRegs *pEg)
{
  return be32_to_cpu(pEg->Control & be32_to_cpu(1 << C_EVG_CTRL_MASTER_ENABLE));
}

int EvgRxEnable(volatile struct MrfEgRegs *pEg, int state)
{
  if (!state)
    pEg->Control |= be32_to_cpu(1 << C_EVG_CTRL_RX_DISABLE);
  else
    pEg->Control &= be32_to_cpu(~(1 << C_EVG_CTRL_RX_DISABLE));
  
  return EvgGetEnable(pEg);
}

int EvgRxGetEnable(volatile struct MrfEgRegs *pEg)
{
  return be32_to_cpu(~(pEg->Control) & be32_to_cpu(1 << C_EVG_CTRL_RX_DISABLE));
}

int EvgGetViolation(volatile struct MrfEgRegs *pEg, int clear)
{
  int result;

  result = be32_to_cpu(pEg->IrqFlag & be32_to_cpu(1 << C_EVG_IRQFLAG_VIOLATION));
  if (clear && result)
    pEg->IrqFlag = be32_to_cpu(result);

  return result;
}

int EvgSWEventEnable(volatile struct MrfEgRegs *pEg, int state)
{
  unsigned int mask = ~((1 << (C_EVG_SWEVENT_CODE_HIGH + 1)) -
    (1 << C_EVG_SWEVENT_CODE_LOW));
  int swe;

  swe = be32_to_cpu(pEg->SWEvent);
  if (state)
    pEg->SWEvent = be32_to_cpu(1 << C_EVG_SWEVENT_ENABLE | (swe & mask));
  else
    pEg->SWEvent = be32_to_cpu(~(1 << C_EVG_SWEVENT_ENABLE) & swe & mask);
  return EvgGetSWEventEnable(pEg);
}

int EvgGetSWEventEnable(volatile struct MrfEgRegs *pEg)
{
  return be32_to_cpu(pEg->SWEvent & be32_to_cpu(1 << C_EVG_SWEVENT_ENABLE));
}

int EvgSendSWEvent(volatile struct MrfEgRegs *pEg, int code)
{
  unsigned int mask = ~((1 << (C_EVG_SWEVENT_CODE_HIGH + 1)) -
    (1 << C_EVG_SWEVENT_CODE_LOW));
  int swcode;

  swcode = be32_to_cpu(pEg->SWEvent);
  swcode &= mask;
  swcode |= (code & EVG_MAX_EVENT_CODE);

  pEg->SWEvent = be32_to_cpu(swcode);

  return be32_to_cpu(pEg->SWEvent);
}

int EvgEvanEnable(volatile struct MrfEgRegs *pEg, int state)
{
  if (state)
    pEg->EvanControl |= be32_to_cpu(1 << C_EVG_EVANCTRL_ENABLE);
  else
    pEg->EvanControl &= be32_to_cpu(~(1 << C_EVG_EVANCTRL_ENABLE));
  
  return EvgEvanGetEnable(pEg);
}

int EvgEvanGetEnable(volatile struct MrfEgRegs *pEg)
{
  return be32_to_cpu(pEg->EvanControl & be32_to_cpu(1 << C_EVG_EVANCTRL_ENABLE)); 
}

void EvgEvanReset(volatile struct MrfEgRegs *pEg)
{
  struct EvanStruct evan;

  pEg->EvanControl |= be32_to_cpu(1 << C_EVG_EVANCTRL_RESET);
  /* Dummy read to clear FIFO */
  EvgEvanGetEvent(pEg, &evan);
}

void EvgEvanResetCount(volatile struct MrfEgRegs *pEg)
{
  pEg->EvanControl |= be32_to_cpu(1 << C_EVG_EVANCTRL_COUNTRES);
}

int EvgEvanGetEvent(volatile struct MrfEgRegs *pEg, struct EvanStruct *evan)
{
  if (pEg->EvanControl & be32_to_cpu(1 << C_EVG_EVANCTRL_NOTEMPTY))
    {
      /* Reading the event code & dbus data, pops the next item first from the event
	 analyzer fifo */
      evan->EventCode = be32_to_cpu(pEg->EvanCode);
      evan->TimestampHigh = be32_to_cpu(pEg->EvanTimeH);
      evan->TimestampLow = be32_to_cpu(pEg->EvanTimeL);
      return 0;
    }
  return -1;
}

int EvgSetMXCPrescaler(volatile struct MrfEgRegs *pEg, int mxc, unsigned int presc)
{
  if (mxc < 0 || mxc >= EVG_MAX_MXCS)
    return -1;

  pEg->MXC[mxc].Prescaler = be32_to_cpu(presc);

  return 0;
}

int EvgSetMxcTrigMap(volatile struct MrfEgRegs *pEg, int mxc, int map)
{
  if (mxc < 0 || mxc >= EVG_MAX_MXCS)
    return -1;

  if (map < -1 || map >= EVG_MAX_TRIGGERS)
    return -1;

  if (map >= 0)
    pEg->MXC[mxc].Control = be32_to_cpu( 1 << (map + C_EVG_MXCMAP_TRIG_BASE));
  else
    pEg->MXC[mxc].Control = be32_to_cpu(0);

  return 0;
}

void EvgSyncMxc(volatile struct MrfEgRegs *pEg)
{
  pEg->Control |= be32_to_cpu(1 << C_EVG_CTRL_MXC_RESET);
}

void EvgMXCDump(volatile struct MrfEgRegs *pEg)
{
  int mxc;

  for (mxc = 0; mxc < EVG_MXCS; mxc++)
    {
      DEBUG_PRINTF("MXC%d Prescaler %08x (%d) Trig %08x State %d\n",
		   mxc,
		   be32_to_cpu(pEg->MXC[mxc].Prescaler),
		   be32_to_cpu(pEg->MXC[mxc].Prescaler),
		   be32_to_cpu(pEg->MXC[mxc].Control) & 
		   ((1 << EVG_MAX_TRIGGERS) - 1),
		   (be32_to_cpu(pEg->MXC[mxc].Control) & (1 << C_EVG_MXC_READ))
		   ? 1 : 0);
    }
}

int EvgSetDBusMap(volatile struct MrfEgRegs *pEg, int dbus, int map)
{
  int mask;

  if (dbus < 0 || dbus >= EVG_DBUS_BITS)
    return -1;

  if (map < 0 || map > C_EVG_DBUS_SEL_MASK)
    return -1;

  mask = ~(C_EVG_DBUS_SEL_MASK << (dbus*C_EVG_DBUS_SEL_BITS));
  pEg->DBusMap &= be32_to_cpu(mask);
  pEg->DBusMap |= be32_to_cpu(map << (dbus*C_EVG_DBUS_SEL_BITS));

  return 0;
}

void EvgDBusDump(volatile struct MrfEgRegs *pEg)
{
  int dbus;

  for (dbus = 0; dbus < EVG_DBUS_BITS; dbus++)
    {
      DEBUG_PRINTF("DBUS%d ", dbus);
      switch ((be32_to_cpu(pEg->DBusMap) >> (dbus*C_EVG_DBUS_SEL_BITS)) &
	      ((1 << C_EVG_DBUS_SEL_BITS) - 1))
	{
	case C_EVG_DBUS_SEL_OFF:
	  DEBUG_PRINTF("OFF\n");
	  break;
	case C_EVG_DBUS_SEL_EXT:
	  DEBUG_PRINTF("EXT\n");
	  break;
	case C_EVG_DBUS_SEL_MXC:
	  DEBUG_PRINTF("MXC\n");
	  break;
	case C_EVG_DBUS_SEL_FORWARD:
	  DEBUG_PRINTF("FWD\n");
	  break;
	default:
	  DEBUG_PRINTF("Unknown\n");
	  break;
	}
    }
}

int EvgSetACInput(volatile struct MrfEgRegs *pEg, int bypass, int sync, int div, int delay)
{
  unsigned int result;

  result = be32_to_cpu(pEg->ACControl);

  if (bypass == 0)
    result &= ~(1 << C_EVG_ACCTRL_BYPASS);
  if (bypass == 1)
    result |= (1 << C_EVG_ACCTRL_BYPASS);

  if (sync == 0)
    result &= ~(1 << C_EVG_ACCTRL_ACSYNC);
  if (sync == 1)
    result |= (1 << C_EVG_ACCTRL_ACSYNC);

  if (div > 0 && div < (2 << (C_EVG_ACCTRL_DIV_HIGH - C_EVG_ACCTRL_DIV_LOW)))
    {
      result &= ~((2 << C_EVG_ACCTRL_DIV_HIGH) - (1 << C_EVG_ACCTRL_DIV_LOW));
      result |= div << C_EVG_ACCTRL_DIV_LOW;
    }

  if (delay > 0 && delay < (2 << (C_EVG_ACCTRL_DELAY_HIGH - C_EVG_ACCTRL_DELAY_LOW)))
    {
      result &= ~((2 << C_EVG_ACCTRL_DELAY_HIGH) - (1 << C_EVG_ACCTRL_DELAY_LOW));
      result |= delay << C_EVG_ACCTRL_DELAY_LOW;
    }

  pEg->ACControl = be32_to_cpu(result);

  return 0;
}

int EvgSetACMap(volatile struct MrfEgRegs *pEg, int map)
{
  if (map > EVG_MAX_TRIGGERS)
    return -1;

  if (map >= 0)
    pEg->ACMap = be32_to_cpu(1 << map);
  else
    pEg->ACMap = 0;

  return 0;
}

void EvgACDump(volatile struct MrfEgRegs *pEg)
{
  unsigned int result;

  result = be32_to_cpu(pEg->ACControl);
  DEBUG_PRINTF(	"AC Input div %d delay %d",
  				(	result &
					(	(2 << C_EVG_ACCTRL_DIV_HIGH)
					-	(1 << C_EVG_ACCTRL_DIV_LOW)	) )
	       		>>	C_EVG_ACCTRL_DIV_LOW,
				(	result &
					(	(2 << C_EVG_ACCTRL_DELAY_HIGH)
					-	(1 << C_EVG_ACCTRL_DELAY_LOW) ) )
				>>	C_EVG_ACCTRL_DELAY_LOW	);
  if (result & (1 << C_EVG_ACCTRL_BYPASS))
    DEBUG_PRINTF(" BYPASS");
  if (result & (1 << C_EVG_ACCTRL_ACSYNC))
    DEBUG_PRINTF(" SYNCMXC7");
  DEBUG_PRINTF("\n");
}

int EvgSetRFInput(volatile struct MrfEgRegs *pEg, int useRF, int div)
{
  int rfdiv;

  rfdiv = be32_to_cpu(pEg->ClockControl);

  if (useRF == 0)
    rfdiv &= ~(1 << C_EVG_CLKCTRL_EXTRF);
  if (useRF == 1)
    rfdiv |= (1 << C_EVG_CLKCTRL_EXTRF);

  if (div >= 0 && div <= C_EVG_RFDIV_MASK)
    {
      rfdiv &= ~(C_EVG_RFDIV_MASK << C_EVG_CLKCTRL_DIV_LOW);
      rfdiv |= div << C_EVG_CLKCTRL_DIV_LOW;
    }
    
  pEg->ClockControl = be32_to_cpu(rfdiv);

  return 0;
}

int EvgSetFracDiv(volatile struct MrfEgRegs *pEg, int fracdiv)
{
  return pEg->FracDiv = be32_to_cpu(fracdiv);
}

int EvgSetSeqRamEvent(volatile struct MrfEgRegs *pEg, int ram, int pos, unsigned int timestamp, int code)
{
  if (ram < 0 || ram >= EVG_SEQRAMS)
    return -1;

  if (pos < 0 || pos >= EVG_MAX_SEQRAMEV)
    return -1;

  if (code < 0 || code > EVG_MAX_EVENT_CODE)
    return -1;

  pEg->SeqRam[ram][pos].Timestamp = be32_to_cpu(timestamp);
  pEg->SeqRam[ram][pos].EventCode = be32_to_cpu(code);

  return 0;
}

void EvgSeqRamDump(volatile struct MrfEgRegs *pEg, int ram)
{
  int pos;

  if (ram < 0 || ram >= EVG_SEQRAMS)
    return;
 
  for (pos = 0; pos < EVG_MAX_SEQRAMEV; pos++)
    if (pEg->SeqRam[ram][pos].EventCode)
      DEBUG_PRINTF("Ram%d: Timestamp %08x Code %02x\n",
		   ram,
		   be32_to_cpu(pEg->SeqRam[ram][pos].Timestamp),
		   be32_to_cpu(pEg->SeqRam[ram][pos].EventCode));
}

int EvgSeqRamControl(volatile struct MrfEgRegs *pEg, int ram, int enable, int single, int recycle, int reset, int trigsel)
{
  int control;

  if (ram < 0 || ram >= EVG_SEQRAMS)
    return -1;

  control = be32_to_cpu(pEg->SeqRamControl[ram]);

  if (enable == 0)
    control |= (1 << C_EVG_SQRC_DISABLE);
  if (enable == 1)
    control |= (1 << C_EVG_SQRC_ENABLE);

  if (single == 0)
    control &= ~(1 << C_EVG_SQRC_SINGLE);
  if (single == 1)
    control |= (1 << C_EVG_SQRC_SINGLE);
  
  if (recycle == 0)
    control &= ~(1 << C_EVG_SQRC_RECYCLE);
  if (recycle == 1)
    control |= (1 << C_EVG_SQRC_RECYCLE);
  
  if (reset == 1)
    control |= (1 << C_EVG_SQRC_RESET);

  if (trigsel >= 0 && trigsel <= C_EVG_SEQTRIG_MAX)
    {
      control &= ~(C_EVG_SEQTRIG_MAX << C_EVG_SQRC_TRIGSEL_LOW);
      control |= trigsel << C_EVG_SQRC_TRIGSEL_LOW;
    }

  pEg->SeqRamControl[ram] = be32_to_cpu(control);

  return 0;
}
					  
int EvgSeqRamSWTrig(volatile struct MrfEgRegs *pEg, int trig)
{
  if (trig < 0 || trig > 1)
    return -1;

  pEg->SeqRamControl[trig] |= be32_to_cpu(1 << C_EVG_SQRC_SWTRIGGER);
  
  return 0;
}

void EvgSeqRamStatus(volatile struct MrfEgRegs *pEg, int ram)
{
  int control;

  if (ram < 0 || ram >= EVG_SEQRAMS)
    return;
  
  control = be32_to_cpu(pEg->SeqRamControl[ram]);
  
  DEBUG_PRINTF("RAM%d:", ram);
  if (control & (1 << C_EVG_SQRC_RUNNING))
    DEBUG_PRINTF(" RUN");
  if (control & (1 << C_EVG_SQRC_ENABLED))
    DEBUG_PRINTF(" ENA");
  if (control & (1 << C_EVG_SQRC_SINGLE))
    DEBUG_PRINTF(" SINGLE");
  if (control & (1 << C_EVG_SQRC_RECYCLE))
    DEBUG_PRINTF(" RECYCLE");
  DEBUG_PRINTF(" Trigsel %02x\n", (control >> C_EVG_SQRC_TRIGSEL_LOW) & C_EVG_SEQTRIG_MAX);
}

int EvgSetUnivinMap(volatile struct MrfEgRegs *pEg, int univ, int trig, int dbus)
{
  int map = 0;

  if (univ < 0 || univ >= EVG_MAX_UNIVIN_MAP)
    return -1;

  if (trig >= EVG_MAX_TRIGGERS)
    return -1;

  if (dbus >= EVG_DBUS_BITS)
    return -1;

  if (trig >= 0)
    map |= (1 << (C_EVG_INMAP_TRIG_BASE + trig));

  if (dbus >= 0)
    map |= (1 << (C_EVG_INMAP_DBUS_BASE + dbus));

  pEg->UnivInMap[univ] = be32_to_cpu(map);
  return 0;
}

void EvgUnivinDump(volatile struct MrfEgRegs *pEg)
{
  int univ;

  for (univ = 0; univ < EVG_UNIVINS; univ++)
    {
      DEBUG_PRINTF( "UnivIn%d Mapped to Trig %08x, DBus %02x\n", univ,
		   (be32_to_cpu(pEg->UnivInMap[univ]) >> C_EVG_INMAP_TRIG_BASE)
		   & ((1 << EVG_MAX_TRIGGERS) - 1),
		   (be32_to_cpu(pEg->UnivInMap[univ]) >> C_EVG_INMAP_DBUS_BASE)
		   & ((1 << EVG_DBUS_BITS) - 1));
    }
}

int EvgSetTriggerEvent(volatile struct MrfEgRegs *pEg, int trigger, int code, int enable)
{
  int result;

  if (trigger < 0 || trigger >= EVG_TRIGGERS)
    return 0;

  result = be32_to_cpu(pEg->EventTrigger[trigger]);
					     
  if (code >= 0 && code <= EVG_MAX_EVENT_CODE)
    {
      result &= (1 << (C_EVG_EVENTTRIG_CODE_HIGH + 1)) -
	(1 << C_EVG_EVENTTRIG_CODE_LOW);
      result |= (code << C_EVG_EVENTTRIG_CODE_LOW);
    }

  if (enable == 0)
    result &= ~(1 << C_EVG_EVENTTRIG_ENABLE);
  if (enable == 1)
    result |= (1 << C_EVG_EVENTTRIG_ENABLE);

  pEg->EventTrigger[trigger] = be32_to_cpu(result);

  return 0;
}

void EvgTriggerEventDump(volatile struct MrfEgRegs *pEg)
{
  int trigger, result;

  for (trigger = 0; trigger < EVG_TRIGGERS; trigger++)
    {
      result = be32_to_cpu(pEg->EventTrigger[trigger]);
      printf("Trigger%d code %02x %s\n",
	     trigger, (result & ((1 << (C_EVG_EVENTTRIG_CODE_HIGH + 1)) -
				 (1 << C_EVG_EVENTTRIG_CODE_LOW))) >>
	     C_EVG_EVENTTRIG_CODE_LOW, result & (1 << C_EVG_EVENTTRIG_ENABLE) ?
	     "ENABLED" : "DISABLED");
    }
}

int EvgSetUnivOutMap(volatile struct MrfEgRegs *pEg, int output, int map)
{
  pEg->UnivOutMap[output] = be16_to_cpu(map);
  return 0;
}

int EvgSetDBufMode(volatile struct MrfEgRegs *pEg, int enable)
{
  if (enable)
    pEg->DataBufControl = be32_to_cpu(1 << C_EVG_DATABUF_MODE);
  else
    pEg->DataBufControl = 0;

  return EvgGetDBufStatus(pEg);
}

int EvgGetDBufStatus(volatile struct MrfEgRegs *pEg)
{
  return be32_to_cpu(pEg->DataBufControl);
}

int EvgSendDBuf(volatile struct MrfEgRegs *pEg, char *dbuf, int size)
{
  int stat;

  stat = EvgGetDBufStatus(pEg);
  //  printf("EvgSendDBuf: stat %08x\n", stat);
  /* Check that DBUF mode enabled */
  if (!(stat & (1 << C_EVG_DATABUF_MODE)))
    return -1;
  /* Check that previous transfer is completed */
  if (!(stat & (1 << C_EVG_DATABUF_COMPLETE)))
    return -1;
  /* Check that size is valid */
  if (size & 3 || size > EVG_MAX_BUFFER || size < 4)
    return -1;

  memcpy((void *) &pEg->Databuf[0], (void *) dbuf, size);

  /* Enable and set size */
  stat &= ~((EVG_MAX_BUFFER-1) | (1 << C_EVG_DATABUF_TRIGGER));
  stat |= (1 << C_EVG_DATABUF_ENA) | size;
  //  printf("EvgSendDBuf: stat %08x\n", stat);
  pEg->DataBufControl = be32_to_cpu(stat);
  //  printf("EvgSendDBuf: stat %08x\n", be32_to_cpu(pEg->DataBufControl));

  /* Trigger */
  pEg->DataBufControl = be32_to_cpu(stat | (1 << C_EVG_DATABUF_TRIGGER));
  //  printf("EvgSendDBuf: stat %08x\n", be32_to_cpu(pEg->DataBufControl));

  return size;
}

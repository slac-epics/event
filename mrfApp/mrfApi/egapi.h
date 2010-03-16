/*
  egapi.h -- Definitions for Micro-Research Event Generator
             Application Programming Interface

  Author: Jukka Pietarinen (MRF)
  Date:   05.12.2006

*/

/*
  Note: Byte ordering is big-endian.
 */

#define EVG_MEM_WINDOW      0x00010000

#ifndef u16
#define u16 unsigned short
#endif
#ifndef u32
#define u32 unsigned long
#endif

#if __BYTE_ORDER == __LITTLE_ENDIAN
#define be16_to_cpu(x) bswap_16(x)
#define be32_to_cpu(x) bswap_32(x)
#else
#define be16_to_cpu(x) ((unsigned short)(x))
#define be32_to_cpu(x) ((unsigned long)(x))
#endif

#define EVG_MAX_FPOUT_MAP   32
#define EVG_MAX_UNIVOUT_MAP 32
#define EVG_MAX_TBOUT_MAP   64
#define EVG_MAX_FPIN_MAP    16
#define EVG_MAX_UNIVIN_MAP  16
#define EVG_MAX_TBIN_MAP    64
#define EVG_MAX_BUFFER      2048
#define EVG_MAX_SEQRAMEV    2048
#define EVG_MAX_SEQRAMS     4
#define EVG_MAX_EVENT_CODE  255
#define EVG_MAX_MXCS        16
#define EVG_MAX_TRIGGERS    16
#define EVG_SEQRAM_CLKSEL_BITS 5
#define C_EVG_SEQRAM_MAX_TRIG_BITS 5
#define EVG_DBUS_BITS       8
#define EVG_SIGNAL_MAP_BITS 6

struct SeqRamItemStruct {
  u32  Timestamp;
  u32  EventCode;
};

struct EvanStruct {
  u32 TimestampHigh;
  u32 TimestampLow;
  u32 EventCode;
};

struct MXCStruct {
  u32 Control;
  u32 Prescaler;
};

struct MrfEgRegs {
  u32  Status;                              /* 0000: Status Register */
  u32  Control;                             /* 0004: Main Control Register */
  u32  IrqFlag;                             /* 0008: Interrupt Flags */
  u32  IrqEnable;                           /* 000C: Interrupt Enable */
  u32  ACControl;                           /* 0010: AC divider control */
  u32  ACMap;                               /* 0014: AC event mapping */
  u32  SWEvent;                             /* 0018: Software Event */
  u32  Resv0x001C;                          /* 001C: Reserved */
  u32  DataBufControl;                      /* 0020: Data Buffer Control */
  u32  DBusMap;                             /* 0024: Distributed Bus Mapping */
  u32  Resv0x0028;                          /* 0028: Reserved */
  u32  FPGAVersion;                         /* 002C: FPGA version */
  u32  Resv0x0030[(0x04C-0x030)/4];         /* 0030-004B: Reserved */
  u32  UsecDiv;                             /* 004C: round(Event clock/1 MHz) */
  u32  ClockControl;                        /* 0050: Clock Control */
  u32  Resv0x0054[(0x060-0x054)/4];         /* 0054-005F: Reserved */
  u32  EvanControl;                         /* 0060: Event Analyzer Control */
  u32  EvanCode;                            /* 0064: Event Analyzer Event Code
					       and DBus data */
  u32  EvanTimeH;                           /* 0068: Event Analyzer Timestamp
					       higher long word */
  u32  EvanTimeL;                           /* 006C: Event Analyzer Timestamp
					       lower long word */
  u32  SeqRamControl[EVG_MAX_SEQRAMS];      /* 0070-007F: Sequence RAM Control Registers */
  u32  FracDiv;                             /* 0080: Fractional Synthesizer SY87739L Control Word */
  u32  Resv0x0084;                          /* 0084: Reserved */
  u32  RxInitPS;                            /* 0088: Initial value for RF Recovery DCM phase */
  u32  Resv0x008Cto0x0100[(0x100-0x08C)/4]; /* 008C-00FF: Reserved */
  u32  EventTrigger[EVG_MAX_TRIGGERS];      /* 0100-013F: Event Trigger Registers */
  u32  Resv0x0140to0x017F[(0x180-0x140)/4]; /* 0140-017F: Reserved */
  struct MXCStruct MXC[EVG_MAX_MXCS];       /* 0180-01FF: Multiplexed Counter Prescalers */
  u32  Resv0x01C0to0x03FC[(0x400-0x200)/4]; /* 0200-03FF: Reserved */
  u16  FPOutMap[EVG_MAX_FPOUT_MAP];         /* 0400-043F: Front panel output mapping */
  u16  UnivOutMap[EVG_MAX_UNIVOUT_MAP];     /* 0440-047F: Universal I/O output mapping */
  u16  TBOutMap[EVG_MAX_TBOUT_MAP];         /* 0480-04FF: TB output mapping */
  u32  FPInMap[EVG_MAX_FPIN_MAP];           /* 0500-053F: Front panel input mapping */
  u32  UnivInMap[EVG_MAX_UNIVIN_MAP];       /* 0540-057F: Universal I/O input mapping */
  u32  Resv0x0580[(0x600-0x580)/4];         /* 0580-05FF: Reserved */
  u32  TBInMap[EVG_MAX_TBIN_MAP];           /* 0600-06FF: TB input mapping */
  u32  Resv0x0700[(0x800-0x700)/4];         /* 0700-07FF: Reserved */
  u32  Databuf[EVG_MAX_BUFFER/4];           /* 0800-0FFF: Data Buffer */
  u32  Resv0x1000[(0x8000-0x1000)/4];       /* 1000-7FFF: Reserved */
  struct SeqRamItemStruct SeqRam[EVG_SEQRAMS][EVG_MAX_SEQRAMEV];
                                            /* 8000-BFFF: Sequence RAM 1 */
                                            /* C000-FFFF: Sequence RAM 2 */
};

/* -- Status Register bit mappings */
#define C_EVG_STATUS_RXDBUS_HIGH    31
#define C_EVG_STATUS_RXDBUS_LOW     24
#define C_EVG_STATUS_TXDBUS_HIGH    23
#define C_EVG_STATUS_TXDBUS_LOW     16
/* -- Control Register bit mappings */
#define C_EVG_CTRL_MASTER_ENABLE    31
#define C_EVG_CTRL_RX_DISABLE       30
#define C_EVG_CTRL_MXC_RESET        24
#define C_EVG_CTRL_SEQRAM_ALT       16
/* -- Interrupt Flag/Enable Register bit mappings */
#define C_EVG_IRQ_MASTER_ENABLE  31
#define C_EVG_IRQFLAG_DATABUF    5
#define C_EVG_IRQFLAG_RXFIFOFULL 1
#define C_EVG_IRQFLAG_VIOLATION  0
/* -- SW Event Register bit mappings */
#define C_EVG_SWEVENT_PENDING    9
#define C_EVG_SWEVENT_ENABLE     8
#define C_EVG_SWEVENT_CODE_HIGH  7
#define C_EVG_SWEVENT_CODE_LOW   0
/* -- AC Input Control Register bit mappings */
#define C_EVG_ACCTRL_BYPASS      17
#define C_EVG_ACCTRL_ACSYNC      16
#define C_EVG_ACCTRL_DIV_HIGH    15
#define C_EVG_ACCTRL_DIV_LOW     8
#define C_EVG_ACCTRL_DELAY_HIGH  7
#define C_EVG_ACCTRL_DELAY_LOW   0
/* -- AC Input Mapping Register bit mappings */
#define C_EVG_ACMAP_TRIG_BASE   0
/* -- Databuffer Control Register bit mappings */
#define C_EVG_DATABUF_COMPLETE   20
#define C_EVG_DATABUF_RUNNING    19
#define C_EVG_DATABUF_TRIGGER    18
#define C_EVG_DATABUF_ENA        17
#define C_EVG_DATABUF_MODE       16
#define C_EVG_DATABUF_SIZEHIGH   11
#define C_EVG_DATABUF_SIZELOW    2
/* -- Clock Control Register bit mapppings */
#define C_EVG_CLKCTRL_EXTRF        24
#define C_EVG_CLKCTRL_DIV_HIGH     21
#define C_EVG_CLKCTRL_DIV_LOW      16
#define C_EVG_CLKCTRL_RECDCM_RUN    15
#define C_EVG_CLKCTRL_RECDCM_INITD  14
#define C_EVG_CLKCTRL_RECDCM_PSDONE 13
#define C_EVG_CLKCTRL_EVDCM_STOPPED 12
#define C_EVG_CLKCTRL_EVDCM_LOCKED 11
#define C_EVG_CLKCTRL_EVDCM_PSDONE 10
#define C_EVG_CLKCTRL_CGLOCK       9
#define C_EVG_CLKCTRL_RECDCM_PSDEC 8
#define C_EVG_CLKCTRL_RECDCM_PSINC 7
#define C_EVG_CLKCTRL_RECDCM_RESET 6
#define C_EVG_CLKCTRL_EVDCM_PSDEC  5
#define C_EVG_CLKCTRL_EVDCM_PSINC  4
#define C_EVG_CLKCTRL_EVDCM_SRUN   3
#define C_EVG_CLKCTRL_EVDCM_SRES   2
#define C_EVG_CLKCTRL_EVDCM_RES    1
#define C_EVG_CLKCTRL_USE_RXRECCLK 0
/* -- RF Divider Settings */
#define C_EVG_RFDIV_MASK 0x003F
#define C_EVG_RFDIV_1    0x00
#define C_EVG_RFDIV_2    0x01
#define C_EVG_RFDIV_3    0x02
#define C_EVG_RFDIV_4    0x03
#define C_EVG_RFDIV_5    0x04
#define C_EVG_RFDIV_6    0x05
#define C_EVG_RFDIV_7    0x06
#define C_EVG_RFDIV_8    0x07
#define C_EVG_RFDIV_9    0x08
#define C_EVG_RFDIV_10   0x09
#define C_EVG_RFDIV_11   0x0A
#define C_EVG_RFDIV_12   0x0B
#define C_EVG_RFDIV_OFF  0x0C
#define C_EVG_RFDIV_14   0x0D
#define C_EVG_RFDIV_15   0x0E
#define C_EVG_RFDIV_16   0x0F
#define C_EVG_RFDIV_17   0x10
#define C_EVG_RFDIV_18   0x11
#define C_EVG_RFDIV_19   0x12
#define C_EVG_RFDIV_20   0x13
/* -- Event Analyser Control Register bit mappings */
#define C_EVG_EVANCTRL_RESET       3
#define C_EVG_EVANCTRL_NOTEMPTY    3
#define C_EVG_EVANCTRL_CLROVERFLOW 2
#define C_EVG_EVANCTRL_OVERFLOW    2
#define C_EVG_EVANCTRL_ENABLE      1
#define C_EVG_EVANCTRL_COUNTRES    0
/* -- Sequence RAM Control Register bit mappings */
#define C_EVG_SQRC_RUNNING         25
#define C_EVG_SQRC_ENABLED         24
#define C_EVG_SQRC_SWTRIGGER       21
#define C_EVG_SQRC_SINGLE          20
#define C_EVG_SQRC_RECYCLE         19
#define C_EVG_SQRC_RESET           18
#define C_EVG_SQRC_DISABLE         17
#define C_EVG_SQRC_ENABLE          16
#define C_EVG_SQRC_TRIGSEL_LOW     0
/* -- Sequence RAM Triggers */
#define C_EVG_SEQTRIG_MAX          31
#define C_EVG_SEQTRIG_SWTRIGGER1   18
#define C_EVG_SEQTRIG_SWTRIGGER0   17
#define C_EVG_SEQTRIG_ACINPUT      16
#define C_EVG_SEQTRIG_MXC_BASE     0
#define EVG_SEQTRIG_SWTRIGGER1     (1 << C_EVG_SEQTRIG_SWTRIGGER1)
#define EVG_SEQTRIG_SWTRIGGER0     (1 << C_EVG_SEQTRIG_SWTRIGGER0)
#define EVG_SEQTRIG_ACINPUT        (1 << C_EVG_SEQTRIG_ACINPUT)
/* -- Event Trigger bit mappings */
#define C_EVG_EVENTTRIG_ENABLE     8
#define C_EVG_EVENTTRIG_CODE_HIGH  7
#define C_EVG_EVENTTRIG_CODE_LOW   0
/* -- Input mapping bits */
#define C_EVG_INMAP_TRIG_BASE      0
#define C_EVG_INMAP_DBUS_BASE      16
/* -- Multiplexed Counter Mapping Bits */
#define C_EVG_MXC_READ             31
#define C_EVG_MXCMAP_TRIG_BASE     0
/* -- Distributed Bus Mapping Bits */
#define C_EVG_DBUS_SEL_BITS        4
#define C_EVG_DBUS_SEL_MASK        3
#define C_EVG_DBUS_SEL_OFF         0
#define C_EVG_DBUS_SEL_EXT         1
#define C_EVG_DBUS_SEL_MXC         2
#define C_EVG_DBUS_SEL_FORWARD     3
/* -- C_SIGNAL_MAP_DBUS defines the starting index of DBUS bit 0 */
#define C_SIGNAL_MAP_DBUS          32
/* -- C_SIGNAL_MAP_PRESC defines the starting index of the prescaler outputs */
#define C_SIGNAL_MAP_PRESC         40
/* -- C_SIGNAL_MAP_HIGH defines the index for state high output */
/* -- undefined indexes drive the output low */
#define C_SIGNAL_MAP_HIGH          62
#define C_SIGNAL_MAP_LOW           63

/* Function prototypes */
int EvgOpen(struct MrfEgRegs **pEg, char *device_name);
int EvgClose(int fd);
int EvgEnable(volatile struct MrfEgRegs *pEg, int state);
int EvgGetEnable(volatile struct MrfEgRegs *pEg);
int EvgRxEnable(volatile struct MrfEgRegs *pEg, int state);
int EvgRxGetEnable(volatile struct MrfEgRegs *pEg);
int EvgGetViolation(volatile struct MrfEgRegs *pEg, int clear);
int EvgSWEventEnable(volatile struct MrfEgRegs *pEg, int state);
int EvgGetSWEventEnable(volatile struct MrfEgRegs *pEg);
int EvgSendSWEvent(volatile struct MrfEgRegs *pEg, int code);
int EvgEvanEnable(volatile struct MrfEgRegs *pEg, int state);
int EvgEvanGetEnable(volatile struct MrfEgRegs *pEg);
void EvgEvanReset(volatile struct MrfEgRegs *pEg);
void EvgEvanResetCount(volatile struct MrfEgRegs *pEg);
int EvgEvanGetEvent(volatile struct MrfEgRegs *pEg, struct EvanStruct *evan);
int EvgSetMXCPrescaler(volatile struct MrfEgRegs *pEg, int mxc, unsigned int presc);
int EvgSetMxcTrigMap(volatile struct MrfEgRegs *pEg, int mxc, int map);
void EvgSyncMxc(volatile struct MrfEgRegs *pEg);
void EvgMXCDump(volatile struct MrfEgRegs *pEg);
int EvgSetDBusMap(volatile struct MrfEgRegs *pEg, int dbus, int map);
void EvgDBusDump(volatile struct MrfEgRegs *pEg);
int EvgSetACInput(volatile struct MrfEgRegs *pEg, int bypass, int sync, int div, int delay);
int EvgSetACMap(volatile struct MrfEgRegs *pEg, int map);
void EvgACDump(volatile struct MrfEgRegs *pEg);
int EvgSetRFInput(volatile struct MrfEgRegs *pEg, int useRF, int div);
int EvgSetFracDiv(volatile struct MrfEgRegs *pEg, int fracdiv);
int EvgSetSeqRamEvent(volatile struct MrfEgRegs *pEg, int ram, int pos, unsigned int timestamp, int code);
void EvgSeqRamDump(volatile struct MrfEgRegs *pEg, int ram);
int EvgSeqRamControl(volatile struct MrfEgRegs *pEg, int ram, int enable, int single, int recycle, int reset, int trigsel);
int EvgSeqRamSWTrig(volatile struct MrfEgRegs *pEg, int trig);
void EvgSeqRamStatus(volatile struct MrfEgRegs *pEg, int ram);
int EvgSetUnivinMap(volatile struct MrfEgRegs *pEg, int univ, int trig, int dbus);
void EvgUnivinDump(volatile struct MrfEgRegs *pEg);
int EvgSetTriggerEvent(volatile struct MrfEgRegs *pEg, int trigger, int code, int enable);
void EvgTriggerEventDump(volatile struct MrfEgRegs *pEg);
int EvgSetUnivOutMap(volatile struct MrfEgRegs *pEg, int output, int map);
int EvgSetDBufMode(volatile struct MrfEgRegs *pEg, int enable);
int EvgGetDBufStatus(volatile struct MrfEgRegs *pEg);
int EvgSendDBuf(volatile struct MrfEgRegs *pEg, char *dbuf, int size);

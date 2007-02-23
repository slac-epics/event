/***************************************************************************************************
|* drvMrfEg.h -- Driver Support Layer Interface Definitions for the Micro-Research Finland (MRF)
|*               Event Generator Card
|*
|*--------------------------------------------------------------------------------------------------
|* Author:   Dayle Kotturi (SLAC)
|* Date:     31 March 2006
|*
|*--------------------------------------------------------------------------------------------------
|* MODIFICATION HISTORY:
|* 31 Mar 2006  D.Kotturi       Original
|*
|*--------------------------------------------------------------------------------------------------
|* MODULE DESCRIPTION:
|*
|* This header file describes the EPICS driver support layer programming interface to the 
|* Micro-Research Finland (MRF) Event Generator (EVG) card.  This interface is primarly
|* used by the EPICS device-support layer to communicate with the hardware. 
|*
\**************************************************************************************************/

/**************************************************************************************************
|*                                     COPYRIGHT NOTIFICATION
|**************************************************************************************************
|*  
|* THE FOLLOWING IS A NOTICE OF COPYRIGHT, AVAILABILITY OF THE CODE,
|* AND DISCLAIMER WHICH MUST BE INCLUDED IN THE PROLOGUE OF THE CODE
|* AND IN ALL SOURCE LISTINGS OF THE CODE.
|*
|**************************************************************************************************
|*
|* Copyright (c) 2006 The University of Chicago,
|* as Operator of Argonne National Laboratory.
|*
|* Copyright (c) 2006 The Regents of the University of California,
|* as Operator of Los Alamos National Laboratory.
|*
|* Copyright (c) 2006 The Board of Trustees of the Leland Stanford Junior
|* University, as Operator of the Stanford Linear Accelerator Center.
|*
|**************************************************************************************************
|*
|* This software is distributed under the EPICS Open License Agreement which
|* can be found in the file, LICENSE, included in this directory.
|*
\*************************************************************************************************/

#ifndef DRV_MRF_EG_H 
#define DRV_MRF_EG_H


#include <epicsEvent.h>         /* epicsEvent sem */
#include <epicsTypes.h>         /* EPICS Architecture-independent type definitions                */
#include <epicsMutex.h>         /* EPICS Mutex support library                   */
                                                                                
#include <ellLib.h>             /* EPICS Linked list library routines and definitions             */
#include <dbScan.h>             /* EPICS Database scan routines and definitions                   */

#include <mrfCommon.h>          /* MRF event system constants and definitions                     */

/**************************************************************************************************/
/*  Configuration Constants                   */
/**************************************************************************************************/

/*---------------------
 * Define the maximum size of the interrupt debug messages
 */
#define EVG_INT_MSG_LEN     256

/*---------------------
 * Define the maximum size of the distributed data buffer
 */
#define EVG_MAX_BUFFER      MRF_MAX_DATA_BUFFER

/*---------------------
 * Define the maximum number of event generator cards allowed
 */
#define EVG_MAX_CARDS           21

/*---------------------
 * Event Generator Distributed Data Stream Definitions
 */
#define EVG_DBUF_TRIGGER    0x04
#define EVG_DBUF_ENABLE     0x02
#define EVG_DBUF_MODE       0x01

/*---------------------
 * Event Generator card structure prototype
 */
typedef struct EgCardStruct EgCardStruct;

/*---------------------
 * Function prototypes for the device-support error, event, and data buffer ready
 * notification functions.
 */
typedef void (*DEV_EVENT_FUNC)  (EgCardStruct *pCard, epicsInt16 EventNum, epicsUInt32 Ticks);
typedef void (*DEV_ERROR_FUNC)  (EgCardStruct *pCard, int ErrorNum);
typedef void (*DEV_DBUFF_FUNC)  (EgCardStruct *pCard, epicsInt16 Size, void *Buffer);

typedef struct MrfEVGRegs
{
  epicsUInt16 Control   ;
  epicsUInt16 EventMask ;
  epicsUInt16 VmeEvent  ;       /* and distributed bus data (byte wide) */
  epicsUInt16 obsolete_Seq2Addr  ;
  epicsUInt16 obsolete_Seq2Data  ;
  epicsUInt16 obsolete_Seq1Addr  ;
  epicsUInt16 obsolete_Seq1Data  ;

  epicsUInt16 Event0Map   ;     /**< Event Mapping Register for external (HW) input 0 */
  epicsUInt16 Event1Map   ;     /**< Event Mapping Register for external (HW) input 1 */
  epicsUInt16 Event2Map   ;     /**< Event Mapping Register for external (HW) input 2 */
  epicsUInt16 Event3Map   ;     /**< Event Mapping Register for external (HW) input 3 */
  epicsUInt16 Event4Map   ;     /**< Event Mapping Register for external (HW) input 4 */
  epicsUInt16 Event5Map   ;     /**< Event Mapping Register for external (HW) input 5 */
  epicsUInt16 Event6Map   ;     /**< Event Mapping Register for external (HW) input 6 */
  epicsUInt16 Event7Map   ;     /**< Event Mapping Register for external (HW) input 7 */

  /* Extended registers */
  epicsUInt16 MuxCountDbEvEn;   /**< Multiplexed counter distributed bus (byte wide) */
                                            /**< enables and event trigger enables (byte wide) */
  epicsUInt16 obsolete_Seq1ExtAddr  ;      /**< Extended address SEQ RAM 1 */
  epicsUInt16 obsolete_Seq2ExtAddr  ;      /**< Extended address SEQ RAM 2 */
  epicsUInt16 Seq1ClockSel  ;     /**< Sequence RAM 1 clock selection */
  epicsUInt16 Seq2ClockSel  ;     /**< Sequence RAM 2 clock selection */
  epicsUInt16 ACinputControl;     /**< AC related controls register */
  epicsUInt16 MuxCountSelect;     /**< reset, Mux seq. trigger enable and Mux select */
  epicsUInt16 MuxPrescaler;       /**< Mux prescaler values (25MHz/value) */
  epicsUInt16 FPGAVersion ;      /**< FPGA firmware register number (only series 100) */
  epicsUInt32 Reserved_0x30;
  epicsUInt32 Reserved_0x34;
  epicsUInt32 Reserved_0x38;
  epicsUInt32 Reserved_0x3C;
  epicsUInt16 RfSelect;         /* 0x40, RF Clock Selection register */
  epicsUInt16 MxcPolarity;      /* 0x42, Mpx counter reset polarity */
  epicsUInt16 Seq1Addr;         /* 0x44, 11 bit address, 0-2047 (2 kB event RAM) */
  epicsUInt16 Seq1Data;         /* 0x46, Event code, 8 bits */
  epicsUInt32 Seq1Time;         /* 0x48, Time offset in event clock from trigger */
  epicsUInt32 Seq1Pos;          /* 0x52, Current sequence time position */
  epicsUInt16 Seq2Addr;         /* 0x56, 11 bit address, 0-2047 */
  epicsUInt16 Seq2Data;         /* 0x58,  Event code, 8 bits */
  epicsUInt32 Seq2Time;         /**< Time offset in event clock from trigger */
  epicsUInt32 Seq2Pos;          /**< Current sequence time position */
  epicsUInt16 EvanControl;      /**< Event analyser control register */
  epicsUInt16 EvanEvent;        /**< Event analyser event code, 8 bits */
  epicsUInt32 EvanTimeH;        /**< Bits 63-32 of event analyser time counter */
  epicsUInt32 EvanTimeL;        /**< Bits 31-0 of event analyser time counter */
  epicsUInt16 uSecDiv;          /* 0x68, Divider to get from event clock to ~1 MHz */
  epicsUInt16 DataBufControl;  /* 0x6A, Data Buffer control register */
  epicsUInt16 DataBufSize;    /* 0x6C, Data Buffer transfer size in bytes,
                            multiple of four */
  epicsUInt16 DBusEvents;  /* 0x6E, Distributed bus events enable, these are for
                         special event codes:
                         DBUS7: event code 0x7D, reset prescaler/load seconds
                         DBUS6: event code 0x71, seconds '1'
                         DBUS5: event code 0x70, seconds '0' */
  epicsUInt32 Reserved_0x70;
  epicsUInt32 Reserved_0x74;
  epicsUInt32 Reserved_0x78;
  epicsUInt32 Reserved_0x7C;
  epicsUInt32 FracDivControl;   /* 0x80, Micrel SY87739L programming word */
  epicsUInt32 DelayRf;          /* 0x84 */
  epicsUInt32 DelayRx;          /* 0x88 */
  epicsUInt32 DelayTx;          /* 0x8C */
  epicsUInt32 AdiControl;       /* 0x90 */              
  epicsUInt32 FbTxFrac;         /* 0x94 */
  epicsUInt32 DelayRFInit;      /* 0x98, Init value for RF Delay Chip */
  epicsUInt32 DelayRxInit;      /* 0x9C, Init value for Rx Delay Chip */
  epicsUInt32 DelayTxInit;      /* 0xA0, Init value for Tx Delay Chip */
  epicsUInt32 Reserved_0xA4;
  epicsUInt32 Reserved_0xA8;
  epicsUInt32 Reserved_0xAC;
  epicsUInt32 Reserved_0xB0;
  epicsUInt32 Reserved_0xB4;
  epicsUInt32 Reserved_0xB8;
  epicsUInt32 Reserved_0xBC;
  epicsUInt32 Reserved_0xC0;
  epicsUInt32 Reserved_0xC4;
  epicsUInt32 Reserved_0xC8;
  epicsUInt32 Reserved_0xCC;
  epicsUInt32 Reserved_0xD0;
  epicsUInt32 Reserved_0xD4;
  epicsUInt32 Reserved_0xD8;
  epicsUInt32 Reserved_0xDC;
  epicsUInt32 Reserved_0xE0;
  epicsUInt32 Reserved_0xE4;
  epicsUInt32 Reserved_0xE8;
  epicsUInt32 Reserved_0xEC;
  epicsUInt32 Reserved_0xF0;
  epicsUInt32 Reserved_0xF4;
  epicsUInt32 Reserved_0xF8;
  epicsUInt32 Reserved_0xFC;
  epicsUInt32 Reserved_0x100[0x700/4];
  epicsUInt32 DataBuffer[0x200];        /* 0x800 */
}MrfEVGRegs;

/*************************************************************************************************/
/*  Event Generator Card Structure Definition                                                    */
/*************************************************************************************************/
struct EgCardStruct {
    ELLNODE               Link;         /* Linked list node structure                            */
    volatile MrfEVGRegs  *pEg;          /* pointer to card described.                            */
	/* Changed name from Card -> Cardno to make sure nobody uses
     * this field with the old semantics.
	 */
    int                   Cardno;       /* Logical card number                                   */
    int                   Slot;         /* Card slot in crate where EVR is starting from 1       */
    epicsMutexId          EgLock;       /* Mutex to lock access to the card                      */
    struct egRecord      *pEgRec;       /* Record that represents the card                       */
    double                Ram1Speed;    /* in Hz                                                 */
    long                  Ram1Dirty;    /* needs to be reloaded                                  */
    double                Ram2Speed;    /* in Hz                                                 */
    long                  Ram2Dirty;    /* needs to be reloaded                                  */
    ELLLIST               EgEvent;      /* RAM event list head. All events are kept in one list. */
    long                  MaxRamPos;    /* Operation limit of RAM                                */
    double                ClockSpeed;   /* clock speed in MHz                                    */
}; /*EgCardStruct*/

/*---------------------
 * Dummy type definitions for user-defined event, error, and data buffer call-back functions.
 * (Used only in the driver support module. Device support will define the real prototypes)
 */
#define EVG_DRIVER_SUPPORT_MODULE   /* Indicates we are in the driver support module environment */
#ifdef EVG_DRIVER_SUPPORT_MODULE
    typedef void (*EVENT_FUNC) (void);
    typedef void (*ERROR_FUNC) (void);
    typedef void (*DBUFF_FUNC) (void);
#endif
                                                                                
typedef struct MrfEvgSeqStruct {
    epicsUInt32 Timestamp;
    epicsUInt16 EventCode;
} MrfEvgSeqStruct;


typedef struct {
    ELLNODE                node;
    struct egeventRecord  *pRec;
} EgEventNode;


/**************************************************************************************************/
/*  Function Prototypes For Driver Support Routines                                               */
/**************************************************************************************************/

long EgCheckCard(int Card);   /* called by dev. needs to be global */
long EgStartRamTask(void);    /* "" */
long EgScheduleRamProgram(int card);
long EgGetRamEvent(EgCardStruct *pParm, long Ram, long Addr);
long EgCheckTaxi(EgCardStruct *pParm);
long EgDisableFifo(EgCardStruct *pParm); 
long EgEnableFifo(EgCardStruct *pParm);
long EgCheckFifo(EgCardStruct *pParm);
long EgMasterEnable(EgCardStruct *pParm);
long EgMasterDisable(EgCardStruct *pParm);
long EgRamClockSet(EgCardStruct *pParm, long Ram, long Clock);
long EgRamClockGet(EgCardStruct *pParm, long Ram);
long EgMasterEnableGet(EgCardStruct *pParm);
long EgSetTrigger(EgCardStruct *pParm, unsigned int Channel, unsigned short Event);
long EgGetTrigger(EgCardStruct *pParm, unsigned int Channel);
long EgClearSeq(EgCardStruct *pParm, int channel);


long EgResetAll(EgCardStruct *pParm);
long EgEnableTrigger(EgCardStruct *pParm, unsigned int Channel, int state);
int  EgSeqEnableCheck(EgCardStruct *pParm, unsigned int Seq);
long EgEnableVme(EgCardStruct *pParm, int state);
long EgGenerateVmeEvent(EgCardStruct *pParm, int Event);
/* Sequencer-related routines */
long EgSeqTrigger(EgCardStruct *pParm, unsigned int Seq);
long EgSetSeqMode(EgCardStruct *pParm, unsigned int Seq, int Mode);
int  EgReadSeqRam(EgCardStruct *pParm, int channel, unsigned char *pBuf);
int  EgWriteSeqRam(EgCardStruct *pParm, int channel, unsigned char *pBuf);
long EgGetAltStatus(EgCardStruct *pParm, int Ram);
long EgEnableAltRam(EgCardStruct *pParm, int Ram);
long EgSetSingleSeqMode(EgCardStruct *pParm, int Ram);
long EgDisableRam(EgCardStruct *pParm, int Ram);
long EgEnableRam(EgCardStruct *pParm, int Ram);
long EgGetMode(EgCardStruct *pParm, int ram, int *pBusy, int *pEnable);

long EgGetEnableTrigger(EgCardStruct *pParm, unsigned int Channel);
long EgGetBusyStatus(EgCardStruct *pParm, int Ram);
long EgGetEnableMuxDistBus(EgCardStruct *pParm, unsigned int Channel);
long EgGetEnableMuxEvent(EgCardStruct *pParm, unsigned int Channel);
long EgEnableMuxDistBus(EgCardStruct *pParm, unsigned int Channel, int state);
long EgEnableMuxEvent(EgCardStruct *pParm, unsigned int Channel, int state);
long EgGetACinputControl(EgCardStruct *pParm, unsigned int Channel);
long EgSetACinputControl(EgCardStruct *pParm, unsigned int Channel, int state);
long EgGetACinputDivisor(EgCardStruct *pParm, unsigned short DlySel);
long EgSetACinputDivisor(EgCardStruct *pParm, unsigned short Divisor, unsigned short DlySel);
long EgResetMuxCounter(EgCardStruct *pParm, unsigned int Channel);
long EgResetMPX(EgCardStruct *pParm, unsigned int Mask);
long EgGetEnableMuxSeq(EgCardStruct *pParm, unsigned int SeqNum);
long EgEnableMuxSeq(EgCardStruct *pParm, unsigned int SeqNum, int state);
unsigned long EgGetMuxPrescaler(EgCardStruct *pParm, unsigned short Channel);
unsigned long EgSetMuxPrescaler(EgCardStruct *pParm, unsigned short Channel, unsigned long Divisor);
long EgGetFpgaVersion(EgCardStruct *pParm);
int EgSeqRamRead(EgCardStruct *pParm, int ram, unsigned short address, int len);
int EgSeqRamWrite(EgCardStruct *pParm, int ram, unsigned short address, MrfEvgSeqStruct *pSeq);
int EgDataBufferMode(volatile MrfEVGRegs *pEvg, int enable);
int EgDataBufferEnable(volatile MrfEVGRegs *pEvg, int enable);
int EgDataBufferSetSize(volatile MrfEVGRegs *pEvg, unsigned short size);
void EgDataBufferLoad(volatile MrfEVGRegs *pEvg, epicsUInt32 *data, int nelm);
void EgDataBufferUpdate(volatile MrfEVGRegs *pEvg, epicsUInt32 *data, int nelm);
void EgDataBufferSend(volatile MrfEVGRegs *pEvg);
int EgDataBufferInit(int Card, int nelm);
EgCardStruct *EgGetCardStruct (int Card);

#endif

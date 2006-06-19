/***************************************************************************************************
|* mrfCommon.h -- Micro-Research Finland (MRF) Event System Series 200 Common Defintions
|*
|*--------------------------------------------------------------------------------------------------
|* Author:   Dayle Kotturi (SLAC)
|* Date:     30 March 2006
|*
|*--------------------------------------------------------------------------------------------------
|* MODIFICATION HISTORY:
|* 30 Mar 2006  D.Kotturi	Original
|* 08 Jun 2006  E.Bjorklund     Merged in common definitions from the old mrf200.h
|*
|*--------------------------------------------------------------------------------------------------
|* MODULE DESCRIPTION:
|*
|* This header file contains various constants and defintions used by the MRF Series 200 event
|* system. The definitions in this file are used by both driver and device support modules, as
|* well as user code that calls the device support interface.
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

#ifndef MRF_COMMON_H
#define MRF_COMMON_H

/**************************************************************************************************/
/*  Other Header Files Required By This File                                                      */
/**************************************************************************************************/

#include <debugPrint.h>                        /* SLAC Debug print utility                        */

/**************************************************************************************************/
/*  MRF Event System Constants                                                                    */
/**************************************************************************************************/

#define MRF_NUM_EVENTS              256        /* Number of possible events                       */
#define MRF_MAX_DATA_BUFFER        2048        /* Maximum size of the distributed data buffer     */

#define MRF_SN_BYTES                  6        /* Number of bytes in serial number                */
#define MRF_SN_STRING_SIZE           18        /* Size of serial number string (including NULL)   */

/*---------------------
 * Globally reserved event numbers
 */
#define EVENT_NULL                 0x00        /* NULL event                                      */

#define EVENT_SECONDS_0            0x70        /* Shift a 0 bit into the timestamp seconds reg    */
#define EVENT_SECONDS_1            0x71        /* Shift a 1 bit into the timestamp seconds reg    */
#define EVENT_HEARTBEAT            0x7a        /* Heartbeat event                                 */
#define EVENT_RESET_PRESCALERS     0x7b        /* Reset the event receiver pre-scalers            */
#define EVENT_TICK                 0x7c        /* Add 1 to the event tick counter                 */
#define EVENT_RESET_TICK           0x7d        /* Reset the event tick counter                    */
#define EVENT_END_SEQUENCE         0x7f        /* Event sequence end                              */

#define EVENT_DELAYED_IRQ    MRF_NUM_EVENTS    /* Dummy event code for delayed interrupt event    */


/**************************************************************************************************/
/*  Define the Micrel SY87739L Fractional-N Synthesizer Configuration Bit Patterns for            */
/*  Some Commonly Used Event Clock Rates.                                                         */
/**************************************************************************************************/

#define CLOCK_124950_MHZ        0x00FE816D      /* 124.950 MHz.                                   */
#define CLOCK_124907_MHZ        0x0C928166      /* 124.907 MHz.                                   */
#define CLOCK_119000_MHZ        0x018741AD      /* 119.000 MHz.                                   */
#define CLOCK_106250_MHZ        0x049E81AD      /* 106.250 MHz.                                   */
#define CLOCK_100625_MHZ        0x02EE41AD      /* 100.625 MHz.                                   */
#define CLOCK_099956_MHZ        0x025B41ED      /*  99.956 MHz.                                   */
#define CLOCK_080500_MHZ        0x0286822D      /*  80.500 Mhz.                                   */
#define CLOCK_050000_MHZ        0x009743AD      /*  50.000 MHz.                                   */
#define CLOCK_049978_MHZ        0x025B43AD      /*  47.978 MHz.                                   */

/**************************************************************************************************/
/*  Special Macros to Document Global Vs Local Routines and Data                                  */
/*  (note that these values can be overridden in the invoking module)                             */
/**************************************************************************************************/

/*---------------------
 * Globally accessible routines
 */
#ifndef GLOBAL_RTN
#define GLOBAL_RTN
#endif

/*---------------------
 * Routines that are normally only locally accessible
 */
#ifndef LOCAL_RTN
#define LOCAL_RTN static
#endif

/*---------------------
 * Data that is normally only locally accessible
 */
#ifndef LOCAL
#define LOCAL static
#endif

/**************************************************************************************************/
/*  Special Macros to Define Symbolic Success/Error Return Codes                                  */
/*  (note that these values can be overridden in the invoking module)                             */
/**************************************************************************************************/

#ifndef OK
#define OK     (0)
#endif

#ifndef ERROR
#define ERROR  (-1)
#endif

#ifndef NULL
#define NULL   ('\0')
#endif

#endif

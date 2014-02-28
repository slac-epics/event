/*************************************************************************\
* Copyright (c) 2002 The University of Chicago, as Operator of Argonne
*     National Laboratory.
* Copyright (c) 2002 The Regents of the University of California, as
*     Operator of Los Alamos National Laboratory.
* EPICS BASE Versions 3.13.7
* and higher are distributed subject to a Software License Agreement found
* in file LICENSE that is included with this distribution. 
\*************************************************************************/

#ifndef EVR_IRQ_HANDLER_THREAD_H
#define EVR_IRQ_HANDLER_THREAD_H

#include "erapi.h"

extern void EvrIrqHandlerThreadCreate(	void 					(	**	handler ) (int)	);
extern void EvrIrqAssignEpicsHandler(	volatile struct MrfErRegs	*	pEr,
										int								fd,
										void					(	*	handler )(int)	);

#endif	/* EVR_IRQ_HANDLER_THREAD_H	*/

#include <sys/types.h>
#include <unistd.h>
#include <fcntl.h>
#include <signal.h>
#include <epicsThread.h>
#include "erapi.h"
#include "evrIrqHandlerThread.h"

static int evrIrqHandlerThread(void *handler)
{
    void (**irqHandler)(int) = handler;
    int signal;
    sigset_t  sigSet;

    sigemptyset(&sigSet);      
    sigaddset(&sigSet,SIGIO);  

    while(1) {
        sigwait(&sigSet, &signal); 
        if (*irqHandler) (*irqHandler)(signal);
    }

    return 0;
}

void EvrIrqHandlerThreadCreate(void (**handler) (int))
{
    epicsThreadMustCreate("evrIrqHandler", epicsThreadPriorityHigh+9,
                          epicsThreadGetStackSize(epicsThreadStackMedium),
                          (EPICSTHREADFUNC)evrIrqHandlerThread,handler);
}

void EvrIrqAssignEpicsHandler(volatile struct MrfErRegs *pEr, int fd,
			 void (*handler)(int))
{
  int oflags;
  static int have_thread = 0;
  static void (*h)(int) = NULL;

  /*
   * The New Regime: We create a separate handler that waits for the signal.
   */
  h = handler;
  if (!have_thread)
      EvrIrqHandlerThreadCreate(&h);

  fcntl(fd, F_SETOWN, getpid());
  oflags = fcntl(fd, F_GETFL);
  fcntl(fd, F_SETFL, oflags | FASYNC);

  /* Now enable handler */
  EvrIrqHandled(fd);
}


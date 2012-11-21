#include <signal.h>
#include <epicsThread.h>

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

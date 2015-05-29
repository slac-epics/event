#include <signal.h>
#include <unistd.h>
#include <epicsThread.h>
#include <errlog.h>
#include <poll.h>
#include <errno.h>
#include <string.h>

static int evrIrqHandlerThread(void *handler)
{
    void (*irqHandler)(int) = handler;
    int signal;
    sigset_t  sigSet;

    sigemptyset(&sigSet);
    sigaddset(&sigSet,SIGIO);

    while(1) {
        sigwait(&sigSet, &signal); 
        if(irqHandler) irqHandler(signal);
    }

    return 0;
}

void EvrIrqHandlerThreadCreate(void (*handler) (int))
{
    epicsThreadMustCreate("evrIrqHandler", epicsThreadPriorityHigh+5,
                          epicsThreadGetStackSize(epicsThreadStackMedium),
                          (EPICSTHREADFUNC)evrIrqHandlerThread,handler);
}

#define TIMEOUT_MSEC 200

static struct EvrIrqFdStruct {
    int fd;
    void *handler;
} evrIrqFdStruct;

static int evrIrqFdHandlerThread(void *eiFdStruct) {
    struct EvrIrqFdStruct *eiFdS = eiFdStruct;
    int val, retP, retR;
    struct pollfd fds[1];
    fds[0].fd = eiFdS->fd;
    fds[0].events = POLLIN;
    void (*irqHandler)(int) = eiFdS->handler;

    while (1) {
        retP = poll(fds, 1, TIMEOUT_MSEC);
        if (retP > 0) {
            retR = read(eiFdS->fd, &val, 4);
            if (retR != 4) {
                errlogPrintf("Read failed, return value: %d\n", retR);
                return -1;
            }
            if (irqHandler) {
                irqHandler(val);
            }
        } else if (retP < 0) {
            errlogPrintf("Poll failed: %s\n", strerror(errno));
            return -1;
        }
    }
    return 0;
}

void EvrIrqFdHandlerThreadCreate(int fd, void (*handler)(int)) {
    evrIrqFdStruct.fd = fd;
    evrIrqFdStruct.handler = handler;

    epicsThreadMustCreate("evrIrqFdHandler", epicsThreadPriorityHigh + 5,
            epicsThreadGetStackSize(epicsThreadStackMedium),
            (EPICSTHREADFUNC) evrIrqFdHandlerThread, &evrIrqFdStruct);
}

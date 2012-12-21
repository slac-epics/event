#include <sys/types.h>
#include <unistd.h>
#include <fcntl.h>
#include <signal.h>
#include <stdio.h>

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

static	unsigned int	fBlockedSIGIO	= 0;

int evrIrqHandlerInit( void )
{
	int			status;
	sigset_t	sigs;

	/* No one should get SIGIO except the IRQ thread. */
	sigemptyset( &sigs );
	sigaddset( &sigs, SIGIO );
	status = pthread_sigmask( SIG_BLOCK, &sigs, NULL );
	if ( status == 0 )
		fBlockedSIGIO	= 1;
	else
		perror( "evrIrqHandlerInit: Unable to block SIGIO signals" );
	return status;
}

void EvrIrqHandlerThreadCreate(void (**handler) (int))
{
	if ( !fBlockedSIGIO )
	{
		fprintf( stderr,
				"\n\n"
				"ERROR in EvrIrqHandlerThreadCreate: evrIrqHandlerInit() either failed or\n"
				"was not called in the process main() routine.\n"
				"EVR will NOT function until this is fixed!\n\n" );
		return;
	}

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


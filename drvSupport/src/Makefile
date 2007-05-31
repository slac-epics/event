TOP=../..

include $(TOP)/configure/CONFIG

#=============================
# build a library

# for gdb/ddd debugger:
USR_CFLAGS_solaris-sparc-gnu += -g
USR_CFLAGS_linux-x86 += -g
# for debug print statements and slc-awareness
USR_CFLAGS += -DDEBUG_PRINT
#USR_CFLAGS_RTEMS += -DPNET_TIMING

SRC_DIRS += $(TOP)/mrfApp/src

INC+=evrMessage.h

DBD += devWFevrMessage.dbd
DBD += drvPnet.dbd
DBD += drvEvr.dbd

LIBRARY_IOC = drvSupport

drvSupport_SRCS += devWFevrMessage.c
drvSupport_SRCS += drvPnet.c
drvSupport_SRCS += drvEvr.c
drvSupport_SRCS += evrMessage.c
drvSupport_LIBS += $(EPICS_BASE_IOC_LIBS)

#===========================

include $(TOP)/configure/RULES

#----------------------------------------
#  ADD RULES AFTER THIS LINE


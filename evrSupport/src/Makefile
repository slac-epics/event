TOP=../..

include $(TOP)/configure/CONFIG

#=============================
# build a library

# for gdb/ddd debugger:
USR_CFLAGS_solaris-sparc-gnu += -g
USR_CFLAGS_linux-x86 += -g
# for debug print statements and slc-awareness
USR_CFLAGS += -DDEBUG_PRINT
# Use EVR driver routines
USR_CFLAGS_RTEMS += -DEVR_DRIVER_SUPPORT
ifeq ($(EVR_DRIVER_SUPPORT_LINUX), YES)
USR_CFLAGS_Linux += -DEVR_DRIVER_SUPPORT
endif

SRC_DIRS += $(TOP)/mrfApp/src

INC+=evrTime.h
INC+=eventPattern.h
INC+=evrEventMessage.h

DBDINC += longSubRecord
DBDINC += bsaRecord

DBD += eventSupport.dbd
DBD += evrEvInvariantDelay.dbd
DBD += evrEvDesc.dbd
DBD += eventPattern.dbd
DBD += evrModifier5.dbd
DBD += evrTime.dbd
DBD += bsa.dbd
DBD += drvEvr.dbd

LIBRARY_IOC = eventSupport
ifeq ($(HAVE_BSA_CORE),YES)
LIBRARY_IOC += eventSupportLegacy
LIBRARY_IOC += eventBsaEventSupport
endif

EVR_SUP_COM_SRCS += evrEvInvariantDelay.c
EVR_SUP_COM_SRCS += evrEvDesc.c
EVR_SUP_COM_SRCS += eventPattern.c
EVR_SUP_COM_SRCS += evrModifier5.c
EVR_SUP_COM_SRCS += evrTime.c
EVR_SUP_COM_SRCS += wrap_bsaCb.cc
EVR_SUP_COM_SRCS += evrEventMessage.c
EVR_SUP_COM_SRCS += longSubRecord.c
EVR_SUP_COM_SRCS += bsaRecord.c
EVR_SUP_COM_SRCS += drvEvr.c


eventSupport_SRCS       += $(EVR_SUP_COM_SRCS)

ifeq ($(HAVE_BSA_CORE),YES)
eventSupportLegacy_SRCS += $(EVR_SUP_COM_SRCS)
eventSupportLegacy_SRCS += bsa.c
eventSupportLegacy_LIBS += $(EPICS_BASE_IOC_LIBS)
eventSupport_SRCS       += devBsaCore.c
eventSupport_SRCS       += devAoBsaCore.c

eventBsaEventSupport_SRCS  += bsaRecord.c
eventBsaEventSupport_SRCS  += devBsaCore.c
eventBsaEventSupport_SRCS  += devAoBsaCore.c
eventBsaEventSupport_LIBS  += $(EPICS_BASE_IOC_LIBS)
else
eventSupport_SRCS       += bsa.c
endif


#eventSupportLegacy_LIBS += generalTime
eventSupport_LIBS       += $(EPICS_BASE_IOC_LIBS)

#===========================

include $(TOP)/configure/RULES

#----------------------------------------
#  ADD RULES AFTER THIS LINE


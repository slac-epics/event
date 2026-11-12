TOP=..

include $(TOP)/configure/CONFIG

#=============================
# build a library

# for gdb/ddd debugger:
USR_CFLAGS_solaris-sparc-gnu += -g
USR_CFLAGS_linux-x86 += -g
# for debug print statements and slc-awareness
USR_CFLAGS += -DDEBUG_PRINT

# Build test applications
DBD += evr.dbd
DBD += evrTest.dbd
evrTest_DBD += base.dbd
evrTest_DBD += mpsEvrProc.dbd
evrTest_DBD += eventSupport.dbd
evrTest_DBD += devMrfErEvent.dbd
evr_DBD += base.dbd
evr_DBD += mpsEvrProc.dbd
evr_DBD += eventSupport.dbd
evr_DBD += devMrfErEvent.dbd

PROD_IOC_Linux = evrTest

ifneq ($(filter $(T_A),RTEMS-beatnik RTEMS-mvme3100),)
PROD_IOC_RTEMS = evr
endif
# <name>_registerRecordDeviceDriver.cpp will be created from <name>.dbd
evrTest_SRCS += evrTest_registerRecordDeviceDriver.cpp
evrTest_SRCS += evrTestMain.cpp
evrTest_SRCS += mpsEvrProc.c
evr_SRCS += evr_registerRecordDeviceDriver.cpp
evr_SRCS += mpsEvrProc.c

evrTest_LIBS += eventSupport devMrfErEvent BsaCore
evrTest_LIBS_Linux += drvMrfEvent
evrTest_LIBS_RTEMS += mrfVme64x
evrTest_LIBS += $(EPICS_BASE_IOC_LIBS)
evr_LIBS += eventSupport devMrfErEvent BsaCore
evr_LIBS_RTEMS += mrfVme64x
evr_LIBS += $(EPICS_BASE_IOC_LIBS)

#===========================

include $(TOP)/configure/RULES

#----------------------------------------
#  ADD RULES AFTER THIS LINE


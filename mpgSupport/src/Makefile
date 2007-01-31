TOP=../..

include $(TOP)/configure/CONFIG

#=============================
# build a library

# for gdb/ddd debugger:
USR_CFLAGS_solaris-sparc-gnu += -g
USR_CFLAGS_linux-x86 += -g
# for debug print statements and slc-awareness
USR_CFLAGS += -DDEBUG_PRINT

SRC_DIRS += $(TOP)/drvSupport/src
SRC_DIRS += $(TOP)/evrSupport/src
SRC_DIRS += $(TOP)/mrfApp/src

DBD += mpgPattern.dbd
DBD += edef.dbd
DBD += mpgSupport.dbd

LIBRARY_IOC = mpgSupport

mpgSupport_SRCS += pnetSeqCheck.c
mpgSupport_SRCS += mpgPattern.c
mpgSupport_SRCS += edefReserve.st edefMask.st

mpgSupport_LIBS += evrSupport
mpgSupport_LIBS += drvSupport
mpgSupport_LIBS += sSubRecord
mpgSupport_LIBS += std
mpgSupport_LIBS += generalTime
mpgSupport_LIBS += seq pv
mpgSupport_LIBS += $(EPICS_BASE_IOC_LIBS)

# Build test applications
DBD += mpg.dbd
DBD += mpgSoft.dbd
mpgSoft_DBD += base.dbd
mpgSoft_DBD += mpgSupport.dbd
mpg_DBD += base.dbd
mpg_DBD += mpgSupport.dbd
mpg_DBD += devMrfEg.dbd

PROD_IOC = mpgSoft
PROD_IOC_RTEMS = mpg

# <name>_registerRecordDeviceDriver.cpp will be created from <name>.dbd
mpgSoft_SRCS += mpgSoft_registerRecordDeviceDriver.cpp
mpgSoft_SRCS += mpgMain.cpp
mpg_SRCS += mpg_registerRecordDeviceDriver.cpp

mpgSoft_LIBS += $(MPG_IOC_LIBS)
mpgSoft_LIBS += $(EPICS_BASE_IOC_LIBS)
mpg_LIBS += $(MPG_IOC_LIBS)
mpg_LIBS += devMrfEg mrfVme64x
mpg_LIBS += $(EPICS_BASE_IOC_LIBS)

#===========================

include $(TOP)/configure/RULES

#----------------------------------------
#  ADD RULES AFTER THIS LINE


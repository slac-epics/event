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

DBD += evrSupport.dbd
DBD += evrPattern.dbd
DBD += evrModifier5.dbd
DBD += evrTime.dbd
DBD += bsa.dbd

LIBRARY_IOC = evrSupport

evrSupport_SRCS += bsa.c
evrSupport_SRCS += evrPattern.c
evrSupport_SRCS += evrModifier5.c
evrSupport_SRCS += evrTime.c
evrSupport_LIBS += drvSupport
evrSupport_LIBS += sSubRecord
evrSupport_LIBS += generalTime
evrSupport_LIBS += $(EPICS_BASE_IOC_LIBS)

# Build test applications
DBD += evrTest.dbd
DBD += evrTestSoft.dbd
evrTestSoft_DBD += base.dbd
evrTestSoft_DBD += evrSupport.dbd
evrTest_DBD += base.dbd
evrTest_DBD += evrSupport.dbd
evrTest_DBD += devMrfEr.dbd

PROD_IOC = evrTestSoft
PROD_IOC_RTEMS = evrTest
# <name>_registerRecordDeviceDriver.cpp will be created from <name>.dbd
evrTestSoft_SRCS += evrTestSoft_registerRecordDeviceDriver.cpp
evrTestSoft_SRCS += evrTestMain.cpp
evrTest_SRCS += evrTest_registerRecordDeviceDriver.cpp

evrTestSoft_LIBS += $(EVR_IOC_LIBS)
evrTestSoft_LIBS += $(EPICS_BASE_IOC_LIBS)
evrTest_LIBS += $(EVR_IOC_LIBS) 
evrTest_LIBS += devMrfEr mrfVme64x
evrTest_LIBS += $(EPICS_BASE_IOC_LIBS)

#===========================

include $(TOP)/configure/RULES

#----------------------------------------
#  ADD RULES AFTER THIS LINE


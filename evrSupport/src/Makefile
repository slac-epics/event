TOP=../..

include $(TOP)/configure/CONFIG

#=============================
# build a library

# for gdb/ddd debugger:
USR_CFLAGS_solaris-sparc-gnu += -g
USR_CFLAGS_linux-x86 += -g
# for debug print statements and slc-awareness
USR_CFLAGS += -DDEBUG_PRINT

DBD += evrSupport.dbd
DBD += evrPattern.dbd
DBD += evrTime.dbd
DBD += bsa.dbd

LIBRARY_IOC = evrSupport

evrSupport_SRCS += bsa.c
evrSupport_SRCS += evrPattern.c
evrSupport_SRCS += evrTime.c
evrSupport_LIBS += sSubRecord
evrSupport_LIBS += generalTime
evrSupport_LIBS += $(EPICS_BASE_IOC_LIBS)

#===========================

include $(TOP)/configure/RULES

#----------------------------------------
#  ADD RULES AFTER THIS LINE


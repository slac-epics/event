#------------------------------------------------------------------------------
# $(TIMING)/mrfApp/src/Makefile
#
# Author:  Eric Bjorklund
# Date:    19 December 2005
#
#------------------------------------------------------------------------------
# This file contains the EPICS make rules and definitions for making and
# installing the driver support modules for the Micro-Research Finland
# Series 200 event receiver module.
#
#------------------------------------------------------------------------------
# Modification History:
#
# 19 Dec 2005   Bjo     Original Release
# 21 Aug 2006   Saa     Changes to build on RTEMS too.
#
#------------------------------------------------------------------------------

#---------------------
# Set up the Environment
#
TOP = ../..
SOURCE_DIR := $(patsubst %../.., %., $(TOP))

include $(TOP)/configure/CONFIG
include $(SOURCE_DIR)/MRF_CONFIG

#USR_CFLAGS += -pedantic
USR_CFLAGS += -DDEBUG_PRINT
ifeq ($(BSP_SUPPORTS_PCI), YES)
    USR_CFLAGS += -DPCI
endif

#---------------------
# Only build for "hardware" IOC's
#
#ifneq ($(findstring Host, $(VALID_BUILDS)),)
#    VALID_BUILDS = Host
#endif

#---------------------
# Build the record support modules
#
CUSTOM_RECORDS += egRecord
CUSTOM_RECORDS += egeventRecord
CUSTOM_RECORDS += erEventRecord
CUSTOM_RECORDS += ereventRecord

DBDINC += $(CUSTOM_RECORDS)

#---------------------
# Install DBD files
#
DBD += drvMrfEg.dbd
DBD += devMrfEg.dbd
DBD += devMrfEgEvent.dbd
DBD += drvMrfErEvent.dbd
DBD += devMrfErEvent.dbd
#DBD += devMrfEgLib.dbd
#DBD += devMrfErLib.dbd
devMrfEgLib_DBD += devMrfEg.dbd
devMrfEgLib_DBD += devMrfEgEvent.dbd
devMrfErLib_DBD += base.dbd
devMrfErLib_DBD += devMrfErEvent.dbd

#---------------------
# Build the Event Generator Library
#
PROD_IOC_vxWorks += devMrfEgLib
LIBRARY_IOC_RTEMS += devMrfEg
LIBRARY_IOC_vxWorks += devMrfEg
ifeq ($(EVR_DRIVER_SUPPORT_LINUX), YES)
LIBRARY_IOC_Linux += devMrfEg
devMrfEg_OBJS_Linux += drvLinuxEvg
endif

devMrfEg_OBJS_RTEMS += drvMrfEg
devMrfEg_OBJS_vxWorks += drvMrfEg
devMrfEg_OBJS += devMrfEg
devMrfEg_OBJS += egRecord
devMrfEg_OBJS += egeventRecord


devMrfEgLib_OBJS += $(devMrfEg_OBJS)
devMrfEgLib_SRCS += devMrfEgLib_registerRecordDeviceDriver.cpp

OBJS_IOC_vxWorks += $(devMrfEg_OBJS)

#---------------------
# Build the Event Receiver Library
#
PROD_IOC_vxWorks += devMrfErLib
LIBRARY_IOC_RTEMS += devMrfErEvent
LIBRARY_IOC_vxWorks += devMrfErEvent
ifeq ($(EVR_DRIVER_SUPPORT_LINUX), YES)
LIBRARY_IOC_Linux += devMrfErEvent
devMrfErEvent_OBJS_Linux += drvLinuxEvr
endif

devMrfErEvent_OBJS_RTEMS += drvMrfErEvent
devMrfErEvent_OBJS_vxWorks += drvMrfErEvent
devMrfErEvent_OBJS += devMrfErEvent
ifeq ($(BSP_SUPPORTS_PCI), YES)
devMrfErEvent_OBJS_RTEMS += devLibPMC
devMrfErEvent_OBJS_RTEMS += plx9080_eeprom.c
devMrfErEvent_OBJS_vxWorks += devLibPMC
devMrfErEvent_OBJS_vxWorks += plx9080_eeprom.c
devMrfErEvent_OBJS_RTEMS += evrEEPROMFixup
endif
devMrfErEvent_OBJS += erEventRecord
devMrfErEvent_OBJS += ereventRecord

devMrfErLib_OBJS += $(devMrfErEvent_OBJS)
devMrfErLib_SRCS += devMrfErLib_registerRecordDeviceDriver.cpp

OBJS_IOC_vxWorks += $(devMrfErEvent_OBJS)

#---------------------
# Build the Library for Accessing VME-64X CR/CSR space
#
PROD_IOC_vxWorks  += mrfVme64xLib
LIBRARY_IOC_RTEMS += mrfVme64x
LIBRARY_IOC_vxWorks += mrfVme64x
ifeq ($(EVR_DRIVER_SUPPORT_LINUX), YES)
LIBRARY_IOC_Linux += drvMrfEvent
drvMrfEvent_OBJS_Linux += drvLinux
drvMrfEvent_OBJS_Linux += evrIrqHandlerThread
drvMrfEvent_OBJS_Linux += erapi.o
endif

mrfVme64x_OBJS_RTEMS += mrfVme64x
mrfVme64x_OBJS_vxWorks += mrfVme64x
mrfVme64x_OBJS_RTEMS += $(CRCSR_PROBE_NAME)
mrfVme64x_OBJS_vxWorks += $(CRCSR_PROBE_NAME)

mrfVme64xLib_OBJS += $(mrfVme64x_OBJS)

OBJS_IOC_vxWorks  += $(mrfVme64x_OBJS)

#---------------------
# Testing
#
#DBD += devMrf.dbd
#devMrf_DBD += devMrfEg.dbd
#devMrf_DBD += devMrfErEvent.dbd
#
#PROD_IOC  += devMrf
#
#devMrf_SRCS += devMrf_registerRecordDeviceDriver.cpp
#
#devMrf_LIBS += devMrfEg devMrfErEvent mrfVme64x
#devMrf_LIBS += $(EPICS_BASE_IOC_LIBS)

#---------------------
# Install include files
#
INC += mrfCommon.h
INC += mrfVme64x.h

INC += drvMrfEg.h
INC += drvMrfErEvent.h
INC += devMrfErEvent.h

INC += erapi.h

INC += debugPrint.h


#=======================================================================#
#                               RULES                                   #
#=======================================================================#

#---------------------
# Generic EPICS build rules
#
include $(TOP)/configure/RULES

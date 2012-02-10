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

# Don't use -O0 for RTEMS as it generates undef references to in_be16()
#USR_CFLAGS_Linux += -O0
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
CUSTOM_RECORDS += erRecord
CUSTOM_RECORDS += ereventRecord

DBDINC += $(CUSTOM_RECORDS)

#---------------------
# Install DBD files
#
DBD += drvMrfEg.dbd
DBD += devMrfEg.dbd
DBD += devMrfEgEvent.dbd
DBD += drvMrfEr.dbd
DBD += devMrfEr.dbd
#DBD += devMrfEgLib.dbd
#DBD += devMrfErLib.dbd
devMrfEgLib_DBD += devMrfEg.dbd
devMrfEgLib_DBD += devMrfEgEvent.dbd
devMrfErLib_DBD += base.dbd
devMrfErLib_DBD += devMrfEr.dbd

#---------------------
# Build the Event Generator Libraries
#
PROD_IOC_vxWorks    += devMrfEgLib
LIBRARY_IOC_vxWorks += devMrfEg
LIBRARY_IOC_RTEMS   += devMrfEg
LIBRARY_IOC_Linux   += devMrfEg

# Source code common to all platforms
devMrfEg_OBJS += devMrfEg
devMrfEg_OBJS += egRecord
devMrfEg_OBJS += egeventRecord

# RTEMS source
devMrfEg_OBJS_RTEMS += drvMrfEg

# VxWorks source
devMrfEg_OBJS_vxWorks += drvMrfEg

# Linux source
devMrfEg_OBJS_Linux += drvLinuxEvg


devMrfEgLib_OBJS += $(devMrfEg_OBJS)
devMrfEgLib_SRCS += devMrfEgLib_registerRecordDeviceDriver.cpp

OBJS_IOC_vxWorks += $(devMrfEg_OBJS)

#---------------------
# Build the Event Receiver Library
#
PROD_IOC_vxWorks    += devMrfErLib
LIBRARY_IOC_RTEMS   += devMrfEr
LIBRARY_IOC_vxWorks += devMrfEr
LIBRARY_IOC_Linux   += devMrfEr

# Source objects for Event Receiver Library
devMrfEr_OBJS       += devMrfEr
devMrfEr_OBJS       += erRecord
devMrfEr_OBJS       += ereventRecord


# RTEMS source
devMrfEr_OBJS_RTEMS   += drvMrfEr
ifeq ($(BSP_SUPPORTS_PCI), YES)
devMrfEr_OBJS_RTEMS   += devLibPMC
devMrfEr_OBJS_RTEMS   += plx9080_eeprom.c
devMrfEr_OBJS_RTEMS   += evrEEPROMFixup
endif

# VxWorks source
devMrfEr_OBJS_vxWorks += drvMrfEr
ifeq ($(BSP_SUPPORTS_PCI), YES)
devMrfEr_OBJS_vxWorks += devLibPMC
devMrfEr_OBJS_vxWorks += plx9080_eeprom.c
devMrfEr_OBJS_vxWorks += evrEEPROMFixup
endif

# Linux source
ifeq ($(EVR_DRIVER_SUPPORT_LINUX), YES)
devMrfEr_OBJS_Linux += drvLinuxEvr
endif

devMrfErLib_OBJS += $(devMrfEr_OBJS)
devMrfErLib_SRCS += devMrfErLib_registerRecordDeviceDriver.cpp

OBJS_IOC_vxWorks += $(devMrfEr_OBJS)

#---------------------
# Build the vxWorks Library for Accessing VME-64X CR/CSR space
#
PROD_IOC_vxWorks       += mrfVme64xLib
LIBRARY_IOC_vxWorks    += mrfVme64x
mrfVme64x_OBJS_vxWorks += mrfVme64x
mrfVme64x_OBJS_vxWorks += $(CRCSR_PROBE_NAME)
OBJS_IOC_vxWorks       += $(drvMrf_OBJS)

#---------------------
# Build the RTEMS Library for Accessing VME-64X CR/CSR space
#
LIBRARY_IOC_RTEMS      += mrfVme64x
mrfVme64x_OBJS_RTEMS   += mrfVme64x
mrfVme64x_OBJS_RTEMS   += $(CRCSR_PROBE_NAME)

mrfVme64xLib_OBJS      += $(mrfVme64x_OBJS)

#---------------------
# Build the Library for the linux driver
#
LIBRARY_IOC_Linux    += drvMrf
drvMrf_OBJS_Linux    += drvLinux
drvMrfLib_OBJS       += $(drvMrf_OBJS)
drvMrfLib_LIBS_Linux += mrfApi

#---------------------
# Testing
#
#DBD += devMrf.dbd
#devMrf_DBD += devMrfEg.dbd
#devMrf_DBD += devMrfEr.dbd
#
#PROD_IOC  += devMrf
#
#devMrf_SRCS += devMrf_registerRecordDeviceDriver.cpp
#
#devMrf_LIBS += devMrfEg devMrfEr mrfVme64x
#devMrf_LIBS += $(EPICS_BASE_IOC_LIBS)

#---------------------
# Install include files
#
INC += mrfCommon.h
INC += mrfVme64x.h

INC += drvMrfEg.h
INC += drvMrfEr.h
INC += devMrfEr.h

INC += debugPrint.h


#=======================================================================#
#                               RULES                                   #
#=======================================================================#

#---------------------
# Generic EPICS build rules
#
include $(TOP)/configure/RULES

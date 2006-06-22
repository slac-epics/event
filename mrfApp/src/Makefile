#------------------------------------------------------------------------------
# $(TIMING)/timingApp/src/Makefile
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

#---------------------
# Only build for "hardware" IOC's
#
ifneq ($(findstring Host, $(VALID_BUILDS)),)
    VALID_BUILDS = Host
endif

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
DBD += drvMrfEr.dbd
DBD += devMrfEr.dbd

#---------------------
# Build the Event Generator Library
#
PROD_IOC      += devMrfEgLib
LIBRARY_IOC   += devMrfEg

devMrfEg_OBJS += drvMrfEg
devMrfEg_OBJS += devMrfEg
devMrfEg_OBJS += egRecord
devMrfEg_OBJS += egeventRecord

devMrfEgLib_OBJS += $(devMrfEg_OBJS)
devMrfEgLib_SRCS += devMrfEg_registerRecordDeviceDriver.cpp

OBJS_IOC += $(devMrfEg_OBJS)

#---------------------
# Build the Event Receiver Library
#
PROD_IOC      += devMrfErLib
LIBRARY_IOC   += devMrfEr

devMrfEr_OBJS += drvMrfEr
devMrfEr_OBJS += devMrfEr
devMrfEr_OBJS += erRecord
devMrfEr_OBJS += ereventRecord

devMrfErLib_OBJS += $(devMrfEr_OBJS)
devMrfErLib_SRCS += devMrfEr_registerRecordDeviceDriver.cpp

OBJS_IOC += $(devMrfEr_OBJS)

#---------------------
# Build the Library for Accessing VME-64X CR/CSR space
#
PROD_IOC       += mrfVme64xLib
LIBRARY_IOC    += mrfVme64x

mrfVme64x_OBJS += mrfVme64x
mrfVme64x_OBJS += $(CRCSR_PROBE_NAME)

mrfVme64xLib_OBJS += $(mrfVme64x_OBJS)

OBJS_IOC += $(mrfVme64x_OBJS)

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

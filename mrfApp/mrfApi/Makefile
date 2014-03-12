#------------------------------------------------------------------------------
# $(TIMING)/mrfApp/mrfApi/Makefile
#
# Author:  Bruce Hill
# Date:    Mar 15, 2010
#------------------------------------------------------------------------------
# EPICS make rules for Micro-Research Finland API library and diagnostic tools
#------------------------------------------------------------------------------

#---------------------
# Set up the Environment
#
TOP = ../..
SOURCE_DIR := $(patsubst %../.., %., $(TOP))

include $(TOP)/configure/CONFIG

USR_CFLAGS += -g
# Don't use -O0 for RTEMS as it generates undef references to in_be16()
#USR_CFLAGS_Linux += -O0
USR_CFLAGS += -fPIC
USR_CFLAGS += -std=gnu99

#---------------------
# Build the MRF APplication Interface library
#
LIBRARY_IOC_Linux	+= mrfApi

# API lib source
mrfApi_SRCS			+=	egapi.c
mrfApi_SRCS			+=	erapi.c

#---------------------
# Got tired of seeing all the build warnings on these
# Define BUILD_MRF_TOOLS if you want to try any of these
#BUILD_MRF_TOOLS	= 1
ifdef BUILD_MRF_TOOLS
# Build the MRF Diagnostic tools
PROD_IOC_Linux		+= evan_monitor
PROD_IOC_Linux		+= evrpresc
PROD_IOC_Linux		+= mmap_test
PROD_IOC_Linux		+= irqtest
PROD_IOC_Linux		+= irq_monitor
PROD_IOC_Linux		+= factest
PROD_IOC_Linux		+= apitest
PROD_IOC_Linux		+= albaseq
PROD_IOC_Linux		+= dbusclk
PROD_IOC_Linux		+= timestampgen
PROD_IOC_Linux		+= simple
PROD_IOC_Linux		+= MRMTest
PROD_IOC_Linux		+= propdel
PROD_IOC_Linux		+= tgtest
PROD_IOC_Linux		+= tgrfgen
PROD_IOC_Linux		+= ELI
PROD_IOC_Linux		+= ELIHytec
PROD_IOC_Linux		+= freqset
PROD_IOC_Linux		+= XL_flash
PROD_IOC_Linux		+= pcieout
endif

# ? Monitor 
evan_monitor_SRCS	+= evan_monitor.c
evan_monitor_SRCS	+= egapi.c

# EVR Prescaler enable
evrpresc_SRCS		+= evrpresc.c
evrpresc_SRCS		+= erapi.c

# Memory map test
mmap_test_SRCS		+= mmap_test.c
mmap_test_SRCS		+= erapi.c
#mmap_test_SRCS		+= egapi.c

# IRQ test
irqtest_SRCS		+= irqtest.c
irqtest_SRCS		+= erapi.c
irqtest_LIBS		+= mrfApi
#irqtest_SRCS		+= egapi.c
#irqtest_SRCS_Linux  += evrIrqHandlerThread.c

# IRQ monitor
irq_monitor_SRCS	+= irq_monitor.c
irq_monitor_LIBS	+= mrfApi

# Factory Test
factest_SRCS		+= factest.c
factest_LIBS		+= mrfApi

apitest_SRCS		= apitest.c egapi.c erapi.c
apitest_LIBS		+= mrfApi

albaseq_SRCS		= albaseq.c egapi.c erapi.c
albaseq_LIBS		+= mrfApi

dbusclk_SRCS		= dbusclk.c egapi.c
dbusclk_LIBS		+= mrfApi

timestampgen_SRCS	= timestampgen.c egapi.c erapi.c
timestampgen_LIBS	+= mrfApi

simple_SRCS			= simple.c egapi.c erapi.c
simple_LIBS			+= mrfApi

MRMTest_SRCS		= MRMTest.c egapi.c erapi.c
MRMTest_LIBS		+= mrfApi

propdel_SRCS		= propdel.c egapi.c erapi.c
propdel_LIBS		+= mrfApi

tgtest_SRCS			= tgtest.c egapi.c erapi.c
tgtest_LIBS			+= mrfApi

tgrfgen_SRCS		= tgrfgen.c erapi.c
tgrfgen_LIBS		+= mrfApi

ELI_SRCS			= ELI.c erapi.c
ELI_LIBS			+= mrfApi

ELIHytec_SRCS		= ELIHytec.c erapi.c
ELIHytec_LIBS		+= mrfApi

freqset_SRCS		= freqset.c erapi.c
freqset_LIBS		+= mrfApi

XL_flash_SRCS		= XL_flash.c
XL_flash_LIBS		+= mrfApi

pcieout_SRCS		= pcieout.c erapi.c
pcieout_LIBS		+= mrfApi

#---------------------
# Install include files
#
INC += egcpci.h
INC += egapi.h
INC += erapi.h


#=======================================================================#
#                               RULES                                   #
#=======================================================================#

#---------------------
# Generic EPICS build rules
#
include $(TOP)/configure/RULES


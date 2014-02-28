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
# Build the MRF Diagnostic tools

# ? Monitor 
PROD_IOC_Linux		+= evan_monitor
evan_monitor_SRCS	+= evan_monitor.c
evan_monitor_SRCS	+= egapi.c

# EVR Prescaler enable
PROD_IOC_Linux		+= evrpresc
evrpresc_SRCS		+= evrpresc.c
evrpresc_SRCS		+= erapi.c

# Memory map test
PROD_IOC_Linux		+= mmap_test
mmap_test_SRCS		+= mmap_test.c
mmap_test_SRCS		+= erapi.c
#mmap_test_SRCS		+= egapi.c

# IRQ test
PROD_IOC_Linux		+= irqtest
irqtest_SRCS		+= irqtest.c
irqtest_SRCS		+= erapi.c
irqtest_LIBS		+= mrfApi
#irqtest_SRCS		+= egapi.c
#irqtest_SRCS_Linux  += evrIrqHandlerThread.c

# IRQ monitor
PROD_IOC_Linux		+= irq_monitor
irq_monitor_SRCS	+= irq_monitor.c
irq_monitor_LIBS	+= mrfApi

# Factory Test
PROD_IOC_Linux		+= factest
factest_SRCS		+= factest.c
factest_LIBS		+= mrfApi

PROD_IOC_Linux		+= apitest
apitest_SRCS		= apitest.c egapi.c erapi.c
apitest_LIBS		+= mrfApi

PROD_IOC_Linux		+= albaseq
albaseq_SRCS		= albaseq.c egapi.c erapi.c
albaseq_LIBS		+= mrfApi

PROD_IOC_Linux		+= dbusclk
dbusclk_SRCS		= dbusclk.c egapi.c
dbusclk_LIBS		+= mrfApi

PROD_IOC_Linux		+= timestampgen
timestampgen_SRCS	= timestampgen.c egapi.c erapi.c
timestampgen_LIBS	+= mrfApi

PROD_IOC_Linux		+= simple
simple_SRCS			= simple.c egapi.c erapi.c
simple_LIBS			+= mrfApi

PROD_IOC_Linux		+= MRMTest
MRMTest_SRCS		= MRMTest.c egapi.c erapi.c
MRMTest_LIBS		+= mrfApi

PROD_IOC_Linux		+= propdel
propdel_SRCS		= propdel.c egapi.c erapi.c
propdel_LIBS		+= mrfApi

PROD_IOC_Linux		+= tgtest
tgtest_SRCS			= tgtest.c egapi.c erapi.c
tgtest_LIBS			+= mrfApi

PROD_IOC_Linux		+= tgrfgen
tgrfgen_SRCS		= tgrfgen.c erapi.c
tgrfgen_LIBS		+= mrfApi

PROD_IOC_Linux		+= ELI
ELI_SRCS			= ELI.c erapi.c
ELI_LIBS			+= mrfApi

PROD_IOC_Linux		+= ELIHytec
ELIHytec_SRCS		= ELIHytec.c erapi.c
ELIHytec_LIBS		+= mrfApi

PROD_IOC_Linux		+= freqset
freqset_SRCS		= freqset.c erapi.c
freqset_LIBS		+= mrfApi

PROD_IOC_Linux		+= XL_flash
XL_flash_SRCS		= XL_flash.c
XL_flash_LIBS		+= mrfApi

PROD_IOC_Linux		+= pcieout
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


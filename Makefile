#Makefile at top of application tree
TOP = .
include $(TOP)/configure/CONFIG

DIRS += configure
DIRS += mrfApp
DIRS += evrSupport
DIRS += evrIoc
ifneq ($(filter RTEMS-beatnik,$(CROSS_COMPILER_TARGET_ARCHS)),)
DIRS += testIoc
endif

include $(TOP)/configure/RULES_TOP

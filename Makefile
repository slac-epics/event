#Makefile at top of application tree
TOP = .
include $(TOP)/configure/CONFIG

DIRS += configure
DIRS += mrfApp
DIRS += drvSupport
DIRS += evrSupport
DIRS += mpgSupport

include $(TOP)/configure/RULES_TOP

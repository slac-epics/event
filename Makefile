#Makefile at top of application tree
TOP = .
include $(TOP)/configure/CONFIG

DIRS += configure
DIRS += mrfApp
DIRS += evrSupport
#DIRS += evrIoc
#DIRS += testIoc

include $(TOP)/configure/RULES_TOP

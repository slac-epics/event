#Makefile at top of application tree
TOP = .
include $(TOP)/configure/CONFIG

DIRS += configure
DIRS += mrfApp
DIRS += evrSupport
DIRS += mpgSupport
#DIRS += dataSupport
DIRS += edefSimApp

include $(TOP)/configure/RULES_TOP

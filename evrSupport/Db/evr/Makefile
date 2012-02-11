TOP=../../..
include $(TOP)/configure/CONFIG
#----------------------------------------
#  ADD MACRO DEFINITIONS AFTER THIS LINE
USR_VPATH += ../../bsa
USR_VPATH += ../../templates

#----------------------------------------------------
#  Optimization of db files using dbst (DEFAULT: NO)
#DB_OPT = YES

#----------------------------------------------------
# Create and install (or just install)
# databases, templates, substitutions like this
#
# EVR databases
DB += evr.db
DB += evr-pmc.db
DB += evrWithDelays.db
DB += evrWithExtDelays.db
DB += evrEvent.db
DB += evrEventCtrl.db
DB += evrDevTrig.db
# evrPattern support
DB += evrPattern.db
DB += evrPatternProc.db
DB += evrEventAll.db
DB += evrPatternAll.db
DB += evrMod36.db

# Event Codes
DB += evrEventCode.template

# LCLS templates
DB += lclsEventCodes.db
DB += lclsEventCodes.substitutions

# PCDS Event Sequencer Codes
DB += eventCodeSequenceAll.db

include $(TOP)/configure/RULES
#----------------------------------------
#  ADD RULES AFTER THIS LINE


TOP=../..
include $(TOP)/configure/CONFIG
#----------------------------------------
#  ADD MACRO DEFINITIONS AFTER THIS LINE

#----------------------------------------------------
#  Optimization of db files using dbst (DEFAULT: NO)
#DB_OPT = YES

#----------------------------------------------------
# Create and install (or just install)
# databases, templates, substitutions like this

# MPG 360Hz processing
DB += mpgPatternProc.db
DB += mpgPatternSim.db
DB += mpgPatternTest.template
# Beam-synchronous acquisition event definitions
DB += bsaEdef.db
DB += bsaEdefPattern.db
DB += bsaEdefMask.db
DB += bsaEVGmask.db
DB += bsaEdef.template
DB += bsaEVGmask.template
# Test stands
DB += evgPatternIN20Test.template
DB += evgPatternXL15.template
# Production
DB += evgPatternIN20.template

include $(TOP)/configure/RULES
#----------------------------------------
#  ADD RULES AFTER THIS LINE
include $(TOP)/configure/RULES.Db



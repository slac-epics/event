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

# EVG setup
DB += evg.db
DB += evgEvent.db
# MPG 360Hz processing
DB += mpgPatternProc.db
# Beam-synchronous acquisition event definitions
DB += bsaEdef.db
DB += bsaEdefMask.db
DB += bsaEVGmask.db
DB += bsaEVGmask.template
DB += bsaEVGmaskConst.db 
DB += bsaEdef.template
DB += bsaEVGmaskUnits.template

include $(TOP)/configure/RULES
#----------------------------------------
#  ADD RULES AFTER THIS LINE
include $(TOP)/configure/RULES.Db



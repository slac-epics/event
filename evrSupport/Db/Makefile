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
#
# For all types of BSA devices
DB += bsaDevStates.db

# Simulation:
DB += bsaTestFanouts.db
DB += bsaTestInput.db
# BSA simulation - evr/evg support
DB += bsaProcTest.db 
DB += bsaProcTest.template
# Device Simulation
DB += bsaBPMSTest.template
DB += bsaTOROTest.template
DB += bsaBLENTest.template
DB += bsaKICKTest.template
DB += bsaWIRETest.template
DB += bsaPMTTest.template
# Device edef records
DB += bsaBPMSEdef.template
DB += bsaTOROEdef.template
DB += bsaBLENEdef.template
DB += bsaKICKEdef.template
DB += bsaPMTEdef.template
DB += bsaWIREEdef.template

# evrPattern support
DB += evrPattern.db
DB += evrPatternProc.db
DB += evrFiducial.db
DB += evrPattern.template
DB += evrPatternProc.template
# evrPattern sim support
DB += evrPatternSim.db
DB += evrPatternTest.template
# bsa using EDEFs
DB += bsaEdefAvg.db
DB += bsaEdefFanouts.db
DB += bsaEdefPattern.db
DB += bsaEdefPatternProc.db
DB += bsaEdefPattern.template

#----------------------------------------------------
# If <anyname>.db template is not named <anyname>*.template add
# <anyname>_TEMPLATE = <templatename>

include $(TOP)/configure/RULES



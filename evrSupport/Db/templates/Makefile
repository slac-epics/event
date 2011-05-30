TOP=../../..
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
# XTOD templates
DB_INSTALLS += basic-evr.substitutions
DB_INSTALLS += evrPmc230.template
DB_INSTALLS += evrPmcEvent.template
DB_INSTALLS += evrEventCtrl.template
DB_INSTALLS += evrEventRate.template
DB_INSTALLS += evrLabel.template
DB_INSTALLS += evrLabels.template
DB_INSTALLS += evrTaxi.template
DB_INSTALLS += evr.template
DB_INSTALLS += evrWithExtDelays.template

#----------------------------------------------------
# Create and install (or just install)
# Archive request files, templates, substitutions like this
# ARCHIVE += <file>.archive
# ARCHIVE += <file>.substitutions
# ARCHIVE += <file>.template
#
ARCHIVE += archive_evr.substitutions
ARCHIVE += archive_evr.template
ARCHIVE += archive_vme_evr.template

#----------------------------------------------------
# Create and install (or just install)
# Autosave request files, templates, substitutions like this
# REQ += <file>.req
# REQ += <file>.substitutions
# REQ += <file>.template
#
REQ += autosave_evr.substitutions
REQ += autosave_evr.template
REQ += autosave_vme_evr.template

include $(TOP)/configure/RULES
#----------------------------------------
#  ADD RULES AFTER THIS LINE


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

# XTOD templates
DB += basic-evr.substitutions
DB += evrPmc230.template
DB += evrVme230.template
DB += evrSLAC.template
DB += evrPmcEvent.template
DB += evrSlaveEvent.template
DB += evrVmeEvent.template
DB += evrEventCtrl.template
DB += evrEventRate.template
DB += evrFiducial.template
DB += evrLabel.template
DB += evrLabels.template
DB += evrTaxi.template
DB += evr.template
DB += evrWithExtDelays.template

#----------------------------------------------------
# Create and install (or just install)
# Archive request files, templates, substitutions like this
# ARCHIVE += <file>.archive
# ARCHIVE += <file>.substitutions
# ARCHIVE += <file>.template
#
ARCHIVE += archive_evr_SLAC.archive
ARCHIVE += archive_evr_SLAC.sub-arch
ARCHIVE += archive_evr_SLAC.tpl-arch
ARCHIVE += archive_pmc_evr.tpl-arch
ARCHIVE += archive_vme_evr.tpl-arch

#----------------------------------------------------
# Create and install (or just install)
# Autosave request files, templates, substitutions like this
# REQ += <file>.req
# REQ += <file>.substitutions
# REQ += <file>.template
#
REQ += autosave_evr_SLAC.req
REQ += autosave_evr_SLAC.sub-req
REQ += autosave_evr_SLAC.tpl-req
REQ += autosave_pmc_evr.tpl-req
REQ += autosave_vme_evr.tpl-req

include $(TOP)/configure/RULES
#----------------------------------------
#  ADD RULES AFTER THIS LINE


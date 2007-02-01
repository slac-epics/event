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
# bsa using EDEFs
DB += bsaEdefAvg.db
DB += bsaEdefFanouts.db
DB += bsaEdefShadow.db
DB += bsaEdefShadow.template
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
DB += bsaAMPLEdef.template
DB += bsaPHASEdef.template

# evrPattern support
DB += evrPattern.db
DB += evrPatternProc.db
DB += evrFiducial.db
DB += evrPattern.template
DB += evrPatternProc.template
# evrPattern sim support
DB += evrPatternSim.db
DB += evrPatternTest.template
# Test stands
DB += evrPatternXL05.template
DB += evrPatternXL09.template
DB += evrPatternXL16.template
DB += bsaXL01.template
DB += bsaSimXL01.template
DB += bsaXL02.template
DB += bsaSimXL02.template
DB += bsaXL03.template
DB += bsaSimXL03.template
DB += bsaXL04.template
DB += bsaSimXL04.template
DB += bsaXL05.template
DB += bsaSimXL05.template
# Laser Room
DB += IOC-LR20-LS01pattern.template
# Pattern Injector
DB += IOC-IN20-AM01pattern.template  
DB += IOC-IN20-BP01pattern.template  
DB += IOC-IN20-BP02pattern.template  
DB += IOC-IN20-IM01pattern.template  
DB += IOC-IN20-LS11pattern.template  
DB += IOC-IN20-LS12pattern.template  
DB += IOC-IN20-LS13pattern.template  
DB += IOC-IN20-LS14pattern.template  
DB += IOC-IN20-LS15pattern.template  
DB += IOC-IN20-LS16pattern.template  
DB += IOC-IN20-LS17pattern.template  
DB += IOC-IN20-LS18pattern.template  
DB += IOC-IN20-LS19pattern.template  
DB += IOC-IN20-MG01pattern.template  
DB += IOC-IN20-MP01pattern.template
DB += IOC-IN20-PM01pattern.template
DB += IOC-IN20-PM02pattern.template
DB += IOC-IN20-PM03pattern.template
DB += IOC-IN20-PM04pattern.template
DB += IOC-IN20-PM05pattern.template
DB += IOC-IN20-PM06pattern.template
DB += IOC-IN20-PM07pattern.template
DB += IOC-IN20-RF01pattern.template
DB += IOC-IN20-WS01pattern.template
# Pattern Sector 21
DB += IOC-LI21-BL01pattern.template
DB += IOC-LI21-BP01pattern.template
DB += IOC-LI21-MG01pattern.template
DB += IOC-LI21-PM01pattern.template

# BSA Injector
DB += IOC-IN20-BP01bsa.template
DB += IOC-IN20-BP02bsa.template
DB += IOC-IN20-IM01bsa.template
DB += IOC-IN20-RF01bsa.template
DB += IOC-IN20-WS01bsa.template

# BSA Sector 21
DB += IOC-LI21-BL01bsa.template
DB += IOC-LI21-BP01bsa.template
DB += IOC-LI21-WS01bsa.template

include $(TOP)/configure/RULES
#----------------------------------------
#  ADD RULES AFTER THIS LINE
include $(TOP)/configure/RULES.Db



#!/usr/bin/env python

# create_edm_event_msi.py
#
# Author:  M. Shankar, Jan 9, 2012
# Modification History
#        Aug 05, 2013, Stubbs   - Added 'EV' (event) subsystem type 
#        Oct 05, 2012, Piccoli  - Added 'FB' subsystem type
#        Sep 24, 2012, Shankar  - Fixed layout bug for Shanta; Added LLRF as a module in the IF statement for ORDINATE4. 
#        Aug 02, 2012, Andrews  - Added - 15 to ORDINATE1 return statement and a + 7 to ORDINATE2 return. These were changes made in such a way as to 
#                                 lessen the gap between the bottom rectangle and the TOD and SystemMode PVs, yet still keep enough room for the second
#                                 row of buttons. Also removed the unit from PAGE_TITLE and HEADER_TITLE, this just makes the displays look a little
#                                 cleaner.
#        Jul 23, 2012, Shankar  - Added ability to override title using EDM_TITLE for Tony
#        Jul 20, 2012, Andrews  - Changed all instances of "LASER" to "Laser" to make the word fit on footer buttons better for Laser displays, added
#                                 8 button footer file to "footerfilenames" for the 8 buttons in Laser Diagnostics display in IN20, changed "subsystemdesc"
#                                 for Lasers to reflect the title and button descriptions on the "Master Timing Laser" and "Laser Diagnostics" displays in
#                                 IN20, added Laser-AM to ORDINATE3 and 4 since Laser-AM will be part of an 8 button display, removed LLRF from ORDINATE4 
#                                 since all the related display buttons fit on one line now. 
#        Jul 17, 2012, Shankar  - We can now take multiple trigger substitution files; we build the internal structures using all of them.
#        Jul 13, 2012, Shankar  - Added PRIM and IOCUNIT to the header as well per Tony
#        Jul 13, 2012, Shankar  - Added PRIM and IOCUNIT columns per Tony
#        Jul 13, 2012, Andrews  - Changed first return statement numbers in ORDINATE1 and ORDINATE2 from 202 to 248, this accounts for the height of the
#                                 new BPMs map.  Also changed getHeaderFileName "if subsystem in ['BPM'...]" to "if subsystem in ['']" to get rid of the
#                                 TREFS related display.  
#        Jul  6, 2012, Andrews  - Added LLRF to "if subsystem in" for the definition of getORDINATE4, and changed +28 to +30 the line below that 
#        Jul  6, 2012, Shankar  - Added PRIMEVR for Tony; this is the IOC column of the first trigger record
#                               - Added ALARMREC's for Tony; for each #EVR, we add a ALAMREC(x) in the footer; max of 7 ALARMRECs. 
#        May 24, 2012, Shankar  - Added a subsystem for Ernest's development subsystem
#        Apr 10, 2012, Shankar  - Added a subsystem for Zen's experimental subsystem
#        Apr  9, 2012, Shankar  - Combined Eli's script into this script. Call Msi to generate the EDL file and place in ${EDM}/install
#        Mar 27, 2012, Shankar  - Generate real PV names for the DEVOPTx (pointing to spare triggers) per Debbie
#        Mar  9, 2012, Shankar  - Pulled out the $EDMSETUP. We pass it as a -I in the generate_evnt_edl_file.py
#        Mar  7, 2012, Shankar  - Added optional CONTROLPV1 and CONTROLPV2 per Debbie. Also added $EDMSETUP/ to the footerfilenames, again, per Debbie
#        Feb 14, 2012, Lahey    - use number of rows of navigation buttons to set sizes and ordinates & change THALES to Thales
#        Feb 13, 2012, Lahey    - support LS03, LS04, LS05 for Thales/Coherent, per Matt, so Thales has 2 rows of navigation buttons
#        Feb  8, 2012, Lahey    - add a DEV_BASE macro to the body rows. DEV_BASE = first 3 fields of DEV up to 2nd :, for use in bsa screens
#        Feb  7, 2012, Lahey    - cleanup output: ignore the database substituion file 'file evrDevTrig.db'. 
#                               - get BODY_TEXT from the Db trig.substitutions source file
#                               - set bsa_file = null for wire scanners, since it isn't supported in production
#        Feb  6, 2012, Lahey    - add BSA_SECN to track secondary for the evnt_bsa_dev.edl file
#                                 & change BSA to 'null' for subsystems with no evnt_bsa_* screens from evnt_dev_diag.edl 
#                               - change BODY_COMMENT to BODY_TEXT
#                               - change term for subsystem to SUBSYS (from PRIM) to avoid confusion
#        Feb  2, 2012, Lahey:   - Set the proper evnt_bsa_* filename in body rows (update trigger records before output of body rows) & remove from header
#                               - Use subsystem's descriptive string in the Header Title
#                               - Start code to get BODY_TEXT from a 3rd tag in the applications trig.substitutions (source) file
#        Jan 31, 2012, Lahey:   Thales laser includes Coherent Laser, per Matt
#                               Generate BODY_TEXT, add "do not edit" comment and body_comment to all header_templates
#        Jan 31, 2012, Shankar: use EVR location/unit on footer buttons
#        Jan 25, 2012, Lahey:   add support for PROF and up to 7 button footer
#        Jan 20, 2012, Lahey:   set ordinates for wirescanner with navigation buttons
#        Jan 19, 2012, Lahey:   Set page sizes for other systems
#        Jam 18, 2012, Shankar: Add macros to body for triggered devices
#        Jan 17, 2012, Lahey:   Change value of Page title
#        Jan 13, 2012, Lahey:   Use parameters in page and header titles
#        Jan 12, 2012, Shankar: Add subsystem, bsa and tref files
#        Jan 31, 2012, Shankar: Picked up LOCA+UNIT from the first record for the footer; use instead of UNIT1/UNIT2/UNIT3...
# 
# Using a *trig.substitutions file specified as an argument, this utility generates an msi file for each EVR that will be used to generate an edl file; it then generates the edl file using msi.
# Each generated msi file has 3 sections: a header, body, and footer.
# In the source trig.substitutions file, each EVR is identified by a comment with the syntax #EVR <EVR Name>. For example, #EVR EVR:IN20:RF03 identifies the sunsequent rows as belonging to the EVR EVR:IN20:RF03
# Similarly, the desired output file name is included in the source file as another comment, #FILE <Dest File Name>. For example, #FILE EVNT_IN20_LLRF3 will generate the output for the subsequent trigger rows into the file EVNT_IN20_LLRF3_msi.substitution
# ...and an optional BODY_TEXT is the 3rd tag. If the IOC engineer wants to display a comment near the top of the edl body, set a value in this tag.
# It is acceptable to include more than one EVR into a single substitution file. Please do make sure to separate these with a #EVR and a #FILE comment.
# For example, for IOC_IN20_RF01, there is one trig file IOC-IN20-RF01trig.substitutions which defines 3 EVRs. The resulting msi files are 
# evnt_in20_llrf_msi.substitution
# evnt_in20_llrf2_msi.substitution
# evnt_in20_llrf3_msi.substitution
# For Headers and Footers, there are common fields, and a variable number of additional buttons. Heights are based on # of entries in the body. Parameters like titles can be derived in the 2nd step.
# The bulk of this code is to generate the lines for the body. For each line for an EVR from the trig.substituions file, we append and derive 6 columns ORDINATE,TRIG, CHAN,  HCHAN, BSA,  BUTTONLABEL 



import sys
import os.path
import operator
import subprocess
import re

# If we need to sjip plex as part of this script add it to the /plex-2.0.0dev/src folder here and uncomment the following line
#sys.path.append(os.path.dirname(sys.argv[0]) + '/plex-2.0.0dev/src')


from plex import *


# Here's where we define the lexicon for the parser. 
# This should be reasonably complete; with the exception of spchars where we'll probably add more special chars as we discover them
letter = Range("AZaz")
digit = Range("09")
hspace = Any(" \t")
spchars = Any(".-_:()+/*")
comma = Str(",")
name = Rep1(letter | digit | spchars)
# We can have commans in the escaped strings
escapedstring = Str("\"") + Rep(letter | digit | spchars | hspace | comma) + Str("\"")
value = name | escapedstring
space = Any(" \t\n")
leftbrace = Str("{")
rightbrace = Str("}")
comment = Bol + Str("#") + Rep(AnyBut("\n")) + Str("\n")
patternstartseq = Seq(Str("pattern"), Rep(space), leftbrace)
filedbsubseq = Seq(Str("file"), Rep(space), Str("evrDevTrig.db"))

lexicon = Lexicon([
    (patternstartseq,'patternstart'),
    (value,TEXT),
    (leftbrace, 'beginlist'),
    (rightbrace,'endlist'),
    (comma, IGNORE),
    (comment,  'comment'),
    (space, IGNORE),
    (filedbsubseq, 'filedbsub')
])

# We are done defining the lexicon
# We parse the input file and generate a list of trigger records each of which is a dictionary

# This is the list of trigger records
triggerrecords = []
# The first trigger record is a special beast that defines the column names. 
# It is identified by  pattern { ....
# These column names are stored in the column names var
columnnames = []
# We also maintain a list of file names so that we can determine sequence for paging buttons
filecommentnames = []
# We also maintain a list of EVR names for the ALARMREC buttons at the bottom
EVRNames = []

# Open the file, parse it and generate the trigger records and column names array
def parseTriggerFile(fileName):
# This is the current trigger record we are working on
    currenttriggerrecord = {}
# We have three special comments that identify the EVR, the Output File, and the EDL BODY_TEXT Comment....
# These then get added as columns in the current trigger record
    currentEVRComment = ""
    currentFileComment = ""
    currentEDMTitleComment = ""
    currentBodyTextComment = ""
    currentCONTROLPV1 = "SIOC:SYS0:AL00:MODE"
    currentCONTROLPV2 = "SIOC:SYS0:AL00:TOD"
# Are we processing the pattern (column names line) now?
    inpattern = 0
# Are we processing a trigger record now?
    intriggerrecord = 0
# The current column that we are processing.
    currcolumnindex = 0

    f = open(fileName, "r")
    scanner = Scanner(lexicon, f, fileName)
    while 1:
        token = scanner.read()
        if token[0] is None:
            break
        elif token[0] == "comment":
            if token[1].startswith("#EVR "):
                currentEVRComment = token[1][5:len(token[1])-1]
                EVRNames.append(currentEVRComment)
            elif token[1].startswith("#FILE "):
                currentFileComment = token[1][6:len(token[1])-1]
                filecommentnames.append(currentFileComment)
# Reset the body text comment when we encounter a new file as these are specific to file
                currentBodyTextComment = ""
                currentEDMTitleComment = ""
            elif token[1].startswith("#BODY_TEXT "):
                currentBodyTextComment = token[1][11:len(token[1])-1]
            elif token[1].startswith("#CONTROLPV1 "):
                currentCONTROLPV1 = token[1][12:len(token[1])-1]
            elif token[1].startswith("#CONTROLPV2 "):
                currentCONTROLPV2 = token[1][12:len(token[1])-1]
            elif token[1].startswith("#EDM_TITLE "):
                currentEDMTitleComment = token[1][11:len(token[1])-1]
        elif token[0] == "patternstart":
# We are in the pattern portion of the definitions; this is where the column names are specified.
            inpattern = 1
        elif token[0] == "beginlist":
            intriggerrecord = 1
        elif token[0] == "endlist":
            if inpattern:
                inpattern = 0
            elif intriggerrecord:
                intriggerrecord = 0
                currenttriggerrecord['BODY_TEXT'] = currentBodyTextComment
                currenttriggerrecord['CONTROLPV1'] = currentCONTROLPV1
                currenttriggerrecord['CONTROLPV2'] = currentCONTROLPV2
                currenttriggerrecord['EDM_TITLE'] = currentEDMTitleComment
                if currentEVRComment:
                    currenttriggerrecord['EVRComment'] = currentEVRComment
                if currentFileComment:
                    currenttriggerrecord['FileComment'] = currentFileComment
                else:
                    raise Exception("Cannot determine the file comment. File comments are comments that start with #FILE[space] and have the file name after that. For example, #FILE abc123.txt. The file comment is used to determine the file name of the output msi file")
                triggerrecords.append(currenttriggerrecord)
                currenttriggerrecord = {}
                currcolumnindex = 0
        elif token[0] == 'filedbsub':
            pass
        else:
            if inpattern:
                columnnames.append(token[1])
            elif intriggerrecord:
                currenttriggerrecord[columnnames[currcolumnindex]] = token[1] 
                currcolumnindex += 1
            else:
                print "Ignoring token ",token
 
# For each trigger record, we add some new columns.
# The CHAN, HCHAN, TRIG and BUTTONLABEL columns are determined from mappings from the TCTL column using this dictionary.
# This comes from Sonya's "timing_display_macros" document.
# The 5th column in this dict is use to sort this for the DEVOTPX part of the header
TCTLMappings = {}
TCTLMappings['DG0E'] = ['0',  'FP',  '0', 'TTB/FP', 0]
TCTLMappings['DG1E'] = ['1',  'FP',  '1', 'TTB/FP', 1]
TCTLMappings['DG2E'] = ['2',  'FP',  '2', 'TTB/FP', 2]
TCTLMappings['DG3E'] = ['3',  'FP',  '3', 'TTB/FP', 3]
TCTLMappings['OTP0'] = ['0',  'TTB', '0', 'TTB/FP', 4]
TCTLMappings['OTP1'] = ['1',  'TTB', '1', 'TTB/FP', 5]
TCTLMappings['OTP2'] = ['2',  'TTB', '2', 'TTB/FP', 6]
TCTLMappings['OTP3'] = ['3',  'TTB', '3', 'TTB/FP', 7]
TCTLMappings['OTP4'] = ['4',  'TTB', '4', 'TTB', 8]
TCTLMappings['OTP5'] = ['5',  'TTB', '5', 'TTB', 9]
TCTLMappings['OTP6'] = ['6',  'TTB', '6', 'TTB', 10]
TCTLMappings['OTP7'] = ['7',  'TTB', '7', 'TTB', 11]
TCTLMappings['OTP8'] = ['8',  'TTB', '8', 'TTB', 12]
TCTLMappings['OTP9'] = ['9',  'TTB', '9', 'TTB', 13]
TCTLMappings['OTPA'] = ['10', 'TTB', 'A', 'TTB', 14]
TCTLMappings['OTPB'] = ['11', 'TTB', 'B', 'TTB', 15]
TCTLMappings['OTPC'] = ['12', 'TTB', 'C', 'TTB', 16]
TCTLMappings['OTPD'] = ['13', 'TTB', 'D', 'TTB', 17]


# Use Sonya's mapping to set the TRIG, CHAN, HCHAN and BUTTONLABEL
def addAdditionalColumns():
# First add these to the column names
    columnnames.append("ORDINATE");
    columnnames.append("TRIG");
    columnnames.append("CHAN");
    columnnames.append("HCHAN");
    columnnames.append("BSA");
    columnnames.append("BUTTONLABEL");
    columnnames.append("BSA_SECN");
    columnnames.append("DEV_BASE");
    columnnames.append("PRIM");
    columnnames.append("IOCUNIT");
    for trecord in triggerrecords:
        TCTLMapping = TCTLMappings[trecord["TCTL"]]
        trecord["CHAN"] = TCTLMapping[0]
        trecord["TRIG"] = TCTLMapping[1]
        trecord["HCHAN"] = TCTLMapping[2]
        trecord["BUTTONLABEL"] = TCTLMapping[3]
        trecord["BSA"] = "evnt_bsa_llrf"
        trecord["BSA_SECN"] = "TMIT"
        trecord["DEV_BASE"] = "xxxx:xxxx:xxxx"
        trecord["PRIM"] = trecord["IOC"].split(":")[0]
        trecord["IOCUNIT"] = trecord["IOC"].split(":")[2]


# We now categorize these by the EVR filename to give a dictionary of lists
# The triggerfiles is the starting point of this categorization
# triggerfiles is a dictionary with the key being the EVR file name and the value being a list of trigger records that belong to this file name
triggerfiles = {}
def categorizeByFileName():
    for trecord in triggerrecords:
        filename = trecord["FileComment"]
        if filename not in triggerfiles:
            triggerfiles[filename] = []
        triggerfiles[filename].append(trecord)

# The ORDINATE depends on the position of the trigger record in the final list.
# This starts at 168 for each file and increments by 24 for each triggerrecord
# As trigger records may be interleaved, it is best to set this after we have sorted the triggerrecords by filename
def addOrdinateColumn():
    for filename, filetriggerrecords in triggerfiles.iteritems():
        currordinate = 168
        for triggerrecord in filetriggerrecords:
            triggerrecord["ORDINATE"] = currordinate
            currordinate += 24


# The following methods implement logic that depend on subsystem
# There are alternate ways of accomplishing that, perhaps more effectively
# but, this way is a direct transalation from the source document and should be easier to map back to the source doc
def getHeaderFileName(subsystem):
    if subsystem in [ '']:
        return 'evnt_trefs_header_template.edl'
    else:
        return 'evnt_header_template.edl'

def getTREFFileName(subsystem):
    if subsystem in [ 'BPMS', 'TOROIDS', 'TBL', 'BL']:
        return 'evnt_all_beamDiag1'
    elif subsystem in [ 'Thales']:
        return 'evnt_all_laser'
    elif subsystem in ['PROF']:
        return 'evnt_all_profileMon'
    elif subsystem in ['WS']:
        return 'evnt_all_controllers'
    else:   
        return ''
def getBSAFileName(subsystem):
    if subsystem in [ 'BPMS']:
        return 'evnt_bsa_bpm'
    elif subsystem in [ 'Laser-AM', 'Laser', 'TOROIDS', 'TBL' ]:
        return 'evnt_bsa_dev'
    elif subsystem in [ 'LLRF' ]:
        return 'evnt_bsa_llrf'
    elif subsystem in ['BL']:
        return 'evnt_bsa_blen'
    elif subsystem in ['WS']:
        return 'null'
    else:
        return 'null'
def getBodyHeight(subsystem, numchannels):
    return 48 + (24*numchannels)

def getBodyTextComment(filetriggerrecords):
    return filetriggerrecords[0]['BODY_TEXT']

# return offset for the next field after the navigation rectangle. This is based on the number of rows of size-24 navigation buttons.
def getNavRectangleOffset(numpages):
    if numpages == 1:
        return 0
    elif numpages <= 4:
        return 24 + 16 + 12
    else:
        return 48 + 16 + 12

# ORDINATE1 defines Y for the TimeOfDay and SystemMode 
def getORDINATE1(subsystem, numchannels, navoffset):
    if subsystem in [ 'BPMS']:
        return 140 + 248  + getBodyHeight(subsystem, numchannels)
    else:
        return 140 + 4 + navoffset + getBodyHeight(subsystem, numchannels) - 15

# ORDINATE2 defines Y for the optional footer rectangle which is used for navigation buttons and maps
def getORDINATE2(subsystem, numchannels, navoffset):
    if subsystem in [ 'BPMS']:
        return getORDINATE1(subsystem, numchannels, navoffset) - 248
    else:
        return getORDINATE1(subsystem, numchannels, navoffset) - navoffset + 7 

# ORDINATE3 defines Y for the optional 1st row of footer buttons
def getORDINATE3(subsystem, numchannels, navoffset):
    if subsystem in [ 'LLRF','WS', 'PROF', 'Laser', 'Laser-AM', 'Thales']:
        return getORDINATE2(subsystem, numchannels, navoffset) + 8
    else:
        return ''

# ORDINATE4 defines Y for the optional 2nd row of footer buttons
def getORDINATE4(subsystem, numchannels, navoffset):
    if subsystem in ['LLRF', 'PROF' , 'Laser', 'Laser-AM', 'Thales']:
        return getORDINATE3(subsystem, numchannels, navoffset) + 30
    else:
        return ''
def getDISPHEIGHT(subsystem, numchannels, navoffset):
    if subsystem in [ 'BPMS']:
        return getORDINATE1(subsystem, numchannels, navoffset) + 34
    else:
        return getORDINATE1(subsystem, numchannels, navoffset) + 24


# Determine the footer based on the page count
# We currently cater to 1, 2, 3, 4, 5, 6, 7, 8, and 9 output files though it should be easy to enhance this for a larger number of pages
footerfilenames = {}
footerfilenames[1] = 'evnt_0btn_footer_template.edl'
footerfilenames[2] = 'evnt_1btn_footer_template.edl'
footerfilenames[3] = 'evnt_2btn_footer_template.edl'
footerfilenames[4] = 'evnt_3btn_footer_template.edl'
footerfilenames[5] = 'evnt_4btn_footer_template.edl'
footerfilenames[6] = 'evnt_5btn_footer_template.edl'
footerfilenames[7] = 'evnt_6btn_footer_template.edl'
footerfilenames[8] = 'evnt_7btn_footer_template.edl'
footerfilenames[9] = 'evnt_8btn_footer_template.edl'



def getFooterFileName(subsystem, numpages):
    if subsystem in [ 'BPMS' ]:
        return 'evnt_bpm_map_footer_template.edl'
    else: 
        return footerfilenames[numpages]
    
# End of subsystem specific functions

# Subsystem codes
subsystemdesc = {}
subsystemdesc['BP'] = ['BPMS', 'BPMs']
subsystemdesc['AM'] = ['Laser', 'Laser Diagnostics']
subsystemdesc['LS'] = ['Laser', 'Laser Diagnostics']
subsystemdesc['LSLR'] = ['Laser', 'Laser Diagnostics']
subsystemdesc['RF'] = ['LLRF', 'Low level RF']
subsystemdesc['MG'] = ['MGNT', 'Magnets']
subsystemdesc['PM'] = ['PROF', 'Profile Monitors']
subsystemdesc['TD'] = ['PROF', 'Profile Monitors']
subsystemdesc['IM'] = ['TOROIDS', 'Toroids']
subsystemdesc['BC'] = ['BCS', 'Beam Containment System']
subsystemdesc['BL'] = ['BL', 'Bunch Length']
subsystemdesc['MP'] = ['MPS', 'MPS']
subsystemdesc['MPSBI'] = ['SBI', 'SBI']
subsystemdesc['VA'] = ['VAC', 'Vacuum']
subsystemdesc['MC'] = ['WS', 'Wire Scanners']
subsystemdesc['EX'] = ['EX', 'Experimental']
subsystemdesc['CD'] = ['CD', 'Controls Department']
subsystemdesc['FB'] = ['FBCK', 'Feedback']
subsystemdesc['EV'] = ['EV', 'Event']

# To determine the subsystem, we use the first two characters of the UNIT
# UNIT LS defines both Laser Steering (LS: laser steering) and Thales/Coherent Lasers (LSLR)
def determineSubSystem(filetriggerrecords):
    triggerrecord = filetriggerrecords[0]
    unit = triggerrecord['UNIT']
    determinant = unit[0:2]
    loca = triggerrecord['LOCA']
    locadt = loca[0:2]
    subsystem = subsystemdesc[determinant]
    if subsystem[0] == 'Laser':
        if locadt == 'LR':
            subsystem = subsystemdesc['LSLR']
    return subsystem[0]

def determineSubSystemDesc(filetriggerrecords):
    triggerrecord = filetriggerrecords[0]
    if len(triggerrecord['EDM_TITLE']) > 1:
        return triggerrecord['EDM_TITLE']
    unit = triggerrecord['UNIT']
    determinant = unit[0:2]
    loca = triggerrecord['LOCA']
    locadt = loca[0:2]
    subsystem = subsystemdesc[determinant]
    if subsystem[0] == 'Laser':
        if locadt == 'LR':
            subsystem = subsystemdesc['LSLR']
    return subsystem[1]
    
# Print the column headers for a file
def printColumnHeaders(subsystem, numchannels, currfile, colnames, distinctLocaUnits, sortedotpx, currfilename, subsystemDesc, bodytextcomment, filetriggerrecords):
    numpages = len(triggerfiles)
    navoffset = getNavRectangleOffset(numpages)
#    print "navoffset = ",navoffset
    print >> currfile, "file ", getHeaderFileName(subsystem), "{"
    print >> currfile, "{"
    print >> currfile, "DISP_HEIGHT={0},".format(getDISPHEIGHT(subsystem, numchannels, navoffset))
    print >> currfile, "BODY_HEIGHT={0},".format(getBodyHeight(subsystem, numchannels))
    print >> currfile, "TREF_FILE=\"{0}\",".format(getTREFFileName(subsystem))
    print >> currfile, "PAGE_TITLE=\"$(LOCA) - Events - {0}\",".format(subsystem)
    print >> currfile, "HEADER_TITLE=\"$(LOCA) Events / Triggers - {0}\",".format(subsystemDesc)
    print >> currfile, "FPTR_DIAG_VIEW=evrDiags,"
    print >> currfile, "FPTR_GLOBTRIG=evrTriggerDiags,"
    print >> currfile, "FPTR_EVNTDIAG=evrEventDiags,"
    locaunitnum = 1
    for locaunit in distinctLocaUnits:
        locaunitnumstr = ""
        if locaunitnum != 1:
            locaunitnum += 1
            locaunitnumstr=str(locaunitnum)
        print >> currfile, "LOCA",locaunitnumstr,"=",locaunit.loca,","
        print >> currfile, "UNIT",locaunitnumstr,"=",locaunit.unit,","
    print >> currfile, "BODY_TITLE=\"{0} - EVR:$(LOCA):$(UNIT)\",".format(subsystem)
    print >> currfile, "BODY_TEXT=",bodytextcomment,","
    for otpxtuple in sortedotpx:
        print >> currfile, '{0} = "{1}",'.format(otpxtuple[0], otpxtuple[1])
    print >> currfile, "BUTTONLABEL1=\"EVR:$(LOCA):$(UNIT)...\","
    print >> currfile, "MENULABEL1=EVR Diags,"
    print >> currfile, "BUTTONLABEL2=\"$(UNIT) Triggers...\","
    print >> currfile, "MENULABEL2=Triggers,"
    print >> currfile, "BUTTONLABEL3=\"Events...\","
    print >> currfile, "MENULABEL3=Events,"
    print >> currfile, "BUTTONLABEL4=\"All TREFs...\","
    print >> currfile, "MENULABEL4=Event IOCs - $(LOCA) Beam Diags,"
    print >> currfile, "PRIMEVR=\"{0}\"".format(filetriggerrecords[0]['IOC'])
    print >> currfile, "PRIM=\"{0}\"".format(filetriggerrecords[0]['PRIM'])
    print >> currfile, "IOCUNIT=\"{0}\"".format(filetriggerrecords[0]['IOCUNIT'])
    print >> currfile, "}"
    print >> currfile, "}"

# update trigger records with the DEV_BASE, BSA filename and SECN
#    BSA file is subsystem-specific, that is the same bsa file for all channels in the file
#    DEV_BASE is the first 3 fields of DEV macro, not including the 3rd :
#    BSA_SECN: only bsa file 'evnt_bsa_dev' uses this macro. Default = TMIT. If FARC device, then CHRG. If laser steering file, SECN = X
    bsafilename = getBSAFileName(subsystem)
    for trecord in triggerrecords:
        device_base =  re.match ('\w*:\w*:\w*',trecord["DEV"])
        if device_base:
            trecord["DEV_BASE"] = device_base.group()
        else:
            print "Did not correctly parse the first 3 fields of the DEV macro"

        trecord["BSA"] = bsafilename
        
        if bsafilename != 'evnt_bsa_dev':
            trecord["BSA_SECN"] = 'null'
        else:
            if subsystem in ['Laser','Laser-AM']:
                trecord["BSA_SECN"] = 'X'
            else:
                devname = trecord['DEV']
                devprim = devname[0:4]
                if devprim == 'FARC':
                    trecord["BSA_SECN"] = 'CHRG'
    
# Our data is updated, so print the Body template file & rows for substitution
    print >> currfile, "file evnt_body_single_channel_template.edl {" 
# print the pattern for the dest
    firstcol = 1
    print >> currfile, "pattern { ",
    for colname in columnnames:
        if firstcol:
            firstcol = 0;
        else:
            print >> currfile, ",",
        print >> currfile, colname,
    for otpx in sortedotpx:
        if firstcol:
            firstcol = 0;
        else:
            print >> currfile, ",",
        print >> currfile, otpx[0],
    print >> currfile, "}"

# Print a trigger record
def printTriggerRecord(currfile, colnames, trecord, sortedotpx):
    firstcol = 1
    print >> currfile, "{",
    for colname in colnames:
        if firstcol:
            firstcol = 0;
        else:
            print >> currfile, ",",
        print >> currfile, trecord[colname],
    for otpx in sortedotpx:
        if firstcol:
            firstcol = 0;
        else:
            print >> currfile, ",",
        print >> currfile, '"{0}"'.format(otpx[1]),
    print >> currfile, "}"

# Print the column footers for a file
def printColumnFooters(subsystem, numchannels, currfile, currfilename, filetriggerrecords):
    numpages = len(triggerfiles)
    navoffset = getNavRectangleOffset(numpages)
    print >> currfile, "}"
    print >> currfile, "file ", getFooterFileName(subsystem, numpages), "{"
    print >> currfile, "{"
    print >> currfile, "ORDINATE1={0},".format(getORDINATE1(subsystem, numchannels, navoffset))
    print >> currfile, "ORDINATE2={0},".format(getORDINATE2(subsystem, numchannels, navoffset))
    print >> currfile, "ORDINATE3={0},".format(getORDINATE3(subsystem, numchannels, navoffset))
    print >> currfile, "ORDINATE4={0},".format(getORDINATE4(subsystem, numchannels, navoffset)) 
    print >> currfile, "SUBSYS={0},".format(subsystem)
# Create the paging buttons. Our intention here is to create buttons that launch the other source files from this file
# If we have only one output source file, we skip this section.
# Otherwise, we create (numpages - 1) buttons one for each file other than this file
    if numpages > 1:
        otherfiles = sorted([item for item in filecommentnames if item != currfilename])
        buttonlabelnum = 1
        for otherfile in otherfiles:
            pagenum = filecommentnames.index(otherfile) + 1
            otherfilelower = otherfile.lower()
# We pick up the LOCA and the UNIT from the first record.
            other_loca = triggerfiles[otherfile][0]['LOCA']
            other_unit = triggerfiles[otherfile][0]['UNIT']
            print >> currfile, 'BUTTONLABEL{0}="Triggers {3}:{4} ...",'.format(buttonlabelnum, pagenum, otherfilelower, other_loca, other_unit)
            print >> currfile, 'MENULABEL{0}="Triggers {3}:{4}",'.format(buttonlabelnum, pagenum, otherfilelower, other_loca, other_unit)
            print >> currfile, 'BUTTONFILE{0}="{2}",'.format(buttonlabelnum, pagenum, otherfilelower, other_loca, other_unit)
            otherEVRName = triggerfiles[otherfile][0]['EVRComment']
            print >> currfile, "ALARMREC{0}=\"{1}\"".format(buttonlabelnum, otherEVRName)
            buttonlabelnum = buttonlabelnum + 1
# We pick up the control PVs from the first record.
    print >> currfile, "CONTROLPV1=\"{0}\",".format(filetriggerrecords[0]['CONTROLPV1'])
    print >> currfile, "CONTROLPV2=\"{0}\"".format(filetriggerrecords[0]['CONTROLPV2'])
    print >> currfile, "}"
    print >> currfile, "}"

# We do a DISTICT of all the LOCA/UNIT values for a trigger file
# We only need to output the first one for now.
class LocaUnit:
    def __init__(self, loca, unit):
        self.loca = loca
        self.unit = unit
    def __cmp__(self, other):
        if(self.loca == other.loca):
            return cmp(self.unit, other.unit)
        else:
            return cmp(self.loca, other.loca)

def getDistinctLocaUnits(filetriggerrecords):
    locaunits = []
    for triggerrecord in filetriggerrecords:
        locaunit = LocaUnit(triggerrecord['LOCA'], triggerrecord['UNIT'])
        if locaunit not in locaunits:
            locaunits.append(locaunit)
    return locaunits


def spareTriggerForSystem(filetriggerrecords):
    if 'CONTROLPV1' in filetriggerrecords[0]:
        controlpv1 = filetriggerrecords[0]['CONTROLPV1']
        # controlpv1 is typically something like SIOC:SYS6:AL00:MODE
        system = controlpv1[5:9]
        print "System as determined from CONTROLPV1 ", system
        return 'TRIG:' + system + ':SPARE_TRIG:'
    return ''
        
# DEVOPTX is a dictionary with the DEVOPTx entries. 
# These are to be added to the header later
# The rule for this is to add "DEV" to the TCTL to generate the name.
# The value of said name is the value of the DEV column
# If this does not exist, we set this to TRIG:<System>:SPARE_TRIG: where System is determined from the control PV.
# We return a list of tuples sorted according to the sort column in the TCTL array
def generateDEVOPTX(filetriggerrecords):
    DEVOTPX = {}
    spareTrigger = spareTriggerForSystem(filetriggerrecords)
    for triggerrecord in filetriggerrecords:
        DEVOTPX["DEV" + triggerrecord["TCTL"]] = triggerrecord["DEV"]
    def getSortKey(tuple):
        return tuple[1][4]
    sortedtctlmappings = sorted(TCTLMappings.iteritems(), key=getSortKey)
    sortedotpx = []
    for tctltuple in sortedtctlmappings:
        tctlkey = tctltuple[0]
        optxkey = "DEV" + tctlkey
        otpxval = spareTrigger
        if optxkey in DEVOTPX:
            otpxval = DEVOTPX[optxkey]
        sortedotpx.append([optxkey, otpxval])
    return sortedotpx

# Generate the EDL file using MSI
def generateEDLFile(currEDLfilename, currmsifilename):
    templateLocation = os.environ['EDM'] + '/templates'
    print "EDM templates from " + templateLocation
    outputEDLFileName = os.environ['EDM'] + '/install/' + currEDLfilename
    print "Generating EDL file " + outputEDLFileName
    subprocess.call(['msi', '-o' + outputEDLFileName, '-S' + currmsifilename, '-I' + templateLocation], shell=False)


# Loop thru the trigger files and the print the header+triggerrecords+footer
def createEDMEvent(fileNames):
    for fileName in fileNames:
        parseTriggerFile(fileName)    
    print "channel count = ",len(triggerrecords)
    addAdditionalColumns()
    categorizeByFileName()
    addOrdinateColumn()
    for currfilename, filetriggerrecords in triggerfiles.iteritems():
        subsystem = determineSubSystem(filetriggerrecords)
        subsystemDesc = determineSubSystemDesc(filetriggerrecords)
        distinctLocaUnits = getDistinctLocaUnits(filetriggerrecords)
        sortedotpx = generateDEVOPTX(filetriggerrecords)
        numchannels = len(filetriggerrecords)
        bodytextcomment = getBodyTextComment(filetriggerrecords)
        currmsifilename = currfilename + "_msi.substitution"
        with open(currmsifilename, "w") as currfile:
            printColumnHeaders(subsystem, numchannels, currfile, columnnames, distinctLocaUnits, sortedotpx, currfilename, subsystemDesc, bodytextcomment, filetriggerrecords)
            for triggerrecord in filetriggerrecords:
                printTriggerRecord(currfile, columnnames, triggerrecord, sortedotpx)
            printColumnFooters(subsystem, numchannels, currfile, currfilename, filetriggerrecords)
        print "Done generating ", currmsifilename
        currEDLfilename = currfilename + ".edl"
        generateEDLFile(currEDLfilename, currmsifilename)

# Done.....



if __name__ == "__main__":
    if len(sys.argv) < 2:
        print "Usage: ", sys.argv[0], "<SrcFile1> <SrcFile2> ..."
        sys.exit(1)
    createEDMEvent(sys.argv[1:len(sys.argv)]) 


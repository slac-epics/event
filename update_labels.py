#! /usr/bin/env python
# Sample pyca script for python 2.7.2
# Before running;
# source /reg/g/pcds/setup/epicsenv-3.14.12.sh
# source /reg/g/pcds/setup/python27.sh
# pythonpathmunge /reg/g/pcds/pds/pyca
#

from Pv import Pv
import pyca
import sys

from options import Options

showCAErrors	= False

def isInteresting( label ):
    if (	label != ""
        and	label != "Spare"
        and	label != "--"
        and	label != "--spare--"
        and	label != "No Ctrl"
        and	label != "No Control"
        and	label != "Trigger Control"
        and	label != "unassigned"
        and label != "<Unassigned>"
        and label != "<unassigned>"	):
        return True
    return False

def update_labels( evrPvName, outputNum, write ):
    triggerName = evrPvName + ":TRIG%1d:" % ( outputNum )
    try:
        # See if this trigger exists
        tDesPv = Pv( triggerName + "TDES" )
        tDesPv.connect(0.1)
        tDesPv.get(False, 0.1)
    except Exception, msg:
        if showCAErrors:
            print >> sys.stderr, "failed: pyca exception: ", msg
        return

    if ( outputNum >= 10 ):
        fpLabelPv	= Pv( evrPvName + ":FP%1cL"				% (ord('A') + outputNum - 10) )
        fpDescPv	= Pv( evrPvName + ":FP%1cL.DESC"		% (ord('A') + outputNum - 10) )
        trigPv		= Pv( evrPvName + ":TRIG%1c:TCTL.DESC"	% (ord('A') + outputNum - 10) )
    else:
        fpLabelPv	= Pv( evrPvName + ":FP%1dL"				% (outputNum) )
        fpDescPv	= Pv( evrPvName + ":FP%1dL.DESC"		% (outputNum) )
        trigPv		= Pv( evrPvName + ":TRIG%1d:TCTL.DESC"	% (outputNum) )

    fpLabelName	= ""
    fpDescName	= ""
    trigName	= ""

    # Connect and get the values for each
    try:
        fpLabelPv.connect(0.1)
        fpLabelPv.get(False, 0.1)
        fpLabelName	= fpLabelPv.value
    except Exception, msg:
        #print msg
        pass
    try:
        fpDescPv.connect(0.1)
        fpDescPv.get(False, 0.1)
        fpDescName	= fpDescPv.value
    except Exception, msg:
        #print msg
        pass
    try:
        trigPv.connect(0.1)
        trigPv.get(False, 0.1)
        trigName	= trigPv.value
    except Exception, msg:
        # print msg
        pass

    showInteresting = False
    if showInteresting:
        if isInteresting( fpLabelName ):
            print "%s: '%-.30s'" % ( fpLabelPv.name, fpLabelName )
        if isInteresting( fpDescName ):
            print "%s: '%-.30s'" % ( fpDescPv.name,	 fpDescName )
        if isInteresting( trigName ):
            print "%s: '%-.30s'" % ( trigPv.name,	 trigName )

    # Start with the label string PV contents
    label = fpLabelName

    # See if the string description field is more interesting
    if ( fpDescName != label and isInteresting( fpDescName ) ):
        if isInteresting( label ):
            label += "|" + fpDescName
        else:
            label = fpDescName

    # See if the trigger name is more interesting
    if ( trigName != label and trigName != fpDescName and isInteresting( trigName ) ):
        if isInteresting( label ):
            label += "|" + trigName
        else:
            label = trigName

    # If label is not interesting, force it to <unassigned>
    if not isInteresting(label):
        label = "<unassigned>"

    # Set all 3 label variants to the new label
    dryRun = False
    if not write:
        dryRun = True
    if dryRun:
        if isInteresting( label ):
            print "would have Set all 3 %s Output %d label variants to: '%-.50s'" % ( evrPvName, outputNum, label )
    else:
        if isInteresting( label ):
            print "Setting all 3 %s Output %d label variants to: '%-.50s'" % ( evrPvName, outputNum, label )
        try:
            fpLabelPv.put( label )
        except Exception, msg:
            print msg
        try:
            fpDescPv.put( label )
        except Exception, msg:
            print msg
        try:
            trigPv.put( label )
        except Exception, msg:
            print msg

# ----------------------------------------------------------------------
if __name__ == "__main__":

    options = Options( ['evrPvName'], [], ['help','write'] )
    try:
        options.parse()
        if options.help is not None:
            options.usage( )
            sys.exit()
    except Exception, msg:
        options.usage( str(msg) )
        sys.exit()

    evrPvName	= options.evrPvName
    write		= options.evrPvName != None
    update_labels( evrPvName, 0, write )
    update_labels( evrPvName, 1, write )
    update_labels( evrPvName, 2, write )
    update_labels( evrPvName, 3, write )
    update_labels( evrPvName, 4, write )
    update_labels( evrPvName, 5, write )
    update_labels( evrPvName, 6, write )
    update_labels( evrPvName, 7, write )
    update_labels( evrPvName, 8, write )
    update_labels( evrPvName, 9, write )
    update_labels( evrPvName, 10, write )
    update_labels( evrPvName, 11, write )

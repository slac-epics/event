#! /usr/bin/env python
# pyca script for python 2.7.2
# Before running;
# source /reg/g/pcds/setup/epicsenv-3.14.12.sh
# source /reg/g/pcds/setup/python27.sh
# pythonpathmunge /reg/g/pcds/pds/pyca
#

from Pv import Pv
import pyca
import sys

from options import Options

def printPvNameValue( pvName ):
    try:
        pv = Pv( pvName )
        pv.connect(0.1)
        pv.get(False, 0.1)
        if isinstance( pv.value, str ):
            print "%s \"%-.30s\"" % ( pv.name, pv.value )
        else:
            print "%s %-.30s" % ( pv.name, pv.value )
    except pyca.pyexc, msg:
        # print >> sys.stderr, "failed: pyca exception: ", msg
        pass
    except pyca.caexc, msg:
        print >> sys.stderr, "failed: channel access exception: ", msg
    except Exception, msg:
        print >> sys.stderr, "failed:", msg.__class__.__name__, "exception: ", msg

def showPulseGenSettings( evrName, dgNumber ):
    try:
        if ( dgNumber >= 10 ):
            # Print the Delay Generator enable value
            printPvNameValue( evrName + ":CTRL.DG%1cE" % ( ord('A') + dgNumber - 10 ) )

            # Print Polarity
            printPvNameValue( evrName + ":CTRL.DG%1cP" % ( ord('A') + dgNumber - 10 ) )
        else:
            # Print the Delay Generator enable value
            printPvNameValue( evrName + ":CTRL.DG%1dE" % ( dgNumber ) )

            # Print Polarity
            printPvNameValue( evrName + ":CTRL.DG%1dP" % ( dgNumber ) )

        #
        # Pre-scale, delay, and width need special handling to
        # convert pre-scalar to 1
        #

        # Get the Pre-scale value for this delay generator
        if ( dgNumber >= 10 ):
            preScalePv = Pv( evrName + ":CTRL.DG%1cC" % ( ord('A') + dgNumber - 10 ) )
        else:
            preScalePv = Pv( evrName + ":CTRL.DG%1dC" % ( dgNumber ) )
        preScalePv.connect(0.1)
        preScalePv.get(False, 0.1)
        # Note we force preScale to 1
        print "%s %-.30s" % ( preScalePv.name, 1 )

        # Delay
        if ( dgNumber >= 10 ):
            delayPv = Pv( evrName + ":CTRL.DG%1cD" % ( ord('A') + dgNumber - 10 ) )
        else:
            delayPv = Pv( evrName + ":CTRL.DG%1dD" % ( dgNumber ) )
        delayPv.connect(0.1)
        delayPv.get(False, 0.1)
        print "%s %-.30s" % ( delayPv.name, delayPv.value * preScalePv.value )

        # Width
        if ( dgNumber >= 10 ):
            widthPv = Pv( evrName + ":CTRL.DG%1cW" % ( ord('A') + dgNumber - 10 ) )
        else:
            widthPv = Pv( evrName + ":CTRL.DG%1dW" % ( dgNumber ) )
        widthPv.connect(0.1)
        widthPv.get(False, 0.1)
        print "%s %-.30s" % ( widthPv.name, widthPv.value * preScalePv.value )

    except pyca.pyexc, msg:
        print >> sys.stderr, "failed: pyca exception: ", msg
    except pyca.caexc, msg:
        print >> sys.stderr, "failed: channel access exception: ", msg
    except Exception, msg:
        print >> sys.stderr, "failed:", msg.__class__.__name__, "exception: ", msg


def saveTriggerSettings( evrName, trigNumber ):
    triggerName = evrName + ":TRIG%1d:" % ( trigNumber )
    printPvNameValue( triggerName + "TEC.VAL" )
    printPvNameValue( triggerName + "TCTL.VAL" )
    printPvNameValue( triggerName + "TCTL.DESC" )
    printPvNameValue( triggerName + "TDES.VAL" )
    printPvNameValue( triggerName + "TDES.DESC" )
    printPvNameValue( triggerName + "TPOL.VAL" )
    printPvNameValue( triggerName + "TPOL.DESC" )
    printPvNameValue( triggerName + "TWID.VAL" )
    printPvNameValue( triggerName + "TWID.DESC" )

def saveEventCtrlSettings( evrName, ctrlNumber ):
    eventCtrlName = evrName + ":EVENT%1dCTRL" % ( ctrlNumber )
    printPvNameValue( eventCtrlName + ".ENM" )
    printPvNameValue( eventCtrlName + ".OUT0" )
    printPvNameValue( eventCtrlName + ".OUT1" )
    printPvNameValue( eventCtrlName + ".OUT2" )
    printPvNameValue( eventCtrlName + ".OUT3" )
    printPvNameValue( eventCtrlName + ".OUT4" )
    printPvNameValue( eventCtrlName + ".OUT5" )
    printPvNameValue( eventCtrlName + ".OUT6" )
    printPvNameValue( eventCtrlName + ".OUT7" )
    printPvNameValue( eventCtrlName + ".VME" )

# ----------------------------------------------------------------------
if __name__ == "__main__":

    options = Options( ['evrPvName'], [], [] )
    try:
        options.parse()
    except Exception, msg:
        options.usage( str(msg) )
        sys.exit()

    evrPvName = options.evrPvName
    try:
        statusPv = Pv( evrPvName + ":STATUS" )
        statusPv.connect(0.1)
        statusPv.get(False, 0.1)
    except Exception, msg:
        print "EVR not accessible: ", msg
        sys.exit()

    saveTriggerSettings( evrPvName, 0 )
    saveTriggerSettings( evrPvName, 1 )
    saveTriggerSettings( evrPvName, 2 )
    saveTriggerSettings( evrPvName, 3 )
    saveTriggerSettings( evrPvName, 4 )
    saveTriggerSettings( evrPvName, 5 )
    saveTriggerSettings( evrPvName, 6 )
    saveTriggerSettings( evrPvName, 7 )
    saveTriggerSettings( evrPvName, 8 )
    saveTriggerSettings( evrPvName, 9 )
    saveTriggerSettings( evrPvName, 10 )
    saveTriggerSettings( evrPvName, 11 )

    showPulseGenSettings( evrPvName, 0 )
    showPulseGenSettings( evrPvName, 1 )
    showPulseGenSettings( evrPvName, 2 )
    showPulseGenSettings( evrPvName, 3 )
    showPulseGenSettings( evrPvName, 4 )
    showPulseGenSettings( evrPvName, 5 )
    showPulseGenSettings( evrPvName, 6 )
    showPulseGenSettings( evrPvName, 7 )
    showPulseGenSettings( evrPvName, 8 )
    showPulseGenSettings( evrPvName, 9 )
    showPulseGenSettings( evrPvName, 10 )
    showPulseGenSettings( evrPvName, 11 )

    saveEventCtrlSettings( evrPvName, 1 )
    saveEventCtrlSettings( evrPvName, 2 )
    saveEventCtrlSettings( evrPvName, 3 )
    saveEventCtrlSettings( evrPvName, 4 )
    saveEventCtrlSettings( evrPvName, 5 )
    saveEventCtrlSettings( evrPvName, 6 )
    saveEventCtrlSettings( evrPvName, 7 )
    saveEventCtrlSettings( evrPvName, 8 )
    saveEventCtrlSettings( evrPvName, 9 )
    saveEventCtrlSettings( evrPvName, 10 )
    saveEventCtrlSettings( evrPvName, 11 )
    saveEventCtrlSettings( evrPvName, 12 )
    saveEventCtrlSettings( evrPvName, 13 )
    saveEventCtrlSettings( evrPvName, 14 )

    printPvNameValue( evrPvName + ":FP0L" )
    printPvNameValue( evrPvName + ":FP0L.DESC" )
    printPvNameValue( evrPvName + ":FP1L" )
    printPvNameValue( evrPvName + ":FP1L.DESC" )
    printPvNameValue( evrPvName + ":FP2L" )
    printPvNameValue( evrPvName + ":FP2L.DESC" )

    # Done

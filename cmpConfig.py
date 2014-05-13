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

def stringToScalar( value ):
    try:
        value = int(value)
    except:
        value = float(value)
    return value

def cmpConfigFile( fileName ):
    try:
        with open( fileName ) as f:
            for line in f:
                valStr = ''
                if len(line.split()) == 1:
                    [ pvName ] = line.split()
                elif len(line.split()) == 2:
                    [ pvName, valStr ] = line.split()
                else:
                    [ pvName, valStr, crlfStr ] = line.split('"')
                if valStr == '""':
                    valStr = ''

                pv = Pv( pvName )
                pv.connect(0.1)
                pv.get( False, 0.1 )
                if isinstance( pv.value, str ):
                    cfgValue = valStr.strip( '"' )
                else:
                    cfgValue = stringToScalar( valStr )
                # print "actual %s %-.30s, cfg %-.30s" % ( pv.name, pv.value, cfgValue )
                if cfgValue == pv.value:
                    continue
                if	(	isinstance( pv.value, float ) and
                        (abs(pv.value - cfgValue)/cfgValue) < 0.0001 ):
                    continue
                print "ERROR: actual %s %-.30s != cfg %-.30s" % ( pv.name, pv.value, cfgValue )

    except pyca.pyexc, msg:
        print "failed: pyca exception: ", msg
    except pyca.caexc, msg:
        print "failed: channel access exception: ", msg
    except Exception, msg:
        print "failed:", msg.__class__.__name__, "exception: ", msg

# ----------------------------------------------------------------------
if __name__ == "__main__":

    options = Options( ['configFile'], [], [] )
    try:
        options.parse()
    except Exception, msg:
        options.usage( str(msg) )
        sys.exit()

    cmpConfigFile( options.configFile )
    pyca.flush_io()
    # Done

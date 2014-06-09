#! /bin/bash

# Setup edm environment
source /reg/g/pcds/setup/epicsenv-3.14.12.sh

edm -x -eolc \
-m "EVR=EVR:MEC:USR01" \
-m "IOC=IOC:MEC:USR:EVR01" \
evrScreens/evrSLAC.edl &

#! /bin/bash

# Setup edm environment
source /reg/g/pcds/setup/epicsenv-3.14.12.sh

edm -x -eolc \
-m "IOC=LAS:SR63:IOC:09" \
-m "EVR=LAS:SR63:EVR:09B" \
-m "EVR1=LAS:SR63:EVR:09" \
-m "EVR2=LAS:SR63:EVR:09B" \
evrScreens/evr4.edl evrScreens/evr_twoModule.edl &

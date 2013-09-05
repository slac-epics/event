#! /bin/bash

# Setup edm environment
source /reg/g/pcds/setup/epicsenv-3.14.12.sh

edm -x -eolc -m "IOC=LAS:R52:IOC:30,EVR=LAS:R52:EVR:30,DEC=14" evrScreens/evr.edl evrScreens/lclsEventCodes.edl evrScreens/emb-event-code.edl evrScreens/emb-10-ec.edl evrScreens/emb-7-ec.edl &
#edm -x -eolc -m "IOC=UND:R02:IOC:16,EVR=UND:R02:EVR:16" evrScreens/evr.edl evrScreens/lclsEventCodes.edl evrScreens/emb-event-code.edl &


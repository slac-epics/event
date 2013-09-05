#! /bin/bash

# Setup edm environment
source /reg/g/pcds/setup/epicsenv-3.14.12.sh

edm -x -eolc -m "IOC=LAS:R52:IOC:49,EVR=LAS:R52:EVR:49" evrScreens/evr14.edl &


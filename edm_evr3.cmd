#! /bin/bash

# Setup edm environment
source /reg/g/pcds/setup/epicsenv-3.14.12.sh

edm -x -eolc			\
-m "IOC=LAS:R52:IOC:30"	\
-m "EVR=LAS:R52:EVR:30"	\
-m "DEC=9"				\
-m "N1=87,N2=171,N3=201" \
-m "EVG=EVNT:SYS0"		\
-m "ECS=ECS:SYS0"		\
-m "NAME1=LCLSBEAM"		\
-m "ID1=140"			\
-m "NAME2=LCLSBURS"		\
-m "ID2=150"			\
-m "NAME3=TS4_120_"		\
-m "ID3=40"				\
evrScreens/evr.edl		\
evrScreens/emb-evg-*-ec.edl	\
evrScreens/emb-ecs-*-ec.edl	\
&


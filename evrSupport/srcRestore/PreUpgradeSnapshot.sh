#!/bin/bash
DATE=$(date +"%Y%m%d%H%M")

echo "Use: $0 ioc-name"
echo "e.g. $0 ioc-in20-ls01"
echo "Save the timing settings from this IOC that must be restored correctly"
echo "after a reboot. Values will be saved to a time-stamped file"

templatedir=$PWD
cd $IOC_DATA/$1
configFile=$1-event.cwConfig

# copy the templates we'll need here
cp $templatedir/evrEvents.template.cwConfig .
cp $templatedir/evrEvrs.template.cwConfig .

events=$(grep -o 'EVR\:[[:alnum:]\:_]*\:EVENT[0-9]*CTRL' iocInfo/IOC.pvlist)
echo "# events"  > $1-event.cwConfig

    for event in $events;
      do
                echo "file evrEvents.template.cwConfig EVENT="$event >> $1-event.cwConfig
    done

evrs=$(grep -o 'EVR\:[[:alnum:]\:_]*\:CTRL' iocInfo/IOC.pvlist)
echo "# evrs"  >> $1-event.cwConfig

    for evr in $evrs;
      do
                echo "file evrEvrs.template.cwConfig EVR="$evr >> $1-event.cwConfig
    done

echo "# triggers"  >> $1-event.cwConfig
grep -of $templatedir/trigPatterns.grep iocInfo/IOC.pvlist | grep -v 'W_' >> $1-event.cwConfig

# Finally, use the contstructed cwConfig file to get values.
CWget $configFile evrPreUpgrade$DATE

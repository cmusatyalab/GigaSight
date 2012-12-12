#/bin/bash

DIR=/tmp

for file in $DIR/*.log
do
NAME=$(echo $file | cut -d '.' -f1)_RRDELAY.txt
echo Parsing $file to $NAME
cat $file | grep "xfer" | grep "192.168.2.7" | cut -d ":" -f4- | cut -d "," -f1,3- | tr ',' '\t' 

done


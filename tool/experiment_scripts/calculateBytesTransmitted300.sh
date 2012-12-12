#!/bin/bash
filepath=/tmptshark/randomv2
outputfile=$1"_bytes.txt"

if [ -f $outputfile ]; then
echo Removing $outputfile
rm $outputfile
fi
node=1
while [ $node -lt 9 ]
do
iter=0
while [ $iter -lt 5 ]
do
logfile=$filepath/$1"_"$node"_"$iter"_tshark.log"
echo Parsing $logfile
bytes=$(sudo tshark -r $logfile -qz io,stat,1185,"ip.src==192.168.2.7" | grep "000.000-1185.000" | cut -d ' ' -f4)
echo $logfile","$bytes >> /home/cloudlet/scripts/$1"_bytes.txt"
iter=$((iter + 1))
echo $iter
done
node=$((node + 1))
done

#/bin/bash

file=$1
outputfile=$(echo $file | cut -d '.' -f1)"_throughput.txt"

if [ -f temp.txt ]; then
rm temp.txt
fi

if [ -f $outputfile ]; then
echo Warning: deleting existing outputfile $outputfile
rm $outputfile
fi

cat $file | grep "Perf" > temp.txt

#we need to print out a value, even for missing seconds!
#also, we strip of the first zero (00 becomes 0, 01 becomes 1, etc...)
read line <  $file
currentHour=$(echo $line | cut -d ' ' -f2- | cut -d ':' -f1 | sed 's/^0//')
currentMin=$(echo $line | cut -d ' ' -f2- | cut -d ':' -f2 | sed 's/^0//')
currentSec=$(echo $line | cut -d ' ' -f2- | cut -d ':' -f3 | cut -d ',' -f1 | sed 's/^0//')
echo starttime: $currentHour:$currentMin:$currentSec
while read line
do
#parse all relevant information from this line
lineHour=$(echo $line | cut -d ' ' -f2- | cut -d ':' -f1 | sed 's/^0//')
lineMin=$(echo $line | cut -d ' ' -f2- | cut -d ':' -f2 | sed 's/^0//')
lineSec=$(echo $line | cut -d ' ' -f2- | cut -d ':' -f3 | cut -d ',' -f1 | sed 's/^0//')
lineTime=$((lineHour * 60  * 60 + lineMin * 60 + lineSec))
lineMB=$(echo $line | cut -d ':' -f4- | cut -d ' ' -f3)
#write zero for missing seconds
while [ "$lineHour:$lineMin:$lineSec" != "$currentHour:$currentMin:$currentSec" ]
do
echo -e $currentHour:$currentMin:$currentSec"\t0" >> $outputfile
#echo missing timestamp: $currentHour:$currentMin:$currentSec
currentSec=$((currentSec + 1))
currentMin=$((currentMin + (currentSec / 60)))
currentSec=$((currentSec % 60))
currentHour=$((currentHour + (currentMin / 60)))
currentMin=$((currentMin % 60))
currentHour=$((currentHour % 24))
done

#write statistics of current time to output file
echo -e $lineHour:$lineMin:$lineSec"\t"$lineMB >> $outputfile
echo -e $lineHour:$lineMin:$lineSec"\t"$lineMB
#increase current time
currentSec=$((currentSec + 1))
currentMin=$((currentMin + (currentSec / 60)))
currentSec=$((currentSec % 60))
currentHour=$((currentHour + (currentMin / 60)))
currentMin=$((currentMin % 60))
done < temp.txt

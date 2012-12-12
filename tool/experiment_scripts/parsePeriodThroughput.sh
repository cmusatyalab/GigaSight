#/bin/bash

file=$1
outputfile=$(echo $file | cut -d '.' -f1)"_throughput.txt"
avg=10

if [ -f temp.txt ]; then
rm temp.txt
fi

if [ -f $outputfile ]; then
echo Warning: deleting existing outputfile $outputfile
rm $outputfile
fi

cat $file | grep "Perf" > temp.txt

endSec=0
endMin=0
endHour=0
endTime=0
intervalMB=0
while read line
do
#parse all relevant information from this line
lineHour=$(echo $line | cut -d ' ' -f2- | cut -d ':' -f1 | sed 's/^0*//')
lineMin=$(echo $line | cut -d ' ' -f2- | cut -d ':' -f2 | sed 's/^0*//')
lineSec=$(echo $line | cut -d ' ' -f2- | cut -d ':' -f3 | cut -d ',' -f1 | sed 's/^0*//')
lineTime=$((lineHour * 60  * 60 + lineMin * 60 + lineSec))
lineMB=$(echo $line | cut -d ':' -f4- | cut -d ' ' -f3)
#do we have a new interval?
if [ $lineTime -ge $endTime ]
then
#write statistics of previous interval to output file
echo -e $endHour:$endMin:$endSec$"\t"$intervalMB >> $outputfile
echo $endHour:$endMin:$endSec 	$intervalMB
endSec=$((lineSec + avg))
endMin=$((lineMin + (endSec / 60)))
endSec=$((endSec % 60))
endHour=$((lineHour + (endMin / 60)))
endMin=$((endMin % 60))
endTime=$((endHour * 60 * 60 + endMin * 60 + endSec))
intervalMB=0
echo new interval $lineHour:$lineMin:$lineSec to $endHour:$endMin:$endSec
else
intervalMB=$(echo $intervalMB + $lineMB | bc)
fi
done < temp.txt

#/bin/bash

file=$1
outputfile=$(echo $file | cut -d '.' -f1)"_power.csv"

if [ -f $outputfile ]; then
echo Warning: deleting existing outputfile $outputfile
rm $outputfile
fi

echo Parsing $file
lineCnt=0
while read line
do

if [ $(( lineCnt%10000 )) -eq 0 ];
then
	echo Lines parsed $lineCnt of $file
fi
avgpower=$(echo $line | cut -d ',' -f3)
lineCnt=$(( lineCnt+1 ))
echo -e $avgpower >> $outputfile
done < $file

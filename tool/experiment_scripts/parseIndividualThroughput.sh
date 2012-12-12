#/bin/bash

file=$1
outputfile=$(echo $file | cut -d '.' -f1)"_individualthroughput.txt"

if [ -f temp.txt ]; then
rm temp.txt
fi

if [ -f $outputfile ]; then
echo Warning: deleting existing outputfile $outputfile
rm $outputfile
fi

cat $file | grep "192.168.2.7" > temp.txt

while read line
do
uploadTime=$(echo $line | cut -d ',' -f8- | cut -d ' ' -f2)
roundTime=$(echo $line | cut -d ',' -f8- | cut -d ' ' -f4)
echo -e $uploadTime"\t"$roundTime >> $outputfile
done < temp.txt

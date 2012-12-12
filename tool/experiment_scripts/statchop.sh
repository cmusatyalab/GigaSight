#!/bin/bash
RES=$1
SIZE=$2
DIR=$RES/$SIZE
i=0
file=$DIR/chop_$i.mp4
outputfile=statchop_${RES}_$SIZE.txt
if [ -f $outputfile ];
then
echo Warning: deleting existing file $outputfile
rm $outputfile
fi

echo Parsing chop $file
echo Generating $outputfile
while [ -f $file ]
do
	echo Parsing chop $file
	line=$(ls -al $file | awk '{ print $9,"\t"$5 }')
	echo $line >> $outputfile
	i=$((i + 1))
	file=$DIR/chop_$i.mp4
done

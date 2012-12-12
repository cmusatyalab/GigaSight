#!/bin/bash
inputFile=$1
baseOutputDirectory=$2

duration=$(ffmpeg -i $inputFile 2>&1 | grep "Duration" | cut -d ' ' -f 4 | sed s/,//)
min=$(echo $duration | cut -d ':' -f 2 | sed 's/0*//')
sec=$(echo $duration | cut -d ':' -f 3 | cut -d '.' -f 1 | sed 's/0*//')
echo $min:$sec
totalSec=$((min*60 + sec))
echo $totalSec

chopSec=5
outputDirectory=$baseOutputDirectory/${chopSec}s
sec=0
while [ $sec -lt $totalSec ]
do
startmin=$((sec / 60))
startsec=$((sec % 60))
echo Generating chop started at $startmin:$startsec
ffmpeg -loglevel quiet -i $inputFile -ss $sec -t $chopSec -acodec copy -vcodec copy $outputDirectory/chop_$((sec / chopSec)).mp4 2>&1
sec=$((sec + chopSec))
done

chopSec=30
outputDirectory=$baseOutputDirectory/${chopSec}s
sec=0
while [ $sec -lt $totalSec ]
do
startmin=$((sec / 60))
startsec=$((sec % 60))
echo Generating chop started at $startmin:$startsec
ffmpeg -loglevel quiet -i $inputFile -ss $sec -t $chopSec -acodec copy -vcodec copy $outputDirectory/chop_$((sec / chopSec)).mp4 2>&1
sec=$((sec + chopSec))
done

chopSec=300
outputDirectory=$baseOutputDirectory/${chopSec}s
sec=0
while [ $sec -lt $totalSec ]
do
startmin=$((sec / 60))
startsec=$((sec % 60))
echo Generating chop started at $startmin:$startsec
ffmpeg -loglevel quiet -i $inputFile -ss $sec -t $chopSec -acodec copy -vcodec copy $outputDirectory/chop_$((sec / chopSec)).mp4 2>&1
sec=$((sec + chopSec))
done

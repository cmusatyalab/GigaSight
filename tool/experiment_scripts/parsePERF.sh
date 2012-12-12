#/bin/bash

DIR=/tmp

for file in $DIR/360p*.txt
do

NAME=$(echo $file | cut -d '.' -f1)_PERF.txt
cat $file | grep "Perf" > $NAME

done

for file in $DIR/480p*.txt
do

NAME=$(echo $file | cut -d '.' -f1)_PERF.txt
cat $file | grep "Perf" > $NAME

done

for file in $DIR/1080p*.txt
do

NAME=$(echo $file | cut -d '.' -f1)_PERF.txt
cat $file | grep "Perf" > $NAME

done

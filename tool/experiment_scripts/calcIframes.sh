#!/bin/bash

inputDirectory=$1

for f in "$inputDirectory"/*
do
	ffmpeg -v 0 -i $f -vf select='eq(pict_type\,P)' -vsync 2 -f image2 dump/outputimage%03d.jpg
	noFrames=$(ls -1 dump | wc -l)
	echo $No of P-frames in $f: $noFrames
	rm dump/*
done

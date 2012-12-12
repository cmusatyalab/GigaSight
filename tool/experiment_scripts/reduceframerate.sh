inputFile=$1
outputFile=$2

#two-steps  needed
ffmpeg -i $inputFile -acodec copy -vcodec copy -r 1 output.mp4
ffmpeg -i output.mp4 -vf setpts=0.041*PTS -an $outputFile

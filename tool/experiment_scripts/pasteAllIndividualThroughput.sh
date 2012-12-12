fileBase=$1
#fileBase=/tmp/1080p_2_Pittsburgh_1fps_5_7
outputFile=${fileBase}_individualthroughput_joined.txt
$(cat ${fileBase}_0_individualthroughput.txt | cut -f1 > dump00.txt)
$(cat ${fileBase}_0_individualthroughput.txt | cut -f2 > dump01.txt)
$(cat ${fileBase}_1_individualthroughput.txt | cut -f1 > dump10.txt)
$(cat ${fileBase}_1_individualthroughput.txt | cut -f2 > dump11.txt)
$(cat ${fileBase}_2_individualthroughput.txt | cut -f1 > dump20.txt)
$(cat ${fileBase}_2_individualthroughput.txt | cut -f2 > dump21.txt)
$(cat ${fileBase}_3_individualthroughput.txt | cut -f1 > dump30.txt)
$(cat ${fileBase}_3_individualthroughput.txt | cut -f2 > dump31.txt)
$(cat ${fileBase}_4_individualthroughput.txt | cut -f1 > dump40.txt)
$(cat ${fileBase}_4_individualthroughput.txt | cut -f2 > dump41.txt)
#$(cat ${fileBase}_5_throughput.txt | cut -f1 > dump50.txt)
#$(cat ${fileBase}_5_throughput.txt | cut -f2 > dump51.txt)
#$(cat ${fileBase}_6_throughput.txt | cut -f1 > dump60.txt)
#$(cat ${fileBase}_6_throughput.txt | cut -f2 > dump61.txt)
#$(cat ${fileBase}_7_throughput.txt | cut -f1 > dump70.txt)
#$(cat ${fileBase}_7_throughput.txt | cut -f2 > dump71.txt)
#$(cat ${fileBase}_8_throughput.txt | cut -f1 > dump80.txt)
#$(cat ${fileBase}_8_throughput.txt | cut -f2 > dump81.txt)
#$(cat ${fileBase}_9_throughput.txt | cut -f1 > dump90.txt)
#$(cat ${fileBase}_9_throughput.txt | cut -f2 > dump91.txt)

$(paste dump00.txt dump01.txt dump10.txt dump11.txt dump20.txt dump21.txt dump30.txt dump31.txt dump40.txt dump41.txt > dump.txt)
#dump50.txt dump51.txt dump60.txt dump61.txt dump70.txt dump71.txt dump80.txt dump81.txt dump90.txt dump91.txt > dump.txt) 
#$(cat dump.txt | tr -d . > $outputFile)
cat dump.txt > $outputFile
echo output written to $outputFile

rm dump*

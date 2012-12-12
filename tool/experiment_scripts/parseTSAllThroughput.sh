fileBase=$1

node=1
while [ $node -lt 9 ]
do
iter=0
	while [ $iter -lt 1 ]
	do
		file=${fileBase}_${node}_${iter}_tshark.log
		echo Parsing $file
		outputfileBase=${fileBase}_${node}_${iter}_throughput
		sudo tshark -q -z io,stat,1 -r $file | grep "-"  | awk '{ print $3 }' > ${outputfileBase}.tmp
		#cat $outputfileBase.tmp | cut -f1 > ${outputfileBase}_1.tmp
		#cat $outputfileBase.tmp | cut -f2 > ${outputfileBase}_2.tmp
		iter=$((iter + 1))
	done
	#paste ${fileBase}_${node}_0_1.log ${fileBase}_${node}_0_2.log ${fileBase}_${node}_1_1.log ${fileBase}_${node}_1_2.log ${fileBase}_${node}_2_1.log ${fileBase}_${node}_2_2.log ${fileBase}_${node}_3_1.log ${fileBase}_${node}_3_2.log ${fileBase}_${node}_4_1.log ${fileBase}_${node}_4_2.log  ${fileBase}_${node}_5_1.log  ${fileBase}_${node}_5_2.log ${fileBase}_${node}_6_1.log ${fileBase}_${node}_6_2.log ${fileBase}_${node}_7_1.log ${fileBase}_${node}_7_2.log ${fileBase}_${node}_8_1.log ${fileBase}_${node}_8_2.log ${fileBase}_${node}_9_1.log ${fileBase}_${node}_9_2.log  > dump.txt
	node=$((node + 1))
done

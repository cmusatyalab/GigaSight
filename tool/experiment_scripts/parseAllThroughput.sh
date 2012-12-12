fileBase=$1

node=1
while [ $node -lt 9 ]
do
iter=0
	while [ $iter -lt 10 ]
	do
		file=${fileBase}_${node}_${iter}.log
		echo Parsing $file
		bash ~/scripts/parseThroughput.sh $file 
		iter=$((iter + 1))
	done
node=$((node + 1))
done

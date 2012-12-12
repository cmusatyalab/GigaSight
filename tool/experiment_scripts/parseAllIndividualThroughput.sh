fileBase=$1

node=1
while [ $node -lt 9 ]
do
iter=0
	while [ $iter -lt 5 ]
	do
		file=${fileBase}_${node}_${iter}.log
		echo Parsing $file
		bash ~/scripts/parseIndividualThroughput.sh $file 
		iter=$((iter + 1))
	done
node=$((node + 1))
done

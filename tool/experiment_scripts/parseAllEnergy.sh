fileBase=$1

node=1
while [ $node -lt 9 ]
do
iter=0
	while [ $iter -lt 10 ]
	do
		file=${fileBase}_${node}_${iter}.csv
		echo Parsing $file
		bash ~/scripts/parseEnergy.sh $file 
		iter=$((iter + 1))
	done
node=$((node + 1))
done

#!/bin/bash

LOG_PATH=/home/sadve/shari2/GEMS/simics/home/dynamic_relyzer/fully_optimized_main_logs;

if [ -d $LOG_PATH ]; then 
	echo "Directory  found"
else
	echo "$LOG_PATH not found"
	exit 1
fi

for app in $(echo blackscholes fluidanimate streamcluster swaptions fft lu ocean water gcc libquantum mcf omnetpp)
do
	for f_type in $(echo both) 
	do
		echo "$file"
		perl -w perl/find_diff_list.pl $app output/fully_optimized/$app\_$app\_fault_list.txt output/$app\_$app\_fault_list.txt $f_type; 
	done
done 

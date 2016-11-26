#!/bin/bash

LOG_PATH=/home/sadve/shari2/GEMS/simics/home/dynamic_relyzer/fully_optimized_main_logs;
#STORE_LOG_PATH=/home/sadve/shari2/GEMS/simics/home/mem_profile/fully_optimized_main_logs;
STORE_LOG_PATH=$LOG_PATH;

if [ -d $LOG_PATH ]; then 
	echo "Directory  found"
else
	echo "$LOG_PATH not found"
	exit 1
fi

#for app in $(echo blackscholes fluidanimate streamcluster swaptions lu ocean water gcc libquantum omnetpp)
for app in $(echo blackscholes fluidanimate streamcluster swaptions fft lu ocean water gcc libquantum omnetpp)
#for app in $(echo ocean)
do
	for file in $(ls output/diff_lists/$app*fault_list.txt) 
	do
		for fault_type in $(echo reg) 
		do
			echo "$file"
			perl -w perl/gen_store_simics_files.pl $app $file $LOG_PATH/$app*_modes.txt $STORE_LOG_PATH/$app*-heap-classes.output $STORE_LOG_PATH/$app*-stack-classes.output $fault_type; 
		done
	done
done 


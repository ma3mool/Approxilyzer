#!/bin/bash

LOG_PATH=$GEMS/simics/home/dynamic_relyzer/fully_optimized_main_logs_new;
#LOG_PATH=/home/sadve/shari2/GEMS/simics/home/dynamic_relyzer/fully_optimized_main_logs;
#LOG_PATH=/home/sadve/shari2/GEMS/simics/home/dynamic_relyzer/optimized_main_logs;

if [ -d $LOG_PATH ]; then 
	echo "Directory  found"
else
	echo "$LOG_PATH not found"
	exit 1
fi

#for app in $(echo blackscholes fluidanimate streamcluster swaptions fft lu ocean water gcc libquantum omnetpp)
for app in $(echo blackscholes)
do
	for file in $(ls output/$app*fault_list.txt) 
	do
		echo "$file"
		perl -w perl/gen_random_simics_files.pl $app $file $LOG_PATH/$app*_modes.txt $LOG_PATH/$app*-heap-classes.output  $LOG_PATH/$app*-stack-classes.output $LOG_PATH/$app*sample_control_pc_patterns.txt; 
	done
done 


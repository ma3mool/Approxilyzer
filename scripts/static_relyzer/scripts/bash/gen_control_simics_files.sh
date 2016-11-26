#!/bin/bash

LOG_PATH=/home/sadve/shari2/GEMS/simics/home/dynamic_relyzer/fully_optimized_main_logs;

if [ -d $LOG_PATH ]; then 
	echo "Directory  found"
else
	echo "$LOG_PATH not found"
	exit 1
fi

#for app in $(echo blackscholes fluidanimate streamcluster swaptions fft lu ocean water gcc libquantum mcf omnetpp)
#for app in $(echo fft ocean)
for app in $(echo blackscholes swaptions lu water)
#for app in $(echo swaptions)
do
	for file in $(ls output/$app*fault_list.txt) 
	do

		for fault_type in $(echo reg) 
		do
			echo "$file"
			perl -w perl/gen_control_simics_files.pl $app 0 $file $LOG_PATH/$app*_modes.txt $LOG_PATH/$app*sample_control_pc_patterns.txt $fault_type; # > pc_list.txt
		done
	done
done 


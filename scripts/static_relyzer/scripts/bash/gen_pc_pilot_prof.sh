#!/bin/bash

#LOG_PATH=/home/sadve/shari2/GEMS/simics/home/dynamic_relyzer/fully_optimized_main_logs;
LOG_PATH=/shared/workspace/amahmou2/Relyzer_GEMS/src_GEMS/simics/home/dynamic_relyzer/fully_optimized_main_logs_new;

if [ -d $LOG_PATH ]; then 
	echo "Directory  found"
else
	echo "$LOG_PATH not found"
	exit 1
fi

rm pilot_pop_map.txt

#for app in $(echo blackscholes fluidanimate streamcluster swaptions fft lu ocean water gcc mcf libquantum omnetpp)
for app in $(echo blackscholes)
#for app in $(echo gcc)
#for app in $(echo blackscholes swaptions fft lu libquantum )
# for app in $(echo mcf)
do
	for file in $(ls output/$app*fault_list.txt) 
	do
		for f_type in $(echo both) 
		do
			echo "$file"
			perl -w perl/gen_pc_pilot_prof.pl $app $file $LOG_PATH/$app*_modes.txt $LOG_PATH/$app*-heap-classes.output  $LOG_PATH/$app*-stack-classes.output $LOG_PATH/$app*sample_control_pc_patterns.txt $f_type; 
		done
	done
done 


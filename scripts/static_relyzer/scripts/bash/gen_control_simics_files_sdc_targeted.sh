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
#for app in $(echo blackscholes)
for app in $(echo swaptions)
#for app in $(echo fft)
#for app in $(echo blackscholes swaptions lu water)
do
	for file in $(ls output/$app*fault_list.txt) 
	do

		for fault_type in $(echo reg) 
		do
			echo "$file"
			perl -w perl/gen_control_simics_files_sdc_targeted.pl $app 0 $file $LOG_PATH/$app*_modes.txt /home/sadve/shari2/GEMS/simics/home/dynamic_relyzer/pickle_dir/consolidated/$app*_sample_control_pc_patterns50.txt $fault_type /home/sadve/shari2/GEMS/simics/home/live_analysis/parsed_results/$app*_all_sdcs.txt; # > pc_list.txt
			#perl -w perl/gen_control_simics_files_sdc_targeted.pl $app 0 $file $LOG_PATH/$app*_modes.txt $LOG_PATH\_new/$app*sample_control_pc_patterns_20_500.txt $fault_type /home/sadve/shari2/GEMS/simics/home/live_analysis/parsed_results/$app*_all_sdcs_sorted.txt; # > pc_list.txt
			#perl -w perl/gen_control_simics_files_sdc_targeted.pl $app 0 $file $LOG_PATH/$app*_modes.txt /home/sadve/shari2/GEMS/simics/home/dynamic_relyzer/pickle_dir/consolidated/swaptions_simsmall_sample_control_pc_patterns100.txt $fault_type /home/sadve/shari2/GEMS/simics/home/live_analysis/parsed_results/$app*_all_sdcs_sorted.txt; # > pc_list.txt
		done
	done
done 


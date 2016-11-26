#!/bin/bash

# LOG_PATH=/nfshome/sshari/research/GEMS/simics/home/test/logs;
# LOG_PATH=./logs;
# dynamic_relyzer_path=/home/sadve/shari2/GEMS/simics/home/dynamic_relyzer/main_logs;

#LOG_PATH=/home/sadve/shari2/GEMS/simics/home/dynamic_relyzer/fully_optimized_main_logs;
LOG_PATH=/shared/workspace/amahmou2/Relyzer_GEMS/src_GEMS/simics/home/dynamic_relyzer/fully_optimized_main_logs_new;

if [ -d $LOG_PATH ]; then 
	echo "Directory  found"
else
	echo "$LOG_PATH not found"
	exit 1
fi

rm -f dyn_all_pruned.txt dyn_reg_pruned.txt dyn_int_pruned.txt
rm -f all_cat_file.txt reg_cat_file.txt int_cat_file.txt
rm -f brk_file.txt
rm -f pruned_all_cat_file.txt pruned_reg_cat_file.txt pruned_int_cat_file.txt

#for app in $(echo blackscholes fluidanimate streamcluster swaptions fft lu ocean water gcc libquantum mcf  omnetpp)
#for app in $(echo blackscholes swaptions fft lu water libquantum)
#for app in $(echo swaptions)
for app in $(echo blackscholes)
do

	chkpt_name="";
	if [ $app = "blackscholes" ] ; then 
		#chkpt_name="blackscholes_simlarge";
		#chkpt_name="blackscholes_simlarge_abdul";
		chkpt_name="blackscholes_abdul";
	fi;
	if [ $app = "fluidanimate" ] ; then 
		chkpt_name="fluidanimate_simsmall";
	fi;
	if [ $app = "streamcluster" ] ; then 
		chkpt_name="streamcluster_simsmall";
	fi;
	if [ $app = "swaptions" ] ; then 
		chkpt_name="swaptions_simsmall";
	fi;
	if [ $app = "fft" ] ; then 
		chkpt_name="fft_small";
	fi;
	if [ $app = "lu" ] ; then 
		chkpt_name="lu_reduced";
	fi;
	if [ $app = "ocean" ] ; then 
		chkpt_name="ocean_small";
	fi;
	if [ $app = "water" ] ; then 
		chkpt_name="water_small";
	fi;
	if [ $app = "gcc" ] ; then 
		chkpt_name="gcc_test";
	fi;
	if [ $app = "mcf" ] ; then 
		chkpt_name="mcf_test";
	fi;
	if [ $app = "omnetpp" ] ; then 
		chkpt_name="omnetpp_test";
	fi;
	if [ $app = "libquantum" ] ; then 
		chkpt_name="libquantum_test";
	fi;

	#echo "list_pc_$app = []" >> brk_file.txt
	#echo "list_sample_size_$app = []" >> brk_file.txt
	#echo "list_population_$app = []" >> brk_file.txt
	for file in $(ls output/$app*) 
	do
		echo "$file"
		perl -W perl/merge-fault-list.pl $app $file $LOG_PATH/$chkpt_name-heap-inj.output $LOG_PATH/$chkpt_name-stack-inj.output merged_fault_list/random_pilot/$file $LOG_PATH/$app*_modes.txt $LOG_PATH/$chkpt_name\_control_pc_patterns.txt; # > pc_list.txt
		#perl -W perl/merge-fault-list.pl $app $file $LOG_PATH/$chkpt_name-heap-randominj.output $LOG_PATH/$chkpt_name-stack-randominj.output merged_fault_list/random_pilot/$file $LOG_PATH/$app*_modes.txt $LOG_PATH/$chkpt_name\-control-randominj.output; # > pc_list.txt
	done
done 


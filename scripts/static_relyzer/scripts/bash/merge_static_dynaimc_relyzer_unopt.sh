#!/bin/bash

# LOG_PATH=/nfshome/sshari/research/GEMS/simics/home/test/logs;
# LOG_PATH=./logs;
# dynamic_relyzer_path=/home/sadve/shari2/GEMS/simics/home/dynamic_relyzer/main_logs;

LOG_PATH=/home/sadve/shari2/GEMS/simics/home/dynamic_relyzer/unopt_main_logs;
STORE_LOG_PATH=/home/sadve/shari2/GEMS/simics/home/lu_test/unopt_main_logs;
#STORE_LOG_PATH=$LOG_PATH;

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

#for app in $(echo blackscholes fluidanimate streamcluster swaptions fft lu ocean water gcc mcf libquantum omnetpp)
for app in $(echo blackscholes swaptions fft lu libquantum)
do

	chkpt_name="";
	if [ $app = "blackscholes" ] ; then 
		chkpt_name="blackscholes_simlarge";
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
	if [ $app = "libquantum" ] ; then 
		chkpt_name="libquantum_test";
	fi;

	#echo "list_pc_$app = []" >> brk_file.txt
	#echo "list_sample_size_$app = []" >> brk_file.txt
	#echo "list_population_$app = []" >> brk_file.txt
	for file in $(ls output/unopt/$app*) 
	do
		echo "$file"
		perl -W perl/merge-fault-list.pl $app $file $STORE_LOG_PATH/$chkpt_name-heap-inj.output $STORE_LOG_PATH/$chkpt_name-stack-inj.output merged_fault_list/$file $LOG_PATH/$app*_modes.txt $LOG_PATH/$chkpt_name\_control_pc_patterns.txt; # > pc_list.txt
	done
done 


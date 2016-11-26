#!/bin/bash

# LOG_PATH=/nfshome/sshari/research/GEMS/simics/home/test/logs;
# LOG_PATH=./logs;
# dynamic_relyzer_path=/home/sadve/shari2/GEMS/simics/home/dynamic_relyzer/main_logs;

LOG_PATH=/home/sadve/shari2/GEMS/simics/home/dynamic_relyzer/optimized_main_logs;
dynamic_relyzer_path=/home/sadve/shari2/GEMS/simics/home/dynamic_relyzer/optimized_main_logs;

if [ -d $LOG_PATH ]; then 
	echo "Directory  found"
else
	echo "$LOG_PATH not found"
	exit 1
fi

rm dyn_pruned.txt
rm cat_file.txt
rm brk_file.txt

#for app in $(echo lu fft blackscholes swaptions)
for app in $(echo lu blackscholes swaptions)
#for app in $(echo fft)
# for app in $(echo lu)
do
	
	echo "list_pc_$app = []" >> brk_file.txt
	echo "list_sample_size_$app = []" >> brk_file.txt
	echo "list_population_$app = []" >> brk_file.txt
	for file in $(ls output/$app*) 
	do
		echo "$file"
		perl perl/merge-fault-list.pl $app $file $LOG_PATH/$app*-stack-inj.output $LOG_PATH/$app*-heap-inj.output merged_fault_list/$file $dynamic_relyzer_path/$app*_modes.txt; # > pc_list.txt
		#for i in $(cat pc_list.txt); do grep $i apps/$app/$app.dis; done | cut -d' ' -f22 | grep "^b" | wc
	done
done 

#perl perl/merge-fault-list.pl output/fft_fft_fault_list.txt $LOG_PATH/fft_small_merged-stack-inj.output $LOG_PATH/fft_small_merged-heap-inj.output merged_fault_list/fft.txt


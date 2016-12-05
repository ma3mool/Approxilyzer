#!/bin/bash

if [ $# -ne 1 ]; then
    echo "Usage: ./gen_all_simics_files.sh [app name]"
    echo "Sample use: ./gen_all_simics_files.sh blackscholes_simlarge"
    echo
    exit 1
fi

### DIR STRUCTURE ###

DIR_0=$APPROXILYZER/workloads/apps/

if [[ ! -d $DIR_0/${1} ]]; then
    echo "No directory created for ${1} in workspace/app"
    exit 1
fi

if [[ ! -d $DIR_0/${1}/analysis_output ]]; then
    echo "No analysis_output directory created in workspace/app/${1}"
    exit 1
fi

LOG_PATH=${DIR_0}/${1}/analysis_output/                         # analysis directory
INPUT_PATH=$APPROXILYZER/fault_list_output/                   # fault list
PERL_PATH=$APPROXILYZER/scripts/static_relyzer/scripts/perl/  # perl script


if [ -d $INPUT_PATH ]; then 
	echo "Directory  found"
else
	echo "$INPUT_PATH not found"
	exit 1
fi

if [ -d $LOG_PATH ]; then 
	echo "Directory  found"
else
	echo "$LOG_PATH not found"
	exit 1
fi

###################


rm pilot_pop_map_common.txt

app_name=$1
echo $app_name

for app in $(echo $app_name)

do
	for file in $(ls $INPUT_PATH/${app}_fault_list.txt) 
	do
		for f_type in $(echo reg) 
		do
			echo "$file"
			perl -w $PERL_PATH/gen_all_simics_files.pl $app $file $LOG_PATH/${1}_modes.txt $LOG_PATH/${1}-heap-classes.output  $LOG_PATH/${1}-stack-classes.output $LOG_PATH/${1}_sample_control_pc_patterns_50_5000.txt  $f_type; 
			#perl -w perl/gen_all_simics_files.pl $app $file $LOG_PATH/$app*_modes.txt $LOG_PATH/$app*-heap-classes.output  $LOG_PATH/$app*-stack-classes.output $LOG_PATH/$app*_sample_control_pc_patterns_50_5000.txt  $f_type; 
			#perl -w perl/gen_all_simics_files.pl $app $file $LOG_PATH/$app*_modes.txt $LOG_PATH/$app*-heap-classes.output  $LOG_PATH/$app*-stack-classes.output /home/sadve/shari2/GEMS/simics/home/dynamic_relyzer/pickle_dir/consolidated/$app*_sample_control_pc_patterns50.txt  $f_type; 
			#perl -w perl/gen_all_simics_files.pl $app $file $LOG_PATH/$app*_modes.txt $LOG_PATH/$app*-heap-classes.output  $LOG_PATH/$app*-stack-classes.output $LOG_PATH\_new/$app*sample_control_pc_patterns_20_500.txt $f_type; 
			#perl -w perl/gen_all_simics_files.pl $app $file $LOG_PATH/$app*_modes.txt $LOG_PATH/$app*-heap-classes.output  $LOG_PATH/$app*-stack-classes.output $LOG_PATH\_new/$app*sample_control_pc_patterns_5_500.txt $f_type; 
			#perl -w perl/gen_all_simics_files.pl $app $file $LOG_PATH/$app*_modes.txt $LOG_PATH/$app*-heap-classes.output  $LOG_PATH/$app*-stack-classes.output $LOG_PATH/$app*sample_control_pc_patterns.txt $f_type; 
			#perl -w perl/sort_list.pl simics_file_list/simics_file_list_$app\_*_all.txt
		done
	done
done 


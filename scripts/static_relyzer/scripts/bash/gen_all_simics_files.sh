#!/bin/bash

if [ $# -gt 2 ] || [ $# -lt 1 ]; then
    echo "Usage: ./gen_all_simics_files.sh [app name] (depth)"
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
            if [ $# -eq 2 ]; then
			    perl -w $PERL_PATH/gen_all_simics_files.pl $app $file $LOG_PATH/${1}_modes.txt $LOG_PATH/${1}-heap-classes.output  $LOG_PATH/${1}-stack-classes.output $LOG_PATH/${1}_sample_control_pc_patterns_${2}_5000.txt  $f_type; 
            else
			    perl -w $PERL_PATH/gen_all_simics_files.pl $app $file $LOG_PATH/${1}_modes.txt $LOG_PATH/${1}-heap-classes.output  $LOG_PATH/${1}-stack-classes.output $LOG_PATH/${1}_sample_control_pc_patterns_50_5000.txt  $f_type; 
            fi
		done
	done
done 


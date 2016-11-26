#!/bin/bash

# Preprocessing to prepare the scripts for use_def_prune.pl
# Takes in the outcome of all injections (*.outcomes_final) and:
#   1. removes all Masked and Detected from the outcomes
#   2. copies over all the 0's for *all_sdc.txt (needs to be run before this script)
#   3. marks all those 0's as masked
#   4. runs use_def_prune.pl
#   5. replaces the masked with the output of use_def_prune.pl

INJECTION_RESULTS_DIR=$RELYZER_SHARED/fault_list_output/injection_results
INJECTION_SCRIPTS=$RELYZER_SHARED/scripts/injections_scripts
OUTCOMES_PATH=$INJECTION_RESULTS_DIR/raw_outcomes # directory with input files

PARSED_RESULTS=$INJECTION_RESULTS_DIR/parsed_results
OUTPUT_DIR=$INJECTION_RESULTS_DIR/full_sdc_list   # directory where the new outputs will be written to

TEMP_DIR=$OUTPUT_DIR/temp_files


if [ -d $OUTCOMES_PATH ]; then
    echo "Input directory found"
else
    echo "$OUTCOMES_PATH not found"
    exit 1
fi

if [ $# -ne 1 ]; then
    echo "Usage: ./prepare_use_def_input.sh [app_name]"
    echo "Sample use: ./prepare_use_def_input.sh blackscholes_simlarge"
    echo
    echo "Warning: You should still look into the script to insure correct paths"
    exit 1
fi

input_file="$OUTCOMES_PATH/${1}.outcomes_raw"
all_sdc_file="$PARSED_RESULTS/$1_all_sdcs.txt"
output_file="$OUTPUT_DIR/$1.full_sdc_list"
stats_file="$OUTPUT_DIR/$1.stats"

###################################### STEP 1 ######################################
echo -ne '\E[1;33;44m'"REMOVING NON-SDC INJECTION OUTCOMES"; tput sgr0
echo

grep SDC $input_file > $TEMP_DIR/$1_just_sdcs.tmp
cp $TEMP_DIR/$1_just_sdcs.tmp $output_file
echo "\nWord count for Step 1: Removing Non-SDC Injection Outcomes:" > $stats_file
wc -l $TEMP_DIR/$1_just_sdcs.tmp >> $stats_file 


###################################### STEP 2 ######################################
echo -ne '\E[1;33;44m'"APPENDING PRUNED DEFS TO SDCS WITH \"MASKED\" LABEL"; tput sgr0
echo

grep ":0$" $all_sdc_file > $TEMP_DIR/$1_pruned_defs.tmp

echo "\nWord count for Step 2a: Total Pruned Defs:" >> $stats_file
wc -l $TEMP_DIR/$1_pruned_defs.tmp >> $stats_file

gawk -F: '{print $2 "::Masked" }' $TEMP_DIR/$1_pruned_defs.tmp >> $TEMP_DIR/$1_just_sdcs.tmp

echo "\nWord count for Step 2b: Appended Pruned Defs to SDCs:" >> $stats_file
wc -l $TEMP_DIR/$1_just_sdcs.tmp >> $stats_file 


###################################### STEP 3 ######################################
echo -ne '\E[1;33;44m'"RUNNING USE_DEF_PRUNING.PL SCRIPT"; tput sgr0
echo

cd $INJECTION_SCRIPTS/
perl use_def_prune.pl $1 $2

echo "\nWord count for Step 3: Def Outcomes after running Use-Def script" >> $stats_file
wc -l $TEMP_DIR/$1_def.outcomes >> $stats_file 


###################################### STEP 4 ######################################
echo -ne '\E[1;33;44m'"FINILIZING FULL SDC LIST"; tput sgr0
echo

cat $TEMP_DIR/$1_def.outcomes >> $output_file
echo "\nWord count for Step 4: Finilizing Script" >> $stats_file
wc -l $output_file >> $stats_file 


###################################### STEP 5 ######################################
echo -ne '\E[1;33;44m'"CLEAN UP"; tput sgr0
echo


rm $TEMP_DIR/$1_def.outcomes
rm $TEMP_DIR/$1_just_sdcs.tmp
rm $TEMP_DIR/$1_pruned_defs.tmp







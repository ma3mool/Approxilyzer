#!/bin/bash

# Runs injections and cleans up the output for Approxilyzer

# How to use this script
if [ $# -ne 2 ]; then
    echo "Usage: ./relyzer_injections.sh [app_name] [phase #]"
    echo "Possible values for [phase #] are {1-4}. Use 4 for all."
    echo "Sample use: ./relyzer_injections.sh blackscholes_simlarge 4"
    exit 1
fi

# checks to make sure file certain paths exist. Relyzer, GEMS, and SIMICS, Relyzer apps
if [ -d $RELYZER_SHARED ]; then
    echo "Relyzer directory exists"
else
    echo "No Relyzer directory. Add it to your .bashrc"
    exit 1
fi


INJECTION_SCRIPTS=$RELYZER_SHARED/scripts/injections_scripts
INJECTION_RESULTS=$RELYZER_SHARED/fault_list_output/injection_results

# Phase 1: parallel write optimization
if [ $2 -eq "1" ]; then

    # create the 625 files to parallelize writes 
    $INJECTION_SCRIPTS/reset_outcomes.sh $1

    # create condor injection script. Run offline inside screen
    echo "READY FOR INJECTIONS:"
    echo "Run submit_full_injection_jobs <simics_file_list> on Veena-Server"

fi  


# Phase 2: combine to a single file, remove temp files

if [ $2 -eq "2" ] || [ $2 -eq "4" ]; then
# concatenate all outputs to a single file
    touch $INJECTION_RESULTS/raw_outcomes/${1}.outcomes_raw  #TODO CHECK IF ALREADY FILLED...
    cat $INJECTION_RESULTS/parallel_outcomes/$1-* >> $INJECTION_RESULTS/raw_outcomes/${1}.outcomes_raw
    cp $INJECTION_RESULTS/parallel_outcomes/${1}.retries $INJECTION_RESULTS/raw_outcomes
    rm $INJECTION_RESULTS/parallel_outcomes/${1}-*
    rm $INJECTION_RESULTS/parallel_outcomes/${1}.retries
fi


# Phase 3: Clean up output
if [ $2 -eq "3" ] || [ $2 -eq "4" ]; then
# gen_reformatted_output.sh - this calls two scripts
# this provides the *_all_sdc.txt files
    $INJECTION_SCRIPTS/gen_reformatted_output.sh $1


# run prepare_use_def_input.sh
    $INJECTION_SCRIPTS/prepare_use_def_input.sh $1
fi

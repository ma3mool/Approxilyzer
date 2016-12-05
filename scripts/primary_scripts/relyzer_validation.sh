#!/bin/bash

# Runs injections and cleans up the output for Approxilyzer

# How to use this script
if [ $# -ne 2 ]; then
    echo "Usage: ./relyzer_validation.sh [app_name] [phase #]"
    echo "Possible values for [phase #] are {1-4}. Use 4 for all."
    echo "Sample use: ./relyzer_validation.sh blackscholes_simlarge 4"
    exit 1
fi

# checks to make sure file certain paths exist. Relyzer, GEMS, and SIMICS, Relyzer apps
if [ -d $APPROXILYZER ]; then
    echo "APPROXILYZER directory exists"
else
    echo "No Approxilyzer directory. Add it to your .bashrc"
    exit 1
fi


INJECTION_SCRIPTS=$APPROXILYZER/scripts/injections_scripts
INJECTION_RESULTS=$APPROXILYZER/fault_list_output/injection_results
VALIDATION_RESULTS=$APPROXILYZER/fault_list_output/validation_results


# Phase 1: parallel write optimization
if [ $2 -eq "1" ]; then

    # create the 625 files to parallelize writes 
    $INJECTION_SCRIPTS/reset_outcomes.sh $1

    # create condor injection script. Run offline inside screen
    echo "READY FOR INJECTIONS:"
    echo "Run $APPROXILYZER/scripts/injections_scripts/submit_full_injection_jobs <validation_file_list> on Veena-Server"

fi  


# Phase 2: combine to a single file, remove temp files

if [ $2 -eq "2" ] || [ $2 -eq "4" ]; then
# concatenate all outputs to a single file
    rm $VALIDATION_RESULTS/${1}.outcomes_validation
    touch $VALIDATION_RESULTS/${1}.outcomes_validation  #TODO CHECK IF ALREADY FILLED...
    cat $INJECTION_RESULTS/parallel_outcomes/$1-* >> $VALIDATION_RESULTS/${1}.outcomes_validation
    cp $INJECTION_RESULTS/parallel_outcomes/${1}.retries $VALIDATION_RESULTS
fi




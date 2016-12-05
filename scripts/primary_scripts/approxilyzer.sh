#!/bin/bash

# This script runs Approxilyzer, which takes in the output of Relyzer (SDCs) and classifies them further 
# TODO FEATURES:
#   run -- currently phase 1
#       provide metric as well
#   plot -- matplotlib plot of distribution of buckets
#       could add other plotting features too
#   pc_list -- provides list of pc's, and which need to be covered


# How to use this script
if [ $# -ne 2 ]; then
    echo "Usage: ./approxilyzer [app_name] [phase #]"
    echo "Possible values for [phase #] are {1-4}. Use 4 for all."
    echo "Sample use: ./approxilyzer blackscholes_simlarge 4"
    exit 1
fi

# checks to make sure file certain paths exist. Relyzer, GEMS, and SIMICS, Relyzer apps
if [ -d $APPROXILYZER ]; then
    echo "APPROXILYZER directory exists"
else
    echo "No Relyzer directory. Add it to your .bashrc"
    exit 1
fi

if [ -d $GEMS_PATH ]; then
    echo "GEMS_PATH directory exists"
else
    echo "No GEMS_PATH directory. Add it to your .bashrc"
    exit 1
fi

if [ -d $RELYZER_APPS ]; then
    echo "RELYZER_APPS directory exists"
else
    echo "No RELYZER_APPS directory. Add it to your .bashrc"
    exit 1
fi


APPROXILYZER_SCRIPTS=$APPROXILYZER/scripts/approx_computing


# Phase 1: generate SDC levels for each pilot

if [ $2 -eq "1" ] || [ $2 -eq "4" ]; then
    ############################## PHASE 1 of APPROXILYZER ############################## 
    echo -ne '\E[1;33;44m'"RUNNING APPROXILYZER ON $1"; tput sgr0
    echo
    # ensure directory structure is set up
    $APPROXILYZER_SCRIPTS/make_dir_structure.sh

    # run sdc categorization
    perl $APPROXILYZER_SCRIPTS/generate_sdc_level.pl $1
fi



# extract PCs that 

if [ $2 -eq "2" ] || [ $2 -eq "4" ]; then
    ############################## APPROXILYZER ############################## 
    echo -ne '\E[1;33;44m'"EXTRACTING SDCS USING APPROXILYZER ON $1"; tput sgr0
    echo
    # ensure directory structure is set up
    $APPROXILYZER_SCRIPTS/extract_pcs.sh $1
fi


# generate impact list for application 

if [ $2 -eq "3" ] || [ $2 -eq "4" ]; then
    ############################## APPROXILYZER ############################## 
    echo -ne '\E[1;33;44m'"GENERATING IMPACT LIST USING APPROXILYZER ON $1"; tput sgr0
    echo
    # ensure directory structure is set up
    perl $APPROXILYZER_SCRIPTS/generate_impact_list.pl $1
fi



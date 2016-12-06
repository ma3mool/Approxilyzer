#!/bin/bash

# This script runs Approxilyzer, which takes in the output of Relyzer (SDCs) and classifies them further 
# TODO FEATURES:
#   run -- currently phase 1
#       provide metric as well
#   plot -- matplotlib plot of distribution of buckets
#       could add other plotting features too
#   pc_list -- provides list of pc's, and which need to be covered


# How to use this script
if [ $# -ne 1 ]; then
    echo "Usage: ./approxilyzer [app_name] "
    echo "Sample use: ./approxilyzer blackscholes_simlarge"
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

RELYZER_APPS=$APPROXILYZER/workloads/apps/

APPROXILYZER_SCRIPTS=$APPROXILYZER/scripts/approx_computing
    ############################## USE CASE ONE: APPROXILYZER ############################## 

GEN_SDC_PATH=$RELYZER_APPS/${1}/gen_sdc_quality_${1}.pl

if [ ! -f $GEN_SDC_PATH ]; then
    echo "MAKE SURE TO PROVIDE A $GEN_SDC_PATH for your app!"
    exit 1
fi

echo -ne '\E[1;33;44m'"RUNNING APPROXILYZER ON $1"; tput sgr0
echo
# GENERATE OUTCOMES LIST
perl $RELYZER_APPS/$1/gen_sdc_quality_${1}.pl $1

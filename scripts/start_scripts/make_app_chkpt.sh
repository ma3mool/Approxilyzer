#!/bin/bash

# Generates a simics script, whose function is to
# create a checkpoint for a given application and run script, from a prepared ISO checkpoint
#
# NOTE: THIS SCRIPT DOES NOT RUN THE SIMICS SCRIPTS, IT JUST GENERATES THEM


script_content()
{
    if [ -z "$1" ]
    then
        echo "No paramters passed into function script_content()"
    fi

    command=${1}

    echo "@sys.path.append(\".\")"
    echo "@import simpoint"
    echo $command
    echo quit
}

if [ $# -ne 3 ]; then
    echo "Usage: ./make_iso_checkpoint.sh [ISO name] [app_name] [run_script_name]"
    echo "Sample use: ./make_iso_checkpoint.sh apps blackscholes_simlarge run"
    echo
    echo "Warning: You should still look into the script to insure correct paths"
    echo "Make sure the ISO is in the correct directory as well."
    exit 1
fi

# make sure checkpoint exists
if [[ ! -e $APPROXILYZER/workloads/checkpoints/${1} ]]; then
    echo "Checkpoint does not exist. Please make a checkpoint of your ISO"
    exit 1
fi


### DIR STRUCTURE ###
DIR_0=$APPROXILYZER/workloads/checkpoints/

if [[ ! -d $DIR_0 ]]; then
    echo "Fix directory structure"
    exit 1
fi

if [[ ! -d $DIR_0/simics_files ]]; then
    mkdir $DIR_0/simics_files
fi

DIR=${DIR_0}/simics_files/       # directory where all these scripts are made


if [[ ! -d $DIR ]]; then
    echo "$DIR not found"
    exit 1
fi


echo -ne '\E[1;33;44m'"GENERATING SIMICS SCRIPTS..."; tput sgr0
echo


#### app_profile
fileName="make_app_run_ckpt_${2}_${3}.simics"
touch $DIR/$fileName
script_content "@simpoint.make_run_checkpoint('$1','$2','$3')" > $DIR/$fileName



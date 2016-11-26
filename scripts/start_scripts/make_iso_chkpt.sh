#!/bin/bash

# Generates a simics script, whose function is to
# create a SIMICS checkpoint with the ISO content copied over and prepared for Relyzer use
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

if [ $# -ne 1 ]; then
    echo "Usage: ./make_iso_checkpoint.sh [ISO name]"
    echo "Sample use: ./make_iso_checkpoint.sh apps"
    echo
    echo "Warning: You should still look into the script to insure correct paths"
    echo "Make sure the ISO is in the correct directory as well."
    exit 1
fi

# make sure ISO exists
if [[ ! -e $RELYZER_SHARED/workloads/iso/${1}.iso ]]; then
    echo "ISO is not in correct location. Please put in workloads/iso"
    exit 1
fi


### DIR STRUCTURE ###
DIR_0=$RELYZER_SHARED/workloads/checkpoints/

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
fileName="make_iso_ckpt_${1}.simics"
touch $DIR/$fileName
script_content "@simpoint.make_iso_checkpoint('$1')" > $DIR/$fileName



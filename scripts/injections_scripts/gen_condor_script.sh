#!/bin/bash

# this is for running the various analysis for Relyzer
# Makes a condor script which can be called on veena-server by "condor_submit [condorFileName]"

SIMICS_DIR=$GEMS_PATH/simics/home/dynamic_relyzer/   # Simics directory to run experiments from 
CONDOR_SCRIPTS_PATH=$APPROXILYZER/fault_list_output/condor_scripts

TIMEOFDAY=$(date +"%m_%d_%Y_%H_%M_%S")

if [ -d $SIMICS_DIR ]; then
    echo "Directory found"
else
    echo "$SIMICS_DIR not found"
    exit 1
fi


if [ $# -ne 2 ]; then
    echo "Usage: ./gen_condor_script.sh [static/dynamic] [app_name]"
    echo "Sample use: ./gen_condor_script.sh static blackscholes"
    echo
    echo "Warning: You should still look into the script to insure correct paths"
    exit 1
fi

#top of condor file
header()
{
    if [ -z "$1" ]
    then
        echo "No paramters passed into function header()"
    fi

    executable=""

    if [ $1 = "static" ]
    then
        executable="$SIMICS_DIR/scripts/run_static_analysis_with_cmd_line.sh"
    fi

    if [ $1 = "dynamic" ] 
    then
        executable="$SIMICS_DIR/scripts/run_dynamic_analysis_with_cmd_line.sh"
    fi
    
    echo "universe            = vanilla"
    echo "requirements        = ((target.memory * 2048) >= ImageSize) && ((Arch == \"X86_64\") || (Arch == \"INTEL\"))"
    echo "Executable          = $executable"
    echo "getenv              = true"
    echo "notification        = error"
    echo
}

# content of condor file
parameters_content()
{
    if [ -z "$1" ]
    then
        echo "No paramters passed into function parameters_content()"
    fi

    parameters=${1}

    echo "arguments           = \"$parameters\""
#    echo "periodic_remove     = (RemoteWallClockTime > 100*10*60)"
    echo "output              = /dev/null"
    echo "error               = /dev/null"
    echo "queue"
    echo
}

cd $CONDOR_SCRIPTS_PATH
condorFileName=$1_$2_$TIMEOFDAY.condor
touch $condorFileName

header "$1" > $condorFileName

if [ $1 = "static" ]; then
    parameters_content "1 0 0 0 0 $2" >> $condorFileName    # 
    parameters_content "0 1 0 0 0 $2" >> $condorFileName
    parameters_content "0 0 1 0 0 $2" >> $condorFileName
    parameters_content "0 0 0 1 0 $2" >> $condorFileName
    parameters_content "0 0 0 0 1 $2" >> $condorFileName
fi


if [ $1 = "dynamic" ]; then
    parameters_content "1 0 0 $2" >> $condorFileName   # heap
    parameters_content "0 1 0 $2" >> $condorFileName   # stack
    parameters_content "0 0 1 $2" >> $condorFileName    # control patterns
fi


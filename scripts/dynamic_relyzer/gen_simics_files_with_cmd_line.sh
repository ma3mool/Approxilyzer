#!/bin/bash

# generates simics files that will analyze the application statically for Relyzer
# generated files should be for
#   - find_app_length
#   - app_profile
#   - mode_change_info
#   - br_profile
#   - count_static_instructions
#
# also generates the simics files for use during dynamic anlysis phase
#   - heap-analyze
#   - stack-analyze
#   - control_patterns
#   - intermediate checkpoints
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


if [ $# -gt 11 ] || [ $# -lt 10 ]; then
    echo "Usage: ./gen_simics_files_with_cmd_line.sh [app_profile] [br_profile] [count_static_inst] [mode_info] [find_app_length] [heap] [stack] [control_patterns] [checkpoints] [app name] (depth)."
    echo "Sample use: ./gen_simics_files_with_cmd_line.sh 1 1 1 1 1 0 0 0 0 blackscholes"
    echo
    echo "Warning: You should still look into the script to insure correct paths"
    exit 1
fi


### DIR STRUCTURE ###
DIR_0=$APPROXILYZER/workloads/apps/

if [[ ! -d $DIR_0/${10} ]]; then
    mkdir $DIR_0/${10}
fi

if [[ ! -d $DIR_0/${10}/simics_files ]]; then
    mkdir $DIR_0/${10}/simics_files
fi

DIR=${DIR_0}/${10}/simics_files/       # directory where all these scripts are made


if [ -d $DIR ]; then
    echo "Directory found"
else
    echo "$DIR not found"
    exit 1
fi


######## Set these parameters to run the scripts you want ##########
RUN_APP_PROFILE=$1          # set to 0 or 1
RUN_BR_PROFILE=$2           # set to 0 or 1 
RUN_COUNT_STATIC_INST=$3    # set to 0 or 1
RUN_MODE_CHANGE_INFO=$4     # set to 0 or 1
RUN_FIND_APP_LENGTH=$5      # set to 0 or 1

RUN_HEAP_ANALYZE=$6         # set to 0 or 1
RUN_STACK_ANALYZE=$7        # set to 0 or 1
RUN_CONTROL_PATTERNS=$8     # set to 0 or 1 
RUN_MAKE_CHKPTS=$9          # set to 0 or 1 
DEPTH=50                    # Control Pattern Depth
if [ $# -eq 11 ]; then
    DEPTH=${11}
fi
echo "Depth=:$DEPTH"

cd $DIR
appSuffix=""

echo -ne '\E[1;33;44m'"GENERATING SIMICS SCRIPTS..."; tput sgr0
echo

for app in $(echo ${10})
do
    appName=$app$appSuffix

    ###### SCRIPTS FOR STATIC ANALYSIS PHASE ######

    #### app_profile
    if [ $RUN_APP_PROFILE -eq 1 ]; then
        file_suf="_app_profile.simics"
        fileName=$appName$file_suf
        touch $fileName
        script_content "@simpoint.app_profile('$appName')" > $fileName
    fi
    
    #### br_profile
    if [ $RUN_BR_PROFILE -eq 1 ]; then
        file_suf="_br_profile.simics"
        fileName=$appName$file_suf
        touch $fileName
        script_content "@simpoint.br_profile('$appName')" > $fileName
    fi
    
    #### count_static_instructions
    if [ $RUN_COUNT_STATIC_INST -eq 1 ]; then
        file_suf="_count_static_instructions.simics"
        fileName=$appName$file_suf
        touch $fileName
        script_content "@simpoint.count_static_instructions('$appName')" > $fileName
    fi

    #### mode_change_info
    if [ $RUN_MODE_CHANGE_INFO -eq 1 ]; then
        file_suf="_mode_change_info.simics"
        fileName=$appName$file_suf
        touch $fileName
        script_content "@simpoint.mode_change_info('$appName')" > $fileName
    fi

    #### find_app_length
    if [ $RUN_FIND_APP_LENGTH -eq 1 ]; then
        file_suf="_find_app_length.simics"
        fileName=$appName$file_suf
        touch $fileName
        script_content "@simpoint.find_length('$appName')" > $fileName
    fi

    ###### SCRIPTS FOR DYNAMIC ANALYSIS PHASE ######

    #### Heap Analysis
    if [ $RUN_HEAP_ANALYZE -eq 1 ]; then
        file_suf="_heap.simics"
        fileName=$appName$file_suf
        touch $fileName
        script_content "@simpoint.profile('$appName', 1)" > $fileName
    fi

    #### Stack Analysis 
    if [ $RUN_STACK_ANALYZE -eq 1 ]; then
        file_suf="_stack.simics"
        fileName=$appName$file_suf
        touch $fileName
        script_content "@simpoint.profile('$appName', 0)" > $fileName
    fi

    #### Control Patterns
    if [ $RUN_CONTROL_PATTERNS -eq 1 ]; then
        depth=$DEPTH
        file_suf="_ctrl_patterns$depth.simics"
        fileName=$appName$file_suf
        touch $fileName
        script_content "@simpoint.new_app_pattern_profile('$appName', 0, -1, $depth)" > $fileName
    fi

    #### Intermediate Checkpoints
    if [ $RUN_MAKE_CHKPTS -eq 1 ]; then
        file_suf="_create_checkpoints.simics"
        fileName=$appName$file_suf
        touch $fileName
        script_content "@simpoint.create_intermediate_checkpoints('$appName')" > $fileName
    fi

done
echo "Done. Finished running script with parameters $1 $2 $3 $4 $5 $6 $7 $8 $9 ${10}."
/shared/workspace/kahmed10/scripts/send_email.sh "Simic Files for $1 $2 $3 $4 $5 $6 $7 $8 $9 ${10}"

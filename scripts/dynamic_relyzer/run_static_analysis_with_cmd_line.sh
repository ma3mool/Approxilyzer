#!/bin/bash

# runs simics files that will analyze the application statically for Relyzer
# executed simics scripts are:
#   - find_app_length
#   - app_profile
#   - mode_change_info
#   - br_profile
#   - count_static_instructions


if [ $# -ne 6 ]; then
    echo "Usage: ./run_static_analysis_with_cmd_line.sh [app_profile] [br_profile] [count_static_inst] [mode_info] [find_app_length] [app name]."
    echo "Sample use: ./run_static_analysis_cmd_line.sh 1 0 1 0 1 blackscholes"
    echo
    echo "Warning: You should still look into the script to insure correct paths"
    exit 1
fi


### DIR STRUCTURE ###

DIR_0=$APPROXILYZER/workloads/apps/

if [[ ! -d $DIR_0/${6} ]]; then
    echo "No directory created for ${6} in workspace/app"
    exit 1
fi

if [[ ! -d $DIR_0/${6}/analysis_output ]]; then
    mkdir $DIR_0/${6}/analysis_output
fi

if [[ ! -d $DIR_0/${6}/analysis_output/error ]]; then
    mkdir $DIR_0/${6}/analysis_output/error
fi

DIR=${DIR_0}/${6}/simics_files/                             # directory where all scripts reside 
SIMICS_DIR=$GEMS_PATH/simics/home/dynamic_relyzer/   # Simics directory to run experiments from 
LOG=${DIR_0}/${6}/analysis_output/                          # output directory
ERROR_LOG=$LOG/error/                                       # stdout and stderr output directory 

LOG_OUTPUT=$(date +"%m_%d_%Y")

if [ -d $DIR ]; then
    echo "Directory found"
else
    echo "$DIR not found"
    exit 1
fi

################


######## Set these parameters to run the scripts you want ##########
RUN_APP_PROFILE=$1          # set to 0 or 1
RUN_BR_PROFILE=$2           # set to 0 or 1 
RUN_COUNT_STATIC_INST=$3    # set to 0 or 1
RUN_MODE_CHANGE_INFO=$4     # set to 0 or 1
RUN_FIND_APP_LENGTH=$5      # set to 0 or 1


cd $SIMICS_DIR
#appSuffix="_opt_abdul"
appSuffix=""

echo -ne '\E[1;33;44m'"RUNNING RELYZER STATIC ANALYSIS..."; tput sgr0
echo


for app in $(echo $6)
do
    appName=$app$appSuffix

    ###### SCRIPTS FOR STATIC ANALYSIS PHASE ######

    #### app_profile
    if [ $RUN_APP_PROFILE -eq 1 ]; then
        file_suf="_app_profile.simics"
        fileName=$appName$file_suf
        errorOut=$LOG_OUTPUT.$fileName.txt
        printf "[%12s] %s\n" $app "Running application profile ..."
        time ./simics -stall -no-win $DIR/$fileName 2&> $ERROR_LOG/$errorOut &
    fi

    #### count_static_instructions
    if [ $RUN_COUNT_STATIC_INST -eq 1 ]; then
        file_suf="_count_static_instructions.simics"
        fileName=$appName$file_suf
        errorOut=$LOG_OUTPUT.$fileName.txt
        printf "[%12s] %s\n" $app "Counting static instructions ..."
        time ./simics -stall -no-win $DIR/$fileName 2&> $ERROR_LOG/$errorOut &
    fi

    #### mode_change_info
    if [ $RUN_MODE_CHANGE_INFO -eq 1 ]; then
        file_suf="_mode_change_info.simics"
        fileName=$appName$file_suf
        errorOut=$LOG_OUTPUT.$fileName.txt
        printf "[%12s] %s\n" $app "Gathering mode change info ..."
        time ./simics -stall -no-win $DIR/$fileName > /dev/null 2> $ERROR_LOG/$errorOut &
    fi

    #### br_profile
    if [ $RUN_BR_PROFILE -eq 1 ]; then
        file_suf="_br_profile.simics"
        file_suf2="_br_profile.txt"
        fileName=$appName$file_suf
        outputName=$appName$file_suf2
        errorOut=$LOG_OUTPUT.$fileName.txt
        printf "[%12s] %s\n" $app "Running branch profile ..."
        time ./simics -stall -no-win $DIR/$fileName > $LOG/$outputName 2> $ERROR_LOG/$errorOut &
    #sed -i '1,17d' $LOG/outputName
    #sed -i '$d' $LOG/outputName
    fi

    #### find_app_length
    if [ $RUN_FIND_APP_LENGTH -eq 1 ]; then
        file_suf="_find_app_length.simics"
        fileName=$appName$file_suf
        errorOut=$LOG_OUTPUT.$fileName.txt
        printf "[%12s] %s\n" $app "Finding app length ..."
        time ./simics -stall -no-win $DIR/$fileName 2&> $ERROR_LOG/$errorOut &
    fi

done
wait
echo "Done. Finished running script with parameters $1 $2 $3 $4 $5 $6."

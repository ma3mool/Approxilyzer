#!/bin/bash

# runs simics files that will analyze the application dynamically for Relyzer
# executed simics scripts are:
#   - heap-analyze 
#   - stack-analyze 
#   - control_patterns

if [ $# -ne 4 ]; then
    echo "Usage: ./run_dynamic_analysis_with_cmd_line.sh [run_heap_analysis] [run_stack_analysis] [run_control_patterns] [app name]."
    echo "Sample use: ./run_dyanmic_analysis_cmd_line.sh 1 0 1 blackscholes"
    echo
    echo "Warning: You should still look into the script to insure correct paths"
    exit 1
fi

### DIR STRUCTURE ###
DIR_0=$APPROXILYZER/workloads/apps/

if [[ ! -d $DIR_0/${4} ]]; then
    echo "No directory created for ${4} in workspace/app"
    exit 1
fi

if [[ ! -d $DIR_0/${4}/analysis_output ]]; then
    mkdir $DIR_0/${4}/analysis_output
fi

if [[ ! -d $DIR_0/${4}/analysis_output/error ]]; then
    mkdir $DIR_0/${4}/analysis_output/error
fi


DIR=${DIR_0}/${4}/simics_files/                             # directory where all scripts reside 
SIMICS_DIR=$GEMS_PATH/simics/home/dynamic_relyzer/   # Simics directory to run experiments from 

LOG=${DIR_0}/${4}/analysis_output/                          # output directory
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
RUN_HEAP_ANALYZE=$1          # set to 0 or 1
RUN_STACK_ANALYZE=$2         # set to 0 or 1
RUN_CONTROL_PATTERNS=$3      # set to 0 or 1 
DEPTH=50                    # Control Pattern Depth



cd $SIMICS_DIR
appSuffix=""

echo -ne '\E[1;33;44m'"RUNNING RELYZER DYNAMIC ANALYSIS..."; tput sgr0
echo


for app in $(echo $4)
do
    appName=$app$appSuffix

    ###### SCRIPTS FOR DYNAMIC ANALYSIS PHASE ######

    #### Heap Analysis 
    if [ $RUN_HEAP_ANALYZE -eq 1 ]; then
        file_suf="_heap.simics"
        fileName=$appName$file_suf
        errorOut=$LOG_OUTPUT.$fileName.txt
        printf "[%12s] %s\n" $app "Analyzing Heap ..."
        time ./simics -stall -no-win $DIR/$fileName 2&> $ERROR_LOG/$errorOut &
    fi

    #### Stack Analysis
    if [ $RUN_STACK_ANALYZE -eq 1 ]; then
        file_suf="_stack.simics"
        fileName=$appName$file_suf
        errorOut=$LOG_OUTPUT.$fileName.txt
        printf "[%12s] %s\n" $app "Analyzing Stack ..."
        time ./simics -stall -no-win $DIR/$fileName 2&> $ERROR_LOG/$errorOut &
    fi

    #### Control Patterns
    if [ $RUN_CONTROL_PATTERNS -eq 1 ]; then
        file_suf=_ctrl_patterns$DEPTH.simics
        fileName=$appName$file_suf
        errorOut=$LOG_OUTPUT.$fileName.txt
        printf "[%12s] %s\n" $app "Obtaining control patterns for depth=$DEPTH ..."
        time ./simics -stall -no-win $DIR/$fileName 2&> $ERROR_LOG/$errorOut &
    fi

done
wait
echo "Done. Finished running script with parameters $1 $2 $3 $4"

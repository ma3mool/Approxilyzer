#!/bin/bash

# cleans up output files for static Relyzer analysis
# cleans up the following scripts for applications: 
#   - find_app_length
#   - app_profile
#   - mode_change_info
#   - br_profile
#   - count_static_instructions

if [ $# -ne 5 ]; then
    echo "Usage: ./clean_up_analysis_output_with_cmd_line.sh [app_profile] [br_profile] [count_static_inst] [mode_info] [app name]."
    echo "Sample use: ./clean_up_analysis_output_with_cmd_line.sh 0 1 0 1 blackscholes"
    echo
    echo "Warning: You should still look into the script to insure correct paths."
    exit 1
fi


### DIR STRUCTURE ###

DIR_0=$APPROXILYZER/workloads/apps/

if [[ ! -d $DIR_0/${5} ]]; then
    echo "No directory created for ${5} in workspace/app"
    exit 1
fi

if [[ ! -d $DIR_0/${5}/analysis_output ]]; then
    echo "No directory analysis_output. Mayhaps you need to run something first."
    exit 1
fi

LOG=${DIR_0}/${5}/analysis_output/      # output directory


######## Set these parameters to run the scripts you want ##########
RUN_APP_PROFILE=$1          # set to 0 or 1
RUN_BR_PROFILE=$2           # set to 0 or 1 
RUN_COUNT_STATIC_INST=$3    # set to 0 or 1 
RUN_MODE_CHANGE_INFO=$4     # set to 0 or 1


cd $LOG
appSuffix=""

echo -ne '\E[1;33;44m'"CLEANING UP RELYZER OUTPUT..."; tput sgr0
echo

#for app in $(echo fft fluidanimate gcc libquantum lu mcf ocean streamcluster swaptions water)
#for app in $(echo fft gcc libquantum lu mcf ocean streamcluster swaptions water)
for app in $(echo $5)
do
    appName=$app$appSuffix

    ###### SCRIPTS FOR CLEANING UP RELYZER OUTPUT ######
    # future optimization: use tail instead of sed... much faster

    #### app_profile
    if [ $RUN_APP_PROFILE -eq 1 ]; then
        file_suf="_profile.txt"
        fileName=$appName$file_suf
        printf "[%12s] %s\n" $app "Cleaning up app_profile..."
        sed -i 's/L//g' $LOG/$fileName              #remove superfluous 'L'
        sed -i 's/\(.*\)/\L\1/g' $LOG/$fileName     #change to lowercase
    fi

    #### count_static_instructions
    if [ $RUN_COUNT_STATIC_INST -eq 1 ]; then
        file_suf="_static_instrn_count.txt"
        fileName=$appName$file_suf
        printf "[%12s] %s\n" $app "Cleaning up count_static_inst..."
        sed -i 's/L//g' $LOG/$fileName              #remove superfluous 'L'
        sed -i 's/\(.*\)/\L\1/g' $LOG/$fileName     #change to lowercase
    fi

    #### mode_change_info
    if [ $RUN_MODE_CHANGE_INFO -eq 1 ]; then
        file_suf="_modes.txt"
        fileName=$appName$file_suf
        printf "[%12s] %s\n" $app "Cleaning up mode_change_info..."
        sed -i '1d' $LOG/$fileName                  #remove 1st line (white space)
    fi

    #### br_profile
    if [ $RUN_BR_PROFILE -eq 1 ]; then
        file_suf="_br_profile.txt"
        fileName=$appName$file_suf
        printf "[%12s] %s\n" $app "Cleaning up br_profile..."
        num=`grep -nm1 ^0x $LOG/$fileName | awk -F: '{ print ($1 - 1) }'` # black magic to extract start of actual output
        sed -i "1,$num d" $LOG/$fileName #MAY NEED TO EDIT HOW MANY LINES ARE DELETED FROM THE TOP
        sed -i '$d' $LOG/$fileName      #remove last line (simics DONE output)
  fi

done
wait
echo "Clean-up complete. My work is done here."
/shared/workspace/amahmou2/scripts/send_email.sh "clean up $1 $2 $3 $4 $5"

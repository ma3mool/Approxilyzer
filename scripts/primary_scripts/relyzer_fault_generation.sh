#!/bin/bash

# This script generates a fault list for a given application and input.

# How to use this script
if [ $# -ne 2 ]; then
    echo "Usage: ./relyzer_fault_generation.sh [app_name] [phase #]"
    echo "Possible values for [phase #] are {1-4}."
    echo "Sample use: ./relyzer_fault_generation.sh blackscholes_simlarge 3"
    exit 1
fi

# checks to make sure file certain paths exist. Approxilyzer , GEMS, and SIMICS, Relyzer apps
if [ -d $APPROXILYZER ]; then
    echo "APPROXILYZER directory exists"
else
    echo "No Approxilyzer directory. Add it to your .bashrc"
    exit 1
fi

if [ -d $GEMS_PATH ]; then
    echo "GEMS_PATH directory exists"
else
    echo "No GEMS_PATH directory. Add it to your .bashrc"
    exit 1
fi

RELYZER_APPS=$APPROXILYZER/workloads/apps

set -e

DYNAMIC_APPROXILYZER_SCRIPTS=$APPROXILYZER/scripts/dynamic_relyzer


if [ $2 -eq "1" ]; then
    ############################## PHASE 1 of APPROXILYZER ############################## 
    
    # create structure for app_txt_info if necessary
    cd $DYNAMIC_APPROXILYZER_SCRIPTS
    ./gen_app_txt_info.sh $1

    # set EMERALD flags for Phase 1 (Profile App)
    EMERALD=$GEMS_PATH/emerald/

    # make emerald
    echo -ne '\E[1;33;44m'"PHASE 1: MAKING EMERALD"; tput sgr0
    echo
    cd $EMERALD
    make clean
    make module DESTINATION=dynamic_relyzer PHASE=1

    # generating scripts
    # Usage: ./gen_simics_files_with_cmd_line.sh [app_profile] [br_profile] [count_static_inst] [mode_info] [find_app_length] [heap] [stack] [control_patterns] [checkpoints] [app name]
    echo -ne '\E[1;33;44m'"PHASE 1: STATIC ANALYSIS"; tput sgr0
    echo
    cd $DYNAMIC_APPROXILYZER_SCRIPTS
    ./gen_simics_files_with_cmd_line.sh 1 1 1 1 1 1 1 1 1 $1
   
    # running static analysis
    # Usage: ./run_static_analysis_with_cmd_line.sh [app_profile] [br_profile] [count_static_inst] [mode_info] [find_app_length] [app name]
    cd $DYNAMIC_APPROXILYZER_SCRIPTS
    ./run_static_analysis_with_cmd_line.sh 1 1 1 1 1 $1

    # cleaning static analysis output
    # Usage: ./clean_up_analysis_output_with_cmd_line.sh [app_profile] [br_profile] [count_static_inst] [mode_info] [app name]
    cd $DYNAMIC_APPROXILYZER_SCRIPTS
    ./clean_up_analysis_output_with_cmd_line.sh 1 1 1 1 $1

    # edit app_txt_info with app_length information
    cd $DYNAMIC_APPROXILYZER_SCRIPTS
    ./update_app_txt_info.sh $1

fi


if [ $2 -eq "2" ]; then
############################## PHASE 2 of APPROXILYZER ############################## 
    # set EMERALD flags for Phase 2 (Dependence Analysis and DPC_LIST_AFTER)
    EMERALD=$GEMS_PATH/emerald/

    # make emerald
    echo -ne '\E[1;33;44m'"PHASE 2: DYNAMIC ANALYSIS"; tput sgr0
    echo
    cd $EMERALD
    make clean
    make module DESTINATION=dynamic_relyzer PHASE=2
 

    # running dynamic analysis

    # echo "Usage: ./run_dynamic_analysis_with_cmd_line.sh [run_heap_analysis] [run_stack_analysis] [run_control_patterns] [app name]"
    cd $DYNAMIC_APPROXILYZER_SCRIPTS
    ./run_dynamic_analysis_with_cmd_line.sh 1 1 1 $1

fi


if [ $2 -eq "3" ]; then
############################## PHASE 3 of APPROXILYZER ############################## 
    # Make sure correct emerald is set
    EMERALD=$GEMS_PATH/emerald/

    # make emerald
    echo -ne '\E[1;33;44m'"PHASE 3: GENERATING FAULT LIST"; tput sgr0
    echo
    cd $EMERALD
    make clean
    make module DESTINATION=dynamic_relyzer PHASE=2
   
    # symbolic links
    cd $DYNAMIC_APPROXILYZER_SCRIPTS
    ./gen_sym_links.sh $1

    # make perl-front end (edit program_list)
    cd $APPROXILYZER/scripts/static_relyzer/src
    make clean; make

    cd $APPROXILYZER/scripts/static_relyzer
 #   app_name="$(python $APPROXILYZER/scripts/dynamic_relyzer/get_app_name.py $1)"
    app_name=$1
    make perl-front-end  APP=$app_name 

    # run Relyzer source code
    prog_list="$1 $1"
    echo $prog_list
    echo "WRITTEN!"
    echo $prog_list > $RELYZER_APPS/program_list.txt
    $APPROXILYZER/scripts/static_relyzer/src/relyzer-front-end $RELYZER_APPS/program_list.txt

    # generate fault list
    $APPROXILYZER/scripts/static_relyzer/scripts/bash/gen_all_simics_files.sh $1

    # generate intermediate checkpoints
fi

if [ $2 -eq "4" ]; then
    #TODO CHECK IF EXISTANT AND DO NOT REDO
    cd $GEMS_PATH/simics/home/dynamic_relyzer/
    ./simics -stall $APPROXILYZER/workloads/apps/${1}/simics_files/${1}_create_checkpoints.simics
fi

echo "DONE!"

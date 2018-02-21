#!/bin/bash

# This is the main approxilyzer script, used to generate fault injections, run
# experiments, and analyze the output.
# checks to make sure file certain paths exist. Relyzer, GEMS, and SIMICS, Relyzer apps
if [ ! -d $APPROXILYZER ]; then
    echo "No APPROXILYZER directory. Add it to your .bashrc"
    exit 1
fi

SIMICS_DIR=$GEMS_PATH/simics/home/dynamic_relyzer/
CHECKPOINT_DIR=$APPROXILYZER/workloads/checkpoints/
RUN_SCRIPT_PATH=$CHECKPOINT_DIR/simics_files/

############################## MAKE AN ISO CHKPT ############################## 
if [[ $# -eq 2 && "$1" == "iso" ]]; then
    # see if checkpoint already exists
        # if yes, ask user if s/he wants to overwrite checkpoint
    if [ -f $CHECKPOINT_DIR/$2 ]; then
        echo "Checkpoint already exists at $CHECKPOINT_DIR/$2"
        exit 1
    fi


    # create simics file
    $APPROXILYZER/scripts/start_scripts/make_iso_chkpt.sh $2

    # runs simics file
    cd $SIMICS_DIR
    ./simics -stall -no-win $RUN_SCRIPT_PATH/make_iso_ckpt_${2}.simics

############################ MAKE AN APP RUN CHKPT ############################ 
elif [[ $# -eq 4 && "$1" == "run" ]]; then
    # see if checkpoint already exists
       # if yes, ask user if s/he wants to overwrite checkpoint
 
    if [ -f $CHECKPOINT_DIR/${3}_${4} ]; then
        echo "Checkpoint already exists at $CHECKPOINT_DIR/${3}_${4}"
        exit 1
    fi
    
    # create simics file
    $APPROXILYZER/scripts/start_scripts/make_app_chkpt.sh $2 $3 $4

    cd $SIMICS_DIR
    ./simics -stall -no-win $RUN_SCRIPT_PATH/make_app_run_ckpt_${3}_${4}.simics

   # runs simics file

##################################### HELP ####################################
else
    echo "Usage: ./mkchkpt.sh iso [iso_name]"
    echo "Sample use: ./mkchkpt.sh iso benchmarks"
    echo "OR"
    echo "Usage: ./mkchkpt.sh run [iso_name] [app_name] [run_script_name]"
    echo "Sample use: ./mkchkpt.sh run benchmarks blackscholes_simlarge run"
    exit 1
fi


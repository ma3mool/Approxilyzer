#!/bin/bash

# This is the main approxilyzer script, used to generate fault injections, run
# experiments, and analyze the output.


# checks to make sure file certain paths exist. Approxilyzer, GEMS, and SIMICS, Approxilyzer apps
ERROR=0;
if [ -d $APPROXILYZER ]; then
    let ERROR=0
else
    let ERROR=1
    echo "No Approxilyzer directory. Add it to your .bashrc"
    exit 1
fi

if [ -d $GEMS_PATH ]; then
    let ERROR=0
else
    let ERROR=1
    echo "No GEMS_PATH directory. Add it to your .bashrc"
    exit 1
fi

if [ -d $SIMICS_INSTALL ]; then
    let ERROR=0
else
    let ERROR=1
    echo "No SIMICS directory. Add it to your .bashrc"
    exit 1
fi

sflag=false
Iflag=false
cflag=false
rflag=false
iflag=false
vflag=false
Aflag=false
aflag=false


isoName=""
run_script_name=""
rValue=""
iValue=""
vValue=""
Avalue=""
app_name=""


print_usage () {
    echo "Usage: $0 [POSIX options]"
    echo "POSIX options:"
    echo -e "\t-s [prep]                                        Start Script"
    echo -e "\t-I iso_name                                      Base ISO chkpt"
    echo -e "\t-c run_script_name                               App chkpt"
    echo -e "\t-r [prof || anlys || fault_gen || int_ckpt]      Relyzer analysis"
    echo -e "\t-i [prep || results]                             Fault Injection"
    echo -e "\t-v [prep || results]                             Validation"
    echo -e "\t-A [approx 1 || approx 2]                        Approxilyzer analysis"
    echo -e "\t-a app_name                                      Application"
    echo -e "\t-h                                               Usage"
    exit 1
}



if [[ "$1" =~ ^((-{1,2})([Hh]$|[Hh][Ee][Ll][Pp])|)$ ]]; then
    print_usage; exit 1
else
    while [[ $# -gt 0 ]]; do
        opt="$1"
        shift;
        current_arg="$1"
        if [[ "$current_arg" =~ ^-{1,2}.* ]]; then
            echo "WARNING: You may have left an argument blank for $1. Double check your command." 
            exit 1
        fi
        case "$opt" in
            "-s"    )   sflag=true; shift ;;
            "-I"    )   Iflag=true; isoName=$current_arg; shift ;;
            "-c"    )   cflag=true; run_script_name=$current_arg; shift ;;
            "-r"    )   rflag=true; rValue=$current_arg; shift ;;
            "-i"    )   iflag=true; iValue=$current_arg; shift ;;
            "-v"    )   vflag=true; vValue=$current_arg; shift ;;
            "-A"    )   Aflag=true; AValue=$current_arg; shift ;;
            "-a"    )   aflag=true; app_name=$current_arg; shift ;;
            *       )   echo "ERROR: Invalid option: \""$opt"\"" >&2
                        exit 1;;
        esac
    done
fi

# start script
if  $sflag && !$Iflag && !$cflag && !$rflag && !$iflag && !$vflag && !$Aflag && !$aflag
then
# ensure directory structure is correct. create it otherwise
    echo "Preparing Approxilyzer!"
    $APPROXILYZER/scripts/start_scripts/dir_setup.sh
   
    # install emerald
    $APPROXILYZER/scripts/start_scripts/emerald_init.sh
    $GEMS_PATH/emerald/makesymlinks_emerald.sh
    EMERALD=$GEMS_PATH/emerald/
    cd $EMERALD
    make clean
    make module DESTINATION=dynamic_relyzer PHASE=1
    cd $GEMS_PATH/simics
    make emerald

    # local symlink of simpoint.py
    ln -s $APPROXILYZER/scripts/dynamic_relyzer/simpoint.py $GEMS_PATH/simics/home/dynamic_relyzer/simpoint.py

    exit 0

# make a new iso
elif  ! $sflag && $Iflag && ! $cflag && ! $rflag && ! $iflag && ! $vflag && ! $Aflag && ! $aflag
then
    echo "Making new ISO checkpoint for $isoName"
    $APPROXILYZER/scripts/primary_scripts/mkchkpt.sh iso $isoName
    exit 0

# make a new app checkpoint in the iso
elif  ! $sflag && $Iflag && $cflag && ! $rflag && ! $iflag && ! $vflag && ! $Aflag && $aflag
then
    echo "Making new app checkpoint for $app_name running script $run_script_name from $isoName"
    $APPROXILYZER/scripts/primary_scripts/mkchkpt.sh run $isoName $app_name $run_script_name
    exit 0

# run Relyzer script
elif  ! $sflag && ! $Iflag && ! $cflag && $rflag && ! $iflag && ! $vflag && ! $Aflag && $aflag
then
    echo "Running relyzer script for $app_name with $rValue"
    if [ "$rValue" == "prof" ]
    then
        $APPROXILYZER/scripts/primary_scripts/relyzer_fault_generation.sh $app_name 1
    elif [ "$rValue" == "anlys" ]
    then
        $APPROXILYZER/scripts/primary_scripts/relyzer_fault_generation.sh $app_name 2
    elif [ "$rValue" == "fault_gen" ]
    then
        $APPROXILYZER/scripts/primary_scripts/relyzer_fault_generation.sh $app_name 3
    elif [ "$rValue" == "int_ckpt" ]
    then
        $APPROXILYZER/scripts/primary_scripts/relyzer_fault_generation.sh $app_name 4
    else
        echo "Incorrect option passed to Relyzer analysis."
        print_usage
        exit 1
    fi
    exit 0
    
# run injection script
elif  ! $sflag && ! $Iflag && ! $cflag && ! $rflag && $iflag && ! $vflag && ! $Aflag && $aflag
then
    echo "Running injection script for $app_name with $iValue"
    if [ "$iValue" == "prep" ]
    then
        $APPROXILYZER/scripts/primary_scripts/relyzer_injections.sh $app_name 1
    elif [ "$iValue" == "results" ]
    then
        $APPROXILYZER/scripts/primary_scripts/relyzer_injections.sh $app_name 2
        $APPROXILYZER/scripts/primary_scripts/relyzer_injections.sh $app_name 3
    else
        echo "Incorrect option passed to injections scripts"
        print_usage
        exit 1
    fi

    exit 0

# run validation script
elif  ! $sflag && ! $Iflag && ! $cflag && ! $rflag && ! $iflag && $vflag && ! $Aflag && $aflag
then
    echo "Running validation script for $app_name with $vValue"
    if [ "$vValue" == "prep" ]
    then
        $APPROXILYZER/scripts/primary_scripts/relyzer_validation.sh $app_name 1
    elif [ "$vValue" == "results" ]
    then
        $APPROXILYZER/scripts/primary_scripts/relyzer_validation.sh $app_name 2
    else
        echo "Incorrect option passed to validation scripts"
        print_usage
        exit 1
    fi


    exit 0
# run Approxilyzer script
elif  ! $sflag && ! $Iflag && ! $cflag && ! $rflag && ! $iflag && ! $vflag && $Aflag && $aflag
then
    echo "Running Approxilyzer script for $app_name with $AValue"
    exit 0
else
    echo "Incorrect combination of options."
    print_usage
    exit 1
fi



INJECTION_SCRIPTS=$APPROXILYZER/scripts/injections_scripts
INJECTION_RESULTS=$APPROXILYZER/fault_list_output/injection_results


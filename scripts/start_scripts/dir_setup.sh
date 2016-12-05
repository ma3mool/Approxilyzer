##!/bin/bash

# This script generates the directory structure for Approxilyzer

# How to use this script
if [ $# -ne 0 ]; then
    echo "Usage: ./dir_setup.sh"
    exit 1
fi

# checks to make sure file certain paths exist. Relyzer, GEMS, and SIMICS, Relyzer apps
if [ -d $APPROXILYZER ]; then
    echo "Relyzer directory exists"
else
    echo "No Relyzer directory. Add it to your .bashrc"
    exit 1
fi


# makes directory structure for Relyzer
# need to have set up Relyzer path in .bashrc file
# Makes dirs that will be used: creates them if not available 

## dir structure for fault_list_output (to be changed to results)

# injection results
mkdir -p $APPROXILYZER/fault_list_output/injection_results/parallel_outcomes
mkdir -p $APPROXILYZER/fault_list_output/injection_results/parsed_results
mkdir -p $APPROXILYZER/fault_list_output/injection_results/raw_outcomes
mkdir -p $APPROXILYZER/fault_list_output/injection_results/reformatted_outcomes
mkdir -p $APPROXILYZER/fault_list_output/injection_results/full_sdc_list

# validation results
mkdir -p $APPROXILYZER/fault_list_output/validation_results/

# approxilyzer results
mkdir -p $APPROXILYZER/fault_list_output/approxilyzer_results/final_output_impact_list
mkdir -p $APPROXILYZER/fault_list_output/approxilyzer_results/pc_list
mkdir -p $APPROXILYZER/fault_list_output/approxilyzer_results/sdc_categorized/av_rel_err/
mkdir -p $APPROXILYZER/fault_list_output/approxilyzer_results/sdc_categorized/diff/
mkdir -p $APPROXILYZER/fault_list_output/approxilyzer_results/sdc_categorized/percent/
mkdir -p $APPROXILYZER/fault_list_output/approxilyzer_results/sdc_categorized/true_sdc/diff
mkdir -p $APPROXILYZER/fault_list_output/approxilyzer_results/sdc_categorized/true_sdc/av_rel_err
mkdir -p $APPROXILYZER/fault_list_output/approxilyzer_results/sdc_categorized/true_sdc/percent

# approxilyzer results
mkdir -p $APPROXILYZER/fault_list_output/approxilyzer_results/sdc_percent/av_rel_err/
mkdir -p $APPROXILYZER/fault_list_output/approxilyzer_results/sdc_percent/diff/
mkdir -p $APPROXILYZER/fault_list_output/approxilyzer_results/sdc_percent/percent/
mkdir -p $APPROXILYZER/fault_list_output/approxilyzer_results/sdc_percent/true_sdc/diff
mkdir -p $APPROXILYZER/fault_list_output/approxilyzer_results/sdc_percent/true_sdc/av_rel_err
mkdir -p $APPROXILYZER/fault_list_output/approxilyzer_results/sdc_percent/true_sdc/percent

# simics file list
mkdir -p $APPROXILYZER/fault_list_output/simics_file_list


## dir structure for workloads
mkdir -p $APPROXILYZER/workloads/apps
mkdir -p $APPROXILYZER/workloads/iso
mkdir -p $APPROXILYZER/workloads/checkpoints
mkdir -p $APPROXILYZER/workloads/golden_output



##      includes copying over simpoint.py TODO



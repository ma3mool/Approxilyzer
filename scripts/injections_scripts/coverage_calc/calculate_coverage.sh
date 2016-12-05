#!/bin/bash

# runs very similarly to gen_reformatted_output, with the intention of finding population

DIR=$APPROXILYZER/scripts/injections_scripts/coverage_calc
INPUT_DIR=$APPROXILYZER/fault_list_output/simics_file_list
OUTPUT_DIR=$APPROXILYZER/fault_list_output/coverage_calculation

if [ -d $DIR ]; then
    echo "Directory found"
else
    echo "$DIR not found"
    exit 1
fi

if [ -d $INPUT_DIR ]; then
    echo "Input directory found"
else
    echo "$INPUT_DIR not found"
    exit 1
fi

if [ -d $OUTPUT_DIR ]; then
    echo "Output directory found"
else
    echo "$OUTPUT_DIR not found"
    exit 1
fi

if [ $# -ne 2 ]; then
    echo "Usage: ./calculate_coverage.sh [app_name] [intended coverage]"
    echo "Sample use: ./calculate_coverage.sh blackscholes_simlarge 95"
    echo
    echo "Warning: You should still look into the script to insure correct paths"
    exit 1
fi

INPUT=${INPUT_DIR}/simics_file_list_${1}_all_50_${2}_1.txt
APP_NAME=$1

# feign injections from simics_file list
sed -e 's/$/::Masked/' ${INPUT} > $OUTPUT_DIR/raw_outcomes_feigned/${1}_${2}.outcomes_raw

# convert injections into correct format
echo "FORMATTING OUTCOMES"
$DIR/convert_outcome_format.sh $APP_NAME $2

# generate all_sdc_* et al
echo "RUNNING parsing_output.pl"
perl $DIR/parsing_output.pl $APP_NAME

# sum up population and print value
gawk -F: '{ if($3 == 1) sum += $1 }; END { print sum }' $OUTPUT_DIR/parsed_results/${1}_all_masked.txt


echo "REMEMBER TO COMPARE WITH THE :1 ONLY!"

echo "DONE!"




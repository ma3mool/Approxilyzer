#!/bin/bash

# convert from this format: $app_name.$pc.$adjusted_pilot.$c.$arch_reg.$i.$src_dest.$fault_type (line 665)
# to this format:           $app_name.pc$pc.p$adjusted_pilot.c$c.t$fault_type.b$i.s     .r$arch_reg.


INPUT_DIR=$RELYZER_SHARED/fault_list_output/coverage_calculation/raw_outcomes_feigned
OUTPUT_DIR=$RELYZER_SHARED/fault_list_output/coverage_calculation/reformatted_outcomes


if [ $# -ne 2 ]; then
    echo "Usage: ./convert_outcome_format.sh [input_file_name] [coverage target]"
    echo "Sample use: ./convert_outcome_format.sh blackscholes_simlarge 95"
    echo
    exit 1
fi

APP_NAME=$1


awk -F ":" '{ print $1 "." $3 ":" $4}' $INPUT_DIR/${APP_NAME}_${2}.outcomes_raw | awk -F "." '{ print $1 ".pc" $2 ".p" $3 ".c" $4 ".t" $8 ".b" $6 ".s4.r" $5 ".d" $7 ".c" $4 ":" $9 "." $10 "." $11 "." $12 "." $13 "." $14 "." $15 "." $16 "." $17 "." $18 "." $19 }' > $OUTPUT_DIR/$APP_NAME.reformatted



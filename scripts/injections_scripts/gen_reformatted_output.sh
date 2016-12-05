#!/bin/bash

# reformats output, creates *_all_sdcs.txt by invoking parsing_output.pl

DIR=$APPROXILYZER/scripts/injections_scripts

if [ -d $DIR ]; then
    echo "Directory found"
else
    echo "$DIR not found"
    exit 1
fi

if [ $# -ne 1 ]; then
    echo "Usage: ./gen_reformatted_output.sh [app_name] "
    echo "Sample use: ./gen_reformatted_output.sh blackscholes_simlarge"
    echo
    echo "Warning: You should still look into the script to insure correct paths"
    exit 1
fi


APP_NAME=$1

echo "FORMATTING OUTCOMES"
$DIR/convert_outcome_format.sh $APP_NAME

echo "RUNNING parsing_output.pl"
perl $DIR/parsing_output.pl $APP_NAME

echo "DONE!"




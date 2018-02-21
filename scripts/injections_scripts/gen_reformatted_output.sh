#!/bin/bash

# reformats output, creates *_all_sdcs.txt by invoking parsing_output.pl

DIR=$APPROXILYZER/scripts/injections_scripts

if [ -d $DIR ]; then
    echo "Directory found"
else
    echo "$DIR not found"
    exit 1
fi

if [ $# -lt 1 ] || [ $# -gt 2 ]; then
    echo "Usage: ./gen_reformatted_output.sh [app_name] (depth)"
    echo "Sample use: ./gen_reformatted_output.sh blackscholes_simlarge"
    echo
    echo "Warning: You should still look into the script to ensure correct paths"
    exit 1
fi


APP_NAME=$1

echo "FORMATTING OUTCOMES"
$DIR/convert_outcome_format.sh $APP_NAME

# if there is a depth parameter passed in, use that
echo "RUNNING parsing_output.pl"
if [ $# == 2 ]; then
    perl $DIR/parsing_output.pl $APP_NAME $2
else
    perl $DIR/parsing_output.pl $APP_NAME
fi
echo "DONE!"




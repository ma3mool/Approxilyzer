#!/bin/bash

DIR=$APPROXILYZER/fault_list_output/injection_results/parallel_outcomes

if [ $# -ne 1 ]; then
    echo "Usage: ./reset_outcomes.sh [app_name]"
    echo "Sample use: ./reset_outcomes.sh blackscholes_simlarge"
    echo
    exit 1
fi

APP_NAME=$1


if [ -f "$DIR/$APP_NAME-1.outcomes" ]; then
    echo "Removing old outcomes files"
    rm $DIR/$APP_NAME-*.outcomes
fi

if [ -f "$DIR/${APP_NAME}.retries" ]; then
    echo "Removing old retries file"
    rm $DIR/${APP_NAME}.retries
fi

echo "Creating new outcome files for $APP_NAME"

for i in `seq 1 625` ;
do
    touch $DIR/$APP_NAME-$i.outcomes
done


touch $DIR/$APP_NAME.retries
touch $DIR/errors.log

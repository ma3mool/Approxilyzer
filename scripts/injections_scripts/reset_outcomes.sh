#!/bin/bash

DIR=$RELYZER_SHARED/fault_list_output/injection_results/parallel_outcomes

if [ $# -ne 1 ]; then
    echo "Usage: ./reset_outcomes.sh [app_name]"
    echo "Sample use: ./reset_outcomes.sh blackscholes_simlarge"
    echo
    exit 1
fi

APP_NAME=$1


echo "Removing old files"
rm $DIR/$APP_NAME-*.outcomes
rm $DIR/${APP_NAME}.retries

echo "Creating new outcome files for $APP_NAME"

for i in `seq 1 625` ;
do
    touch $DIR/$APP_NAME-$i.outcomes
done


touch $DIR/$APP_NAME.retries
touch $DIR/errors.log

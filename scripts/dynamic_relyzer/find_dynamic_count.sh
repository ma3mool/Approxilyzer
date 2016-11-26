#!/bin/bash

APP=$1

DIR=$RELYZER_SHARED/workloads/apps/
OUTPUT=$DIR/$APP/$APP.dynamic

file1=$DIR/$APP/static_inst  # original job injection file list
file2=$DIR/$APP/$APP.prof  # output results for injection file list

size=($(wc $file1))

#for i in `seq 1 5`
while read -r line
do
#    line=($(sed '$iq;d' $file1))
    (
    injection=(${line//:/ })
    result=($(grep -m 1 ${injection[0]} $file2)) 

    if [ "$result" != "" ]; then
        echo "$result" >> $OUTPUT 
 #   else 
        #echo "$result:${injection[1]}:${injection[2]}" >> $OUTPUT
 #       echo "FOUND!"
    fi

    ) &
    NPROC=$(($NPROC+1))
    if [ "$NPROC" -ge 50 ]; then
        wait
        NPROC=0
    fi

done < "$file1"

echo "STATS!"
dyn_inst=($(gawk -F: '{ sum += $2 }; END { print sum }' $OUTPUT))

echo "Total Dynamic Instructions: $dyn_inst"



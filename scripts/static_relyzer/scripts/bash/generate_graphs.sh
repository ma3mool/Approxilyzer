#!/bin/bash


# export PATH=/home/sadve/shari2/santoor-local-disk/tools/local/bin/:$PATH

if [ ! -d "cfg" ]; then
	mkdir "cfg";
    # Control will enter here if $DIRECTORY doesn't exist.
fi

DOT=dot

#for app in $(echo blackscholes fluidanimate streamcluster swaptions fft lu ocean water gcc libquantum mcf omnetpp)
for app in $(echo streamcluster)
do

	if [ ! -d "cfg/$app" ]; then
		mkdir "cfg/$app";
	fi

	FILES="cfg/$app/*.dot";
	for f in $FILES
	do
		echo "Processing $f file..."
		$DOT $f -Tpdf -o${f/.dot/.pdf}
	done
done

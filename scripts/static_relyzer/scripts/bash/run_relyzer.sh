#!/bin/bash

rm -f full_table.txt full_distr.txt
#for app in $(echo blackscholes fluidanimate streamcluster swaptions fft lu ocean water gcc libquantum mcf omnetpp)
for app in $(echo blackscholes swaptions fft lu libquantum )
do
	echo "$app $app" > temp_file.txt;
	./src/relyzer-front-end temp_file.txt;
	cat table.txt >> full_table.txt;
	cat distr.txt >> full_distr.txt;

done

rm temp_file.txt;


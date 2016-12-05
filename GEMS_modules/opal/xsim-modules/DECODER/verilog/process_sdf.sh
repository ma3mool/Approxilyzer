#!/bin/bash
# Script to remove the posedge, negedge and resulting () from the
# decoder_unit_flag.SDF file. For other files, there maybe more signals
# with the posedge 

sed -i "s/posedge //g" $1 ;
sed -i "s/negedge //g" $1 ;
sed -i "s/(A1)/A1/g" $1 ;
sed -i "s/(A2)/A1/g" $1 ;
sed -i "s/(B)/B/g" $1 ;
sed -i "s/(SL)/SL/g" $1 ;

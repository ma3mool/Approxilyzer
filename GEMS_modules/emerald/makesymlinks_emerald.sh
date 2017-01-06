#!/bin/bash

echo "Making symlink for emerald module"
cd $GEMS_PATH/simics/modules
rm -rf emerald 
mkdir emerald 
cd emerald 
ln -s ../../../emerald/system/commands.h
ln -s ../../../emerald/module/commands.py
ln -s ../../../emerald/common/config.h
ln -s ../../../emerald/common/emerald_api.h
ln -s ../../../emerald/module/emerald.c
ln -s ../../../emerald/module/Makefile
cd ..


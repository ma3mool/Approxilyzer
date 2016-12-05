#!/bin/bash

echo "Making symlink for Emerald module in GEMS directory"

# checks to make sure file certain paths exist. Relyzer, GEMS, and SIMICS, Relyzer apps
if [ -d $APPROXILYZER ]; then
    echo "Relyzer directory exists"
else
    echo "No Relyzer directory. Add it to your .bashrc"
    exit 1
fi

if [ -d $GEMS_PATH ]; then
    echo "GEMS_PATH directory exists"
else
    echo "No GEMS_PATH directory. Add it to your .bashrc"
    exit 1
fi


dir=$GEMS_PATH/emerald
src=$APPROXILYZER/GEMS_modules/emerald


# checks if dir exists. Creates it if not, plus does symlinking
if [[ ! -e $dir ]]; then
    mkdir $dir
    ln -s $src/common $dir/
    ln -s $src/module $dir/
    ln -s $src/system $dir/
    ln -s $src/init.C $dir/
    ln -s $src/init.h $dir/
    ln -s $src/Makefile $dir/
    ln -s $src/makesymlinks_emerald.sh $dir/
fi



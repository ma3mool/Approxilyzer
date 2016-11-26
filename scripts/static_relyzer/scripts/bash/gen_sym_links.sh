#!/bin/bash

# creates symbolic links for the applications, to point to branch profile and app profile

RELYZER_APPS=/shared/workspace/amahmou2/relyzer/apps/ #directory to put sym links
STATIC_ANALYSIS=/shared/workspace/amahmou2/Relyzer_GEMS/src_GEMS/simics/home/dynamic_relyzer/abdul_fully_optimized_main_logs_new/


if [ -d $RELYZER_APPS ]; then
    echo "Relyzer Apps directory found"
else
    echo "$RELYZER_APPS not found"
    exit 1
fi


if [ -d $STATIC_ANALYSIS ]; then
    echo "Static Analysis directory found"
else
    echo "$STATIC_ANALYSIS not found"
    exit 1
fi

brProf="_opt_abdul_br_profile.txt"
appProf="_opt_abdul_profile.txt"

echo -ne '\E[1;33;44m'"PREPARING DYNAMIC RELYZER PHASE ..."; tput sgr0
echo

for app in $(echo blackscholes fft fluidanimate gcc libquantum lu mcf ocean streamcluster swaptions water )
#for app in $(echo blackscholes)
do
    cd $RELYZER_APPS/$app
    brFileName=$app$brProf
    appFileName=$app$appProf
    printf "[%12s] %s\n" $app "Creating symbolic links ..."
    ln -s $STATIC_ANALYSIS$brFileName $RELYZER_APPS/$app/$app.br_prof
    ln -s $STATIC_ANALYSIS$appFileName $RELYZER_APPS/$app/$app.prof
done



#!/bin/bash

# creates symbolic links for the applications, to point to branch profile and app profile


# How to use this script
if [ $# -ne 1 ]; then
    echo "Usage: ./gen_sym_links.sh [app_name]"
    echo "Sample use: ./gen_sym_links.sh blackscholes"
    exit 1
fi
### DIR STRUCTURE ###

DIR_0=$RELYZER_SHARED/workloads/apps/

if [[ ! -d $DIR_0/${1} ]]; then
    echo "No directory created for ${1} in workspace/app"
    exit 1
fi

if [[ ! -d $DIR_0/${1}/analysis_output ]]; then
    echo "analysis_output directory not found in $DIR_0/${1}"
    exit 1
fi

STATIC_ANALYSIS=$DIR_0/${1}/analysis_output/
RELYZER_APPS=$DIR_0/

#####################

brProf="_br_profile.txt"
appProf="_profile.txt"

echo -ne '\E[1;33;44m'"PREPARING DYNAMIC RELYZER PHASE ..."; tput sgr0
echo

app_name=$1

echo $app_name

for app in $(echo $app_name)
do
    cd $RELYZER_APPS/$app
    brFileName=$app$brProf
    appFileName=$app$appProf
    printf "[%12s] %s\n" $app "Creating symbolic links ..."

    # remove old symlink if it exists

    if [[ -L $RELYZER_APPS/${app}/${app}.br_prof ]]; then
        rm $RELYZER_APPS/$app/$app.br_prof
    fi
    if [[ -L $RELYZER_APPS/${app}/${app}.prof ]]; then
        rm $RELYZER_APPS/$app/$app.prof
    fi

    # create new symlink
    ln -s $STATIC_ANALYSIS$brFileName $RELYZER_APPS/$app/$app.br_prof
    ln -s $STATIC_ANALYSIS$appFileName $RELYZER_APPS/$app/$app.prof
done



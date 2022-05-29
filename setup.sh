#! /bin/bash

function checkout()
{
    directory=$1
    branch=$2
    if [[ -d "../$directory" ]]
    then
        echo $directory
        pushd ../$directory > /dev/null
        git pull
        git checkout $branch
        git pull
        popd > /dev/null
    else
        echo $directory
        url=$( git remote -v | grep fetch | awk '{print $2}' | sed -e s/casual.git/${directory}.git/g )
        echo $url
        pushd .. > /dev/null
        git clone $url
        pushd $directory > /dev/null
        git checkout $branch
        popd > /dev/null
        popd > /dev/null
    fi

}

MAKE_PATH="casual-make"
MAKE_BRANCH="master"
THIRDPARTY_PATH="casual-thirdparty"
THIRDPARTY_BRANCH="master"

checkout $MAKE_PATH $MAKE_BRANCH
checkout $THIRDPARTY_PATH $THIRDPARTY_BRANCH

TEMPLATE_ENV_FILE="middleware/example/env/casual.env"
ENV_FILE="casual.env"
if [[ ! -f $ENV_FILE ]]
then
    echo "copying $TEMPLATE_ENV_FILE to $ENV_FILE"
    echo "review and adapt file"
    cp $TEMPLATE_ENV_FILE $ENV_FILE
else
    echo "$ENV_FILE already exists"
    if ! diff $ENV_FILE $TEMPLATE_ENV_FILE > /dev/null
    then
        echo "consider updating from $TEMPLATE_ENV_FILE"
    fi
fi
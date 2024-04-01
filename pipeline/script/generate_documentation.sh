#! /bin/bash

SCRIPTDIR=$( dirname $0)
BUILDDIR=$1

releases="release/1.6 patch/1.6/main feature/1.7/main"

cd ${BUILDDIR}

CURRENT_BRANCH=$(git rev-parse --abbrev-ref HEAD)

for release in $releases
do
    echo $release
    git checkout $release
    git pull
done

git checkout ${CURRENT_BRANCH}
git pull

export CASUAL_VERSION="Unused"
export CASUAL_RELEASE="Unused"

${SCRIPTDIR}/sphinx-multiversion.py  . build/html

cp -r build/html/* ../casualcore.github.io/docs/.

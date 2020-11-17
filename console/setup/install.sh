#!/bin/bash

echo 'Copying console files'
python $CASUAL_BUILD_HOME/console/setup/install_console.py

type pip
whereis pip


echo 'Running pip install'
cd ${CASUAL_HOME}/console/server && pip install --no-cache-dir --target=${CASUAL_HOME}/console/server/lib -r ${CASUAL_HOME}/console/server/requirements.txt

echo 'Console installed'
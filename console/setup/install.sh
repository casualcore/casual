#!/bin/bash

echo 'Copying console files'
python $CASUAL_BUILD_HOME/console/setup/install_console.py

echo 'Running pip install'
python -m pip install --no-cache-dir --target=${CASUAL_HOME}/console/server/lib -r ${CASUAL_HOME}/console/server/requirements.txt

echo 'Console installed'
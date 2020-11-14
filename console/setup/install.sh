#!/bin/sh

echo 'Copying console files'
python $CASUAL_BUILD_HOME/console/setup/install_console.py

echo 'Running pip install'
pip install --no-cache-dir --target=/opt/casual/console/server/lib -r /opt/casual/console/server/requirements.txt

echo 'Console installed'

#
# Usage: python install_nginx.py
#

import urllib.request
import os
import tarfile
import subprocess

from shutil import copyfile

URL="http://nginx.org/download/"
FILENAME="nginx-1.18.0.tar.gz"
TMP="/tmp/"

if not os.getenv("CASUAL_MAKE_SOURCE_ROOT") or not os.getenv("CASUAL_HOME"):
    raise SystemError("CASUAL-environment need to be set")

BASENAME=os.path.splitext(os.path.splitext(FILENAME)[0])[0]

if not os.path.exists(TMP + BASENAME):
    print("Fetching: " + URL + FILENAME)
    with urllib.request.urlopen( URL + FILENAME) as response:
        with open(TMP + FILENAME, "wb") as file:
            file.write( response.read())
    
    archive = tarfile.open( TMP + FILENAME, "r:gz")
    archive.extractall(TMP)

os.chdir(TMP + BASENAME)

prefix = os.getenv('CASUAL_HOME', '/usr/local/casual') + '/nginx'

print("Start setting up: " + BASENAME)
print("Running configure")
print( subprocess.check_output(['./configure',
'--with-debug',
'--with-cc-opt=-Wno-deprecated',
'--prefix=' + prefix,
'--add-module=' + os.getenv('CASUAL_MAKE_SOURCE_ROOT') + '/middleware/plugin',
'--without-http_rewrite_module']))
print("Running make")
print( subprocess.check_output(['make']))
print("Running install")
print( subprocess.check_output(['make', 'install']))
print("Updating configuration")
copyfile(os.getenv("CASUAL_MAKE_SOURCE_ROOT") + '/thirdparty/nginx/nginx.conf', prefix + '/conf/nginx.conf') 
print("Done")

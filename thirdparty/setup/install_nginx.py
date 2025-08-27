
#
# Usage: python install_nginx.py
#

import os
import tarfile
import subprocess

from shutil import copyfile

NGINX_VERSION = "1.28.0"
BASENAME = "nginx-" + NGINX_VERSION
SOURCE_ROOT = os.getenv("CASUAL_MAKE_SOURCE_ROOT")
CASUAL_THIRDPARTY = os.getenv("CASUAL_THIRDPARTY")

if not SOURCE_ROOT or not CASUAL_THIRDPARTY:
	raise SystemError("CASUAL_MAKE_SOURCE_ROOT and CASUAL_THIRDPARTY need to be set")

os.chdir(CASUAL_THIRDPARTY + '/nginx/' + BASENAME)

prefix = os.getenv('CASUAL_HOME', '/opt/casual') + '/nginx'

print("Start setting up: " + BASENAME)

print("Running configure")
print( subprocess.check_output(['./configure',
   '--with-debug',
   '--prefix=' + prefix,
   '--with-cc-opt=-Wno-deprecated',
   '--add-module=' + SOURCE_ROOT + '/middleware/http/source/inbound/nginx/plugin',
   '--without-http_rewrite_module']).decode())
print("Running make")
print( subprocess.check_output(['make']).decode())
print("Running install")
print( subprocess.check_output(['make', 'install']).decode())
print("Updating configuration")
copyfile( SOURCE_ROOT + '/thirdparty/nginx/nginx.conf', prefix + '/conf/nginx.conf') 
print("Done")

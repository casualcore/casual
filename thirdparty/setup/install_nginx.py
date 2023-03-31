
#
# Usage: python install_nginx.py
#

import os
import subprocess

from shutil import copyfile

NGNIX_VERSION = "nginx-1.22.1"
SOURCE_ROOT = os.getenv( "CASUAL_MAKE_SOURCE_ROOT", os.getenv("CASUAL_BUILD_HOME"))
NGINX_ROOT = SOURCE_ROOT + '/../casual-thirdparty/nginx/' + NGNIX_VERSION + '/'

if not SOURCE_ROOT or not os.getenv("CASUAL_HOME"):
	raise SystemError("CASUAL-environment need to be set")

if not os.path.exists(NGINX_ROOT):
   raise SystemError("NGINX-version " + NGINX_VERSION + " not found!")

prefix = os.getenv('CASUAL_HOME', '/usr/local/casual') + '/nginx'

os.chdir( NGINX_ROOT)
print("Start setting up: " + NGINX_ROOT)
print("Running configure")
print( subprocess.check_output(['./configure',
'--with-debug',
'--with-cc-opt=-Wno-deprecated',
'--prefix=' + prefix,
'--add-module=' + SOURCE_ROOT + '/middleware/http/source/inbound/nginx/plugin',
'--without-http_rewrite_module']).decode())
print("Running make")
print( subprocess.check_output(['make']).decode())
print("Running install")
print( subprocess.check_output(['make', 'install']).decode())
print("Updating configuration")
copyfile( SOURCE_ROOT + '/thirdparty/nginx/nginx.conf', prefix + '/conf/nginx.conf') 
print("Done")

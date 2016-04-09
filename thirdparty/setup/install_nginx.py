
#
# Usage: python install_nginx.py
#

import urllib2
import os
import tarfile
import subprocess


URL="http://nginx.org/download/"
FILENAME="nginx-1.8.1.tar.gz"
TMP="/tmp/"

if not os.getenv("CASUAL_BUILD_HOME") or not os.getenv("CASUAL_HOME"):
	raise SystemError, "CASUAL-environment need to be set"

BASENAME=os.path.splitext(os.path.splitext(FILENAME)[0])[0]

if not os.path.exists(TMP + BASENAME):
	print("Fetching: " + URL + FILENAME)
	response = urllib2.urlopen(URL + FILENAME)

	with open(TMP + FILENAME, "wb") as f:
		f.write(response.read())
	
	archive = tarfile.open(TMP + FILENAME, "r:gz")
	archive.extractall(TMP)

os.chdir(TMP + BASENAME)

print("Start setting up: " + BASENAME)
print("Running configure")
print( subprocess.check_output(['./configure',
'--with-debug',
'--with-cc-opt=-Wno-deprecated',
'--prefix=' + os.getenv('CASUAL_HOME', '/usr/local/casual') + '/nginx' ,
'--add-module=' + os.getenv('CASUAL_BUILD_HOME') + '/plugin',
'--without-http_rewrite_module']))
print("Running make")
print( subprocess.check_output(['make']))
print("Running install")
print( subprocess.check_output(['make', 'install']))
	

	

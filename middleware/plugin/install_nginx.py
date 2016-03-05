import urllib2
import os
import tarfile
import subprocess

class cd(object):
    """Context manager for changing the current working directory"""
    def __init__(self, newPath):
        self.newPath = os.path.expanduser(newPath)

    def __enter__(self):
        self.savedPath = os.getcwd()
        os.chdir(self.newPath)

    def __exit__(self, etype, value, traceback):
        os.chdir(self.savedPath)


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

with cd(TMP + BASENAME):
	print("Start setting up: " + BASENAME)
	print("Running configure")
	print( subprocess.check_output(['./configure', 
	'--with-cc-opt=-Wno-deprecated',
	'--prefix=' + os.getenv('CASUAL_HOME', '/usr/local/') + '/nginx' ,
	'--add-module=' + os.getenv('CASUAL_BUILD_HOME') + '/plugin',
	'--without-http_rewrite_module']))
	print("Running make")
	print( subprocess.check_output(['make']))
	print("Running install")
	print( subprocess.check_output(['make', 'install']))
	

	
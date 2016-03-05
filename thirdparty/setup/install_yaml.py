 
# Usage: python install_yaml.py
#

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


URL="https://github.com/jbeder/yaml-cpp/archive/"
FILENAME="release-0.3.0.tar.gz"
TMP="/tmp/"

BASENAME="release-0.3.0"
BUILDDIR="yaml-cpp-release-0.3.0/build"

if not os.path.exists(TMP + BASENAME):
    print("Fetching: " + URL + FILENAME)
    response = urllib2.urlopen(URL + FILENAME)

    with open(TMP + FILENAME, "wb") as f:
        f.write(response.read())
    
    archive = tarfile.open(TMP + FILENAME, "r:gz")
    archive.extractall(TMP)
    os.mkdir(TMP + BUILDDIR)

with cd(TMP + BUILDDIR):
    print("Start setting up: " + BASENAME)
    print("Running cmake")
    print( subprocess.check_output(['cmake', '-DBUILD_SHARED_LIBS=ON', '..' ]))
    print("Running make")
    print( subprocess.check_output(['make']))
    print("Running install")
    print( subprocess.check_output(['make', 'install']))
 
# Usage: python install_yaml.py
#

import urllib2
import os
import tarfile
import subprocess
import platform


URL="https://github.com/jbeder/yaml-cpp/archive/"
FILENAME="release-0.3.0.tar.gz"
TMP="/tmp/"

BASENAME="release-0.3.0"
BUILDDIR="yaml-cpp-release-0.3.0/build"

if not os.path.exists(TMP + BUILDDIR):
    print("Fetching: " + URL + FILENAME)
    response = urllib2.urlopen(URL + FILENAME)

    with open(TMP + FILENAME, "wb") as f:
        f.write(response.read())
    
    archive = tarfile.open(TMP + FILENAME, "r:gz")
    archive.extractall(TMP)
    os.mkdir(TMP + BUILDDIR)

os.chdir(TMP + BUILDDIR)

print("Start setting up: " + BASENAME)
print("Running cmake")
if platform.system() == 'Darwin':
    
    if os.uname()[ 2] < '16.3.0':
        print( subprocess.check_output(['cmake', '-DBUILD_SHARED_LIBS=OFF', '-DCMAKE_CXX_FLAGS="-stdlib=libstdc++"', '..' ]))
    else:
        print( subprocess.check_output(['cmake', '-DBUILD_SHARED_LIBS=OFF', '-DCMAKE_CXX_FLAGS=-std=c++11', '..' ]))
else:	
    print( subprocess.check_output(['cmake', '-DBUILD_SHARED_LIBS=ON', '..' ]))

print("Running make")
print( subprocess.check_output(['make']))
print("Running install")
print( subprocess.check_output(['make', 'install']))

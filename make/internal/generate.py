#! /usr/bin/env python

'''
Created on 28 apr 2012

@author: hbergk
'''

#
# Imports
#
import sys
import os.path

#from casual.make.functiondefinitions import Compile
from casual.make.internal.engine import engine

        
def casual_make( casual_makefile):
    
    if not os.path.isfile( casual_makefile):
        sys.stderr.write( casual_makefile + ": No such file\n")
        sys.exit(1)
    
    #
    # Start up the engine
    #
    engine().run( casual_makefile)
    
    
if __name__ == '__main__':
    if len(sys.argv) < 2:
        casual_makefile = "makefile.cmk"
    else:
        casual_makefile = sys.argv[1]
        
    casual_make( casual_makefile)

#! /usr/bin/python

'''
Created on 28 apr 2012

@author: hbergk
'''

#
# Imports
#
import sys
import os.path

from casual.make.engine import Engine

class Casual_Make( object):
    """Responsible for handling of producing makefiles from casual_make-files"""
    
    def __init__(self, casual_makefile):
        '''
        Constructor
        '''
        if not os.path.isfile( casual_makefile):
            sys.stderr.write( casual_makefile + ": No such file\n")
            sys.exit(1)
        self.casual_makefile = casual_makefile
    
    def run(self):
        #
        # Start up the engine
        #
        Engine( self.casual_makefile).run()
    
if __name__ == '__main__':
    if len(sys.argv) < 2:
        casual_makefile = "makefile.cmk"
    else:
        casual_makefile = sys.argv[1]
        
    Casual_Make( casual_makefile).run()

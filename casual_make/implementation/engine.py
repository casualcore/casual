'''
Created on 28 apr 2012

@author: hbergk

This is the module running the "process"
'''

#
# Imports
#
# Standard
#
import re
import sys
import traceback
#
# Project
#
from configuration import Configuration
from parser import Parser
from implementation.functiondefinitions import *
from implementation.internal import debug
from implementation.preprocess import *


class Engine(object):
    '''
    Responsible for running the process
    '''

    
    def __init__(self, casual_makefile):
        '''
        Constructor
        '''
        #
        # Select configuration
        #
        self.configuration = Configuration()
        #
        # Select makefile
        #
        self.casual_makefile = casual_makefile
        self.makefile = open( os.path.splitext(casual_makefile)[0] + ".mk","w")
        
    def run(self):
        debug("Engine running...")
        
        try:
            data = Preprocess( self.casual_makefile).process()
            debug(data)
        except:
            sys.stderr.write("Error preprocessing " + self.casual_makefile + '\n')
            sys.exit(2)
        #
        # Set up parser
        #
        self.parser = Parser( data)
        #
        # Start normalizing 
        #
        self.parser.normalize()
        #
        #
        #
        
        #
        # Start by writing CASUALMAKE_PATH
        #
        self.makefile.write( "CASUALMAKE_PATH = " + os.path.dirname(os.path.abspath(sys.argv[0])) + "/..\n")
        #
        # Print out includes in makefile
        #
        if self.parser.includes:
            for include in self.parser.includes: 
                self.makefile.write( "include " + include + "\n")
        #
        # Print out all detected defines in makefile
        #
        for define in self.parser.defines: 
            self.makefile.write( define + "\n")
        #
        # Print out all configuration content into makefile
        #
        self.makefile.write(self.configuration.content)
        debug( self.parser.content)
        #
        # Turn stdout over to makefile
        #
        sys.stdout=self.makefile
        #
        # Traverse the parsed content and execute statements
        #
        lines=re.split(r'\n', self.parser.content)
        for command in lines:
            try:
                #
                # Excecute command and catch possible errors
                #
                if command.strip():
                    debug( command)
                    exec command
            except:
                documentation=""
                searchMethod = re.search(r'(\w+)[ \t]*\(', command)
                if searchMethod:
                    method = searchMethod.group(1)
                    try:
                        documentation = eval(method).__doc__
                        debug( method)
                    except:
                        #
                        # No documentation means probably no method
                        #
                        traceback.print_exc(0) 
                        sys.exit(2)
                        
                sys.stderr.write( command + '\n' + documentation + '\n')
                traceback.print_exc(0) 
                sys.exit(2)
        
        #
        # Reset stdout
        #
        sys.stdout = sys.__stdout__
        debug( "Engine done.")
        
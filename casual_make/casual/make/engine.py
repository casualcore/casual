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

import tempfile
import string
#
# Project
#
from casual.make.configuration import Configuration
from casual.make.functiondefinitions import *
from casual.make.internal import debug

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
        self.internalCasual_makefile = tempfile.NamedTemporaryFile(delete=False)
        
        #
        # Need to add statement for internal_post_make_rules
        #
        casual_makefile_file = open(self.casual_makefile,'r')
        self.internalCasual_makefile.write( string.join( casual_makefile_file.readlines(), ''))
        self.internalCasual_makefile.write( '\n' + 'internal_post_make_rules()\n')
        casual_makefile_file.close()
        self.internalCasual_makefile.close()
        
        self.makefile = open( os.path.splitext(casual_makefile)[0] + ".mk","w")
        
    def __del__(self):
        '''
                Destructor
        '''
        os.unlink(self.internalCasual_makefile.name)
        
    def run(self):
        debug("Engine running...")
                
        #
        # Create temporary file
        #
        tempMakefile = tempfile.NamedTemporaryFile(delete=False)
        #
        # Print out all configuration content into makefile
        #
        tempMakefile.write(self.configuration.content)
#       debug( self.parser.content)
        #
        # Turn stdout over to makefile
        #
        sys.stdout=tempMakefile
        
        globalVariables= {}
        localVariables= {}
        try:
            execfile( self.internalCasual_makefile.name, globalVariables, localVariables )
        except (NameError, SyntaxError, TypeError):
            sys.stderr.write( "Error in " + os.path.realpath(self.casual_makefile) + ".\n")
            raise
            
        #
        # Reset stdout
        #
        sys.stdout = sys.__stdout__
                
        #
        # Start by writing CASUALMAKE_PATH
        #
        self.makefile.write( "CASUALMAKE_PATH = " + os.path.dirname(os.path.abspath(sys.argv[0])) + u"/..\n")
        
        if 'include' in localVariables:
            for entry in localVariables['include']:
                self.makefile.write( 'include ' +  entry + '\n')

        if 'export' in localVariables:
            for key in localVariables['export']:
                self.makefile.write(  key + '=' + ' '.join( localVariables['export'][key]) + '\n')
                    
        tempMakefile.close()
        
        output = open(tempMakefile.name, 'r')
        
        self.makefile.write( string.join( output.readlines(), ''))
        
        os.unlink(output.name)  
        
        debug( "Engine done.")
        
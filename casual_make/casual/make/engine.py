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
import StringIO
#
# Project
#
from casual.make.configuration import configuration
from casual.make.functiondefinitions import *
from casual.make.internal import debug
#from _pyio import StringIO

class Engine(object):
    '''
    Responsible for running the process
    '''

    
    def __init__(self, casual_makefile):
        '''
        Constructor
        '''

        self.casual_makefile = casual_makefile        
        self.makefile = os.path.splitext( casual_makefile)[0] + ".mk";
        
        
        
    def __del__(self):
        '''
                Destructor
        '''

        
    def prepareCasualMakeFile(self):
        
        with open( self.casual_makefile,'r') as origin:
        
            cmk = StringIO.StringIO();
            
            cmk.write( 'from casual.make.functiondefinitions import *')
            
            cmk.write( '\n' + 'internal_pre_make_rules()\n');
            
                   
            #
            # add platform specific configuration
            #
            with configuration() as config:
                for line in config:
                    cmk.write( line);
            
            #
            # make sure we call, and generate, the platform configuration
            #
            cmk.write( '\n' + 'casual_make_platform_dynamic()\n');
            
            #
            # write user casual-make-file
            #
            for line in origin:
                cmk.write( line);    
            
            cmk.write( '\n' + 'internal_post_make_rules()\n')
                        
        
            
            
        return cmk
        
        
    def run(self):
        debug("Engine running...")
                
        # 
        # Keep the output in memory
        #
        output = StringIO.StringIO();
        
        
#       debug( self.parser.content)
        #
        # Turn stdout over to output
        #
        sys.stdout = output;
        
        
        globalVariables= {}
        
        try:
            cmk = self.prepareCasualMakeFile()
            
            code = compile( cmk.getvalue(), self.casual_makefile, 'exec')
            cmk.close();
            
            exec( code, globalVariables)
            
        except (NameError, SyntaxError, TypeError):
            sys.stderr.write( "Error in " + os.path.realpath(self.casual_makefile) + ".\n")
            raise
            
        #
        # Reset stdout
        #
        sys.stdout = sys.__stdout__
        output.seek( 0);
        
        
        

        #
        # Create temporary file, so we can have 'atomic' behavior
        #
        with open( self.makefile + '.tmp', 'w+') as temp:    
        
            #
            # Start by writing CASUALMAKE_PATH
            #
            temp.write( "CASUALMAKE_PATH = " + os.path.dirname(os.path.abspath(sys.argv[0])) + u"/..\n")
            temp.write( "USER_CASUAL_MAKE_FILE = " + self.casual_makefile + "\n");
            
            #
            # Write pre-make-statements. i.e include statements, INCLUDE_PATHS, and such
            #
            if 'internal_globalPreMakeStatements' in globalVariables:
                for statement in globalVariables['internal_globalPreMakeStatements']:
                    temp.write( statement + '\n')
            
            #
            # Write the makefile-output
            #
            for line in output:
                temp.write( line);
            
        
            #
            # "swap" the files
            #
            os.rename( temp.name, self.makefile);
 
        
        debug( "Engine done.")
        
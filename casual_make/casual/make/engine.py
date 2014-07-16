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
from casual.make.configuration import Configuration
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
            cmk.write( string.join( origin.readlines(), ''));
            cmk.write( '\n' + 'internal_post_make_rules()\n')
            
        
        return cmk
        
        
    def run(self):
        debug("Engine running...")
        
        
            
        cmk = self.prepareCasualMakeFile();
        
                
        #
        # Create temporary file
        #
        temp = open( self.makefile + '.tmp', 'w+');
        
        #sys.stderr.write( 'tempfile: ' + temp.name + '\n');
        
        #
        # Print out all configuration content into makefile
        #
        temp.write( Configuration().content);
        
        #
        # Start by writing CASUALMAKE_PATH
        #
        temp.write( "CASUALMAKE_PATH = " + os.path.dirname(os.path.abspath(sys.argv[0])) + u"/..\n")
        temp.write( "USER_CASUAL_MAKE_FILE = " + self.casual_makefile + "\n");
        
#       debug( self.parser.content)
        #
        # Turn stdout over to makefile
        #
        sys.stdout = temp;
        
        
        try:
        
            code = compile( cmk.getvalue(), self.casual_makefile, 'exec')
            exec( code)
            
        except (NameError, SyntaxError, TypeError):
            sys.stderr.write( "Error in " + os.path.realpath(self.casual_makefile) + ".\n")
            raise
            
        #
        # Reset stdout
        #
        sys.stdout = sys.__stdout__
            
                    
        temp.close();
        
        os.rename( temp.name, self.makefile);
 
        
        debug( "Engine done.")
        
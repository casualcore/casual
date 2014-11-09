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
import sys
import os
#
# Project
#

import path

#from casual.make.functiondefinitions import *
#from casual.make.internal import debug
#from _pyio import StringIO

class Engine(object):
    '''
    Responsible for running the process
    '''

    
    def __init__(self):
        '''
        Constructor
        '''        
        self.test = 0
        
        self.output = StringIO.StringIO();
        sys.stdout = self.output;
        
        
        self.output_stack = []
        self.output_done = []
        
        self.casual_make_file = None
        self.identifier = 0
        
        
        
    def __del__(self):
        '''
                Destructor
        '''

    def scopeBegin(self, name = None):
        
        if not name:
            self.identifier += 1
            name = 'scope_' + str( self.identifier)
        
        output = StringIO.StringIO();
        sys.stdout = output;
        
        scoped_make_file =  self.makefiles_path + '/' + self.makefile_stem + '_' + name +'.mk';
        
        self.output_stack.append( [ output, scoped_make_file]);
        
        return self.casual_make_file, scoped_make_file
        
    
    def scopeEnd(self):    
    
        self.output_done.append( self.output_stack.pop())
        
        if self.output_stack:
            output = self.output_stack[ -1]
            sys.stdout = output[ 0];
        else:
            sys.stdout = self.output
            
        
    
    def prepareCasualMakeFile( self, casual_makefile):
        
        with open( casual_makefile,'r') as origin:
        
            cmk = StringIO.StringIO();
            
            cmk.write( 'from casual.make.directive import *')
            
            cmk.write( '\n' + 'internal_pre_make_rules()\n');
            
            
            #
            # write user casual-make-file
            #
            for line in origin:
                cmk.write( line);    
            
            cmk.write( '\n' + 'internal_post_make_rules()\n')
                        
        
            
            
        return cmk
        
    def write(self, stream, filename, pre_make_statements):
        
        stream.seek( 0);
        
        with open( filename, 'w+') as makefile:    
        
            #
            # Start by writing CASUALMAKE_PATH
            #
            makefile.write( "CASUALMAKE_PATH = " + os.path.dirname( os.path.abspath(sys.argv[0] + u"/..")) + '\n')
            makefile.write( "USER_CASUAL_MAKE_FILE = " + self.casual_make_file + '\n');
            
            #
            # Write pre-make-statements. i.e include statements, INCLUDE_PATHS, and such
            #
            for statement in pre_make_statements:
                makefile.write( statement + '\n')
            
            #
            # Write the makefile-output
            #
            for line in stream:
                makefile.write( line);
                
    def prepareOutputs(self, casual_makefile):
        
        self.makefiles_path = path.makepath( casual_makefile)
        
        self.makefile_stem = path.makestem( casual_makefile) 
        
        #
        # Make sure the path exists
        #
        if not os.path.exists( self.makefiles_path):
            os.makedirs( self.makefiles_path);

        
        
    def run(self, casual_makefile):
        
        self.casual_make_file = os.path.abspath( casual_makefile)
        self.prepareOutputs( self.casual_make_file)
        
        globalVariables= {}
        
        try:
            cmk = self.prepareCasualMakeFile( casual_makefile)
            
            code = compile( cmk.getvalue(), casual_makefile, 'exec')
            cmk.close();
            
            exec( code, globalVariables)
            
        except (NameError, SyntaxError, TypeError):
            sys.stderr.write( "Error in " + os.path.realpath( casual_makefile) + ".\n")
            raise
            
        #
        # Reset stdout
        #
        sys.stdout = sys.__stdout__
        
        
        makefile = self.makefiles_path + '/' + self.makefile_stem + '.mk';

        pre_make_statements = globalVariables[ 'internal_global_pre_make_statement_stack']
            
            
        self.write( self.output, makefile, pre_make_statements[ 0])
        
 
        #
        # Take care of scopes (if any)
        #
        index = 1
        
        for output, filename in self.output_done:
            
            self.write( output, filename, pre_make_statements[ index])
            index += 1
                        
            
        
        #debug( "Engine done.")



__global_engine = None

def engine():
    
    global __global_engine
    
    if not __global_engine:
        __global_engine = Engine()
    
    return __global_engine





        
'''
Created on 13 maj 2012

@author: hbergk
'''

import os
import sys
import re
from contextlib import contextmanager

import casual.make.platform.platform
from casual.make.internal.engine import engine

import casual.make.internal.directive as internal
import casual.make.internal.path as path

_platform = casual.make.platform.platform.platform()

        



#
# Some global defines
#


#
# Set up casual_make-program. Normally casual_make
#
def_casual_make='casual-make-generate'

def_Deploy='make.deploy.ksh'


global_build_targets = [ 
          'link',
          'cross', 
          'clean_exe',
          'clean_object',
          'clean_dependency', 
          'compile', 
          'deploy', 
          'install',
          'test', 
          'print_include_paths' ];

global_targets = [ 
          'make', 
          'clean',
           ] + global_build_targets;





 

global_pre_make_statement_stack = []




class State:
    
    def __init__(self, old_state = None):
        
        self.parallel_make = None
        self.object_paths_to_clean = set()
        self.files_to_Remove=set()
        self.paths_to_create = set()
        
        self.pre_make_statements = list()
        
        
        self.make_files_to_build = list()
        
        if old_state:
            self.pre_make_statements = list( old_state.pre_make_statements) 
             
        global global_pre_make_statement_stack
        
        global_pre_make_statement_stack.append( self.pre_make_statements)
        
        
        
    

global_current_state = State()

def state():
    return global_current_state

@contextmanager
def scope( inherit, name):
    global global_current_state
    
    if name:
        name  = internal.normalize_string( name)
    
    old_state = global_current_state;
    
    current_makefile_state = None
    
    if inherit:
        global_current_state = State( old_state)
    else:
        global_current_state = State();
    
    try:
        current_makefile_state = engine().scopeBegin( name);
        
        pre_make_rules()
        yield
        
    finally:
        post_make_rules()
        
        engine().scopeEnd();
        global_current_state = old_state;
       
        #
        # We have to invoke the "scoped makefile"
        #
        state().make_files_to_build.append( [ True, current_makefile_state[ 0], current_makefile_state[ 1]])
        
        



def add_pre_make_statement( statement):
    
    state().pre_make_statements.append( statement)
     

#
# Normalize name 
#
def clean_directory_name(name):
    return str.replace(name, './', '')

#
# replace / to _
#
def convert_path_to_target_name(name):
    return 'target_' + str.replace( name, '/', '_' )


def pre_make_rules():
    #
    # Default targets and such
    #
    
    print
    print '#'
    print '# If no target is given we assume \'all\''
    print '#'
    print 'all:'
    
    print
    print '#'
    print '# Dummy targets to make sure they will run, even if there is a corresponding file'
    print '# with the same name'
    print '#'
    for target in global_targets:
        print '.PHONY ' + target + ':' 
    
    
    #
    # Platform specific prolog
    #
    _platform.pre_make();
    


def produce_build_targets():
  
    if state().make_files_to_build:
       
        casual_build_targets = list()
        casual_make_targets = list()
       
        for scoped_build, casual_make_file, make_file in state().make_files_to_build:

            casual_make_directory = os.path.dirname( casual_make_file)
            
            target_name  = unique_target_name( make_file)
            
            build_target_name = 'build_' + target_name
            
            
            make_target_name = 'make_' + target_name
            
            
            if not scoped_build:
                
                print
                print "#"
                print '# targets to handle recursive stuff for ' + casual_make_file
                print '#'
                print make_file + ": " + casual_make_file
                print "\t@echo generates makefile from " + casual_make_file
                print "\t@" + _platform.change_directory( casual_make_directory) + ' && ' + def_casual_make + " " + casual_make_file
            
                print 
                print build_target_name + ": " + make_file
                print "\t@echo " + casual_make_file + '  $(MAKECMDGOALS)'
                print '\t@$(MAKE) -C "' + casual_make_directory + '" $(MAKECMDGOALS) -f ' + make_file

                casual_build_targets.append( build_target_name);
                        
                print
                print make_target_name + ":"
                print "\t@echo generates makefile from " + casual_make_file
                print "\t@" + _platform.change_directory( casual_make_directory) + ' && ' + def_casual_make + " " + casual_make_file
                print '\t@$(MAKE) -C "' + casual_make_directory + '" $(MAKECMDGOALS) -f ' + make_file

                casual_make_targets.append( make_target_name)
                
            else:
            
                print
                print '#'
                print '# targets to handle scope build for ' + casual_make_file
                print '#'
                print build_target_name + ':' 
                print '\t@$(MAKE) $(MAKECMDGOALS) -f ' + make_file
                
                casual_build_targets.append( build_target_name);
                casual_make_targets.append( build_target_name);
                
                


            print
        
        casual_build_targets_name = unique_target_name( 'casual_build_targets')
        
        print
        print '#'
        print '# set up dependences to build-targets'
        print '#'
        print casual_build_targets_name + ' = ' + internal.multiline( casual_build_targets)
        print 
        
        for target in global_build_targets:
            print target + ': $(' + casual_build_targets_name + ')'
       
        casual_make_targets_name = unique_target_name( 'casual_make_targets')
        
        print 
        print casual_make_targets_name + ' = ' + internal.multiline( casual_make_targets)
        print
        print 'make: $(' + casual_make_targets_name + ')' 
        print
         

#
# colled by engine after casual-make-file has been parsed.
#
# Hence, this function can use accumulated information.
#
def post_make_rules():

    #
    # Platform specific epilogue
    #
    _platform.post_make();

    print
    print '#'
    print '# Make sure recursive makefiles get the linker'
    print '#'
    print 'export EXECUTABLE_LINKER'
    
    print
    print '#'
    print '# de facto target \"all\"'
    print '#'
    print 'all: link'
    print
    
 
    if state().parallel_make is None or state().parallel_make:
        print
        print '#'
        print '# This makefile will run in parallel by default'
        print '# but, sequential processing can be forced'
        print '#'
        print 'ifdef FORCE_NOTPARALLEL'
        print '.NOTPARALLEL:'
        print 'endif'
        print
    else:   
        print
        print '#'
        print '# This makefile will be processed sequential'
        print '# but, parallel processing can be forced' 
        print '#'
        print 'ifdef FORCE_NOTPARALLEL'
        print '.NOTPARALLEL:'
        print 'endif'
        print 'ifndef FORCE_PARALLEL'
        print '.NOTPARALLEL:'
        print 'endif'
        print
    
    
    produce_build_targets()
       
    
       
    #
    # Targets for creating directories
    #
    for path in state().paths_to_create:
        print
        print path + ":"        
        print "\t" + _platform.make_directory( path)
        print

    
    #
    # Target for clean
    #
    
    
    print
    print "clean: clean_object clean_dependency clean_exe"
    
    print "clean_object:"
    for objectpath in state().object_paths_to_clean:
        print "\t-" + _platform.remove( objectpath + "/*.o")
        
    print "clean_dependency:"
    for objectpath in state().object_paths_to_clean:  
        print "\t-" + _platform.remove( objectpath + "/*.d")
    
    #
    # Remove all other known files.
    #
    print "clean_exe:"
    for filename in state().files_to_Remove:
        print "\t-" + _platform.remove( filename)

#
# Registration of files that will be removed with clean
#

def register_object_path_for_clean( objectpath):
    ''' '''
    state().object_paths_to_clean.add( objectpath)


def register_file_for_clean( filename):
    ''' '''
    state().files_to_Remove.add( filename)


def register_path_for_create( path):
    ''' '''
    state().paths_to_create.add( path)





#
# Name and path's helpers
#


def normalize_path(path, platformSpefic = None):

    path = os.path.abspath( path)
    if platformSpefic:
        return os.path.dirname( path) + '/' + platformSpefic( os.path.basename( path))
    else:
        return path



def shared_library_name_path( path):

    return normalize_path( path, _platform.library_name)


def archive_name_path( path):

    return normalize_path( path, _platform.archive_name)


def executable_name_path( path):

    return normalize_path( path, _platform.executable_name)


def bind_name_path( path):

    return normalize_path( path, _platform.bind_name)


def unique_target_name(name):

    unique_target_name.targetSequence += 1
    return internal.target_name( name) + "_" + str( unique_target_name.targetSequence)

unique_target_name.targetSequence = 0

def object_name_list( objects):
    
    internal.validate_list( objects)
    
    result = list()
    
    for obj in objects:
        result.append( os.path.abspath( obj)) 

    return result


def normalize_string( string):
    return internal.normalize_string(string)

def target(filename, source = '', name = None):
    
    return internal.Target( filename, source, name)


def cross_object_name(name):

    #
    # Ta bort '.o' och lagg till _crosscompile.o
    #
    return normalize_path( name.replace( '.o', '_crosscompile.o' ))



def dependency_file_name(objectfile):
    
    return objectfile.replace(".o", "") + ".d"

def set_ld_path():
    
    if not set_ld_path.is_set:
        print "#"
        print "# Set LD_LIBRARY_PATH so that unittest has access to dependent libraries"
        print "#"
        print "space :=  "
        print "space +=  "
        print "formattet_library_path = $(subst -L,,$(subst $(space),:,$(LIBRARY_PATHS) $(DEFAULT_LIBRARY_PATHS)))"
        print "LOCAL_LD_LIBRARY_PATH=$(formattet_library_path):$(PLATFORMLIB_DIR)"
        print 
        #
        # Make sure we don't set it again int this makefile
        #
        set_ld_path.is_set = True


set_ld_path.is_set = False;
    

def deploy( target, directive):
    
    targetname = 'deploy_' + target.name
    
    print
    print 'deploy: ' + targetname
    print
    print targetname + ": " + target.name
    print "\t-@" + def_Deploy + " " + target.file + ' ' + directive
    print


def build( casual_make_file):
    
    casual_make_file = os.path.abspath( casual_make_file)
    
    state().make_files_to_build.append( [ False, casual_make_file, path.makefile( casual_make_file)])
    

def library_targets( libs):

    targets = []
    dummy_targets = []
    
    if not libs:
        return targets
    
    internal.validate_list( libs)
    
    #
    # empty targets to make it work
    #
        
    for lib in libs:
        if not isinstance( lib, internal.Target):
            library = internal.target_name( _platform.library_name( lib))
            archive = internal.target_name( _platform.archive_name( lib))
            
            targets.append( library)
            targets.append( archive)
            
            dummy_targets.append( library)
            dummy_targets.append( archive)
                      
        else:
            targets.append( lib.name)


    print "#"
    print "# This is the only way I've got it to work. We got to have a INTERMEDIATE target"
    print "# even if the target exist and have dependency to a file that is older. Don't really get make..." 
    print "#";
    for target in targets:    
        print ".INTERMEDIATE: " + target
    print
    print "#";
    print "# dummy targets, that will be used if the real target is absent";
    for target in dummy_targets:    
        print target + ":"
    
    return targets

def link( operation, target, objectfiles, libraries, linkdirectives = '', prefix = ''):

    internal.validate_list( objectfiles);
    internal.validate_list( libraries);
    
    
    
    print "#"
    print "# Links: " + os.path.basename( target.file)
    
    
    #
    # extract file if some is targets
    # 
    objectfiles = internal.target_files( objectfiles)
    
             
    dependent_targets = library_targets( libraries)

    #
    # Convert library targets to names/files, 
    #
    libraries = internal.target_base( libraries)

    destination_path = os.path.dirname( target.file)
    
    print
    print "link: " + target.name
    print 
    print target.name + ': ' + target.file
    print
    print '   objects_' + target.name + ' = ' + internal.multiline( object_name_list( objectfiles))
    print
    print '   libs_'  + target.name + ' = ' + internal.multiline( _platform.link_directive( libraries))
    print
    print '   #'
    print '   # possible dependencies to other targets (in this makefile)'
    print '   depenency_' + target.name + ' = ' + internal.multiline( dependent_targets)
    print 
    print target.file + ': $(objects_' + target.name + ') $(depenency_' + target.name + ')' + " $(USER_CASUAL_MAKE_FILE) | " + destination_path
    print '\t' + prefix + operation( target.file, '$(objects_' + target.name + ')', '$(libs_' + target.name + ')', linkdirectives)
    print
    
    
    register_file_for_clean( target.file)
    register_path_for_create( destination_path)
    
    return target


def link_resource_proxy( target, resource, libraries, directive):
    
    internal.validate_list( libraries);
    
    dependent_targets = library_targets( libraries)
    
    #
    # Convert library targets to names/files, 
    #
    libraries = internal.target_base( libraries)
    
    destination_path = os.path.dirname( target.file)
    
    build_resource_proxy = _platform.executable_name( 'casual-build-resource-proxy')
    
    directive += ' ' + _platform.link_directive( libraries)
    
    
     
    
    print
    print "link: " + target.name
    print 
    print target.name + ': ' + target.file
    print
    print '   #'
    print '   # possible dependencies to other targets (in this makefile)'
    print '   depenency_' + target.name + ' = ' + internal.multiline( dependent_targets)
    print
    print target.file + ': $(depenency_' + target.name + ')' + " $(USER_CASUAL_MAKE_FILE) | " + destination_path
    print '\t' + build_resource_proxy + ' --output ' + target.file + ' --resource-key ' + resource + ' --link-directives "' + directive + ' $(INCLUDE_PATHS) $(DEFAULT_INCLUDE_PATHS) $(LIBRARY_PATHS) $(DEFAULT_LIBRARY_PATHS) $(DEFAULT_LIBS) $(LINK_DIRECTIVES_EXE)"'
    
    register_file_for_clean( target.file)
    register_path_for_create( destination_path)

    return target

def install(target, destination):
    
    if isinstance( target, list):
        for t in target:
            install( t, destination)
    
    else:
        target.name = 'install_' + target.name
        
        register_path_for_create( destination);
        
        print 'install: '+ target.name
        print
        print target.name + ": " + target.file + ' | ' + destination
        print "\t" + _platform.install( target.file, destination)
        print
        
        return target;

    



def set_parallel_make( value):
    global parallel_make;
    
    # we only do stuff if we haven't set parallel before...
    if state().parallel_make is None:        
        state().parallel_make = value;
    
def platform():
    return _platform 

def debug( message):
    if os.getenv('PYTHONDEBUG'): sys.stderr.write( message + '\n')

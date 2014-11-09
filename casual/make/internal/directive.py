'''
Created on 13 maj 2012

@author: hbergk
'''

import os
import sys
import re
from contextlib import contextmanager

from casual.make.platform.factory import factory
from engine import engine

import path



#
# Defines the return from "all" functions, that can be used
# in other function, as Install or whatnot...
# 
class Target:
    
    def __init__(self, filename, source = '', name = None):
        self.file = filename
        if name:
            self.name = name
        else:
            self.name = internal_target_name( filename)
        self.source = source
        self.base = os.path.basename( source);
        



#
# Some global defines
#


#
# Set up casual_make-program. Normally casual_make
#
def_casual_make='casual-make-generate'

def_Deploy='make.deploy.ksh'









def internal_platform():
    return factory();


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





 

internal_global_pre_make_statement_stack = []




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
             
        global internal_global_pre_make_statement_stack
        
        internal_global_pre_make_statement_stack.append( self.pre_make_statements)
        
        
        
    

global_current_state = State()

def state():
    return global_current_state




@contextmanager
def internal_scope( inherit, name):
    global global_current_state
    
    if name:
        name  = internal_normalize_string( name)
    
    old_state = global_current_state;
    
    current_makefile_state = None
    
    if inherit:
        global_current_state = State( old_state)
    else:
        global_current_state = State();
    
    try:
        current_makefile_state = engine().scopeBegin( name);
        
        internal_pre_make_rules()
        yield
        
    finally:
        internal_post_make_rules()
        
        engine().scopeEnd();
        global_current_state = old_state;
       
        #
        # We have to invoke the "scoped makefile"
        #
        state().make_files_to_build.append( [ True, current_makefile_state[ 0], current_makefile_state[ 1]])
        
        



def internal_add_pre_make_statement( statement):
    
    state().pre_make_statements.append( statement)
     

#
# Normalize name 
#
def internal_clean_directory_name(name):
    return str.replace(name, './', '')

#
# replace / to _
#
def internal_convert_path_to_target_name(name):
    return 'target_' + str.replace( name, '/', '_' )


def internal_pre_make_rules():
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
    internal_platform().pre_make();
    


def internal_produce_build_targets():
  
  if state().make_files_to_build:
       
        casual_build_targets = list()
        casual_make_targets = list()
       
        for scoped_build, casual_make_file, make_file in state().make_files_to_build:

            casual_make_directory = os.path.dirname( casual_make_file)
            
            target_name  = internal_unique_target_name( make_file)
            
            build_target_name = 'build_' + target_name
            
            
            make_target_name = 'make_' + target_name
            
            
            if not scoped_build:
                
                print
                print "#"
                print '# targets to handle recursive stuff for ' + casual_make_file
                print '#'
                print make_file + ": " + casual_make_file
                print "\t@echo generates makefile from " + casual_make_file
                print "\t@" + internal_platform().change_directory( casual_make_directory) + ' && ' + def_casual_make + " " + casual_make_file
            
                print 
                print build_target_name + ": " + make_file
                print "\t@echo " + casual_make_file + '  $(MAKECMDGOALS)'
                print '\t@$(MAKE) -C "' + casual_make_directory + '" $(MAKECMDGOALS) -f ' + make_file

                casual_build_targets.append( build_target_name);
                        
                print
                print make_target_name + ":"
                print "\t@echo generates makefile from " + casual_make_file
                print "\t@" + internal_platform().change_directory( casual_make_directory) + ' && ' + def_casual_make + " " + casual_make_file
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
        
        casual_build_targets_name = internal_unique_target_name( 'casual_build_targets')
        
        print
        print '#'
        print '# set up dependences to build-targets'
        print '#'
        print casual_build_targets_name + ' = ' + internal_multiline( casual_build_targets)
        print 
        
        for target in global_build_targets:
            print target + ': $(' + casual_build_targets_name + ')'
       
        casual_make_targets_name = internal_unique_target_name( 'casual_make_targets')
        
        print 
        print casual_make_targets_name + ' = ' + internal_multiline( casual_make_targets)
        print
        print 'make: $(' + casual_make_targets_name + ')' 
        print
         

#
# colled by engine after casual-make-file has been parsed.
#
# Hence, this function can use accumulated information.
#
def internal_post_make_rules():

    #
    # Platform specific epilogue
    #
    internal_platform().post_make();

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
    
    
    internal_produce_build_targets()
       
    
       
    #
    # Targets for creating directories
    #
    for path in state().paths_to_create:
        print
        print path + ":"        
        print "\t" + internal_platform().make_directory( path)
        print

    
    #
    # Target for clean
    #
    
    
    print
    print "clean: clean_object clean_dependency clean_exe"
    
    print "clean_object:"
    for objectpath in state().object_paths_to_clean:
        print "\t-" + internal_platform().remove( objectpath + "/*.o")
        
    print "clean_dependency:"
    for objectpath in state().object_paths_to_clean:  
        print "\t-" + internal_platform().remove( objectpath + "/*.d")
    
    #
    # Remove all other known files.
    #
    print "clean_exe:"
    for filename in state().files_to_Remove:
        print "\t-" + internal_platform().remove( filename)


#
# Registration of files that will be removed with clean
#

def internal_register_object_path_for_clean( objectpath):
    ''' '''
    state().object_paths_to_clean.add( objectpath)


def internal_register_file_for_clean( filename):
    ''' '''
    state().files_to_Remove.add( filename)


def internal_register_path_for_create( path):
    ''' '''
    state().paths_to_create.add( path)





#
# Name and path's helpers
#

def internal_normalize_string( string):
    return re.sub( '[^\w]+', '_', string)

def internal_normalize_path(path, platformSpefic = None):

    path = os.path.abspath( path)
    if platformSpefic:
        return os.path.dirname( path) + '/' + platformSpefic( os.path.basename( path))
    else:
        return path



def internal_shared_library_name_path( path):

    return internal_normalize_path( path, internal_platform().library_name)


def internal_archive_name_path( path):

    return internal_normalize_path( path, internal_platform().archive_name)


def internal_executable_name_path( path):

    return internal_normalize_path( path, internal_platform().executable_name)


def internal_bind_name_path( path):

    return internal_normalize_path( path, internal_platform().bind_name)



def internal_target_name( name):

    return 'target_' + internal_normalize_string( os.path.basename( name))


def internal_unique_target_name(name):

    internal_unique_target_name.targetSequence += 1
    return internal_target_name( name) + "_" + str( internal_unique_target_name.targetSequence)

internal_unique_target_name.targetSequence = 0


def internal_multiline( values):
    if isinstance( values, basestring):
        values = values.split()

    return ' \\\n      '.join( values)


def internal_object_name_list( objects):
    
    internal_validate_list( objects)
    
    result = list()
    
    for obj in objects:
        result.append( os.path.abspath( obj)) 

    return result






def internal_cross_object_name(name):

    #
    # Ta bort '.o' och lagg till _crosscompile.o
    #
    return internal_normalize_path( name.replace( '.o', '_crosscompile.o' ))



def internal_dependency_file_name(objectfile):
    
    return objectfile.replace(".o", "") + ".d"

def internal_set_ld_path():
    
    if not internal_set_ld_path.is_set:
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
        internal_set_ld_path.is_set = True


internal_set_ld_path.is_set = False;



def internal_validate_list( list):
    
    if isinstance( list, basestring):
        raise SyntaxError( 'not a list - content: ' + list);
    




def internal_deploy( target, directive):
    
    targetname = 'deploy_' + target.name
    
    print
    print 'deploy: ' + targetname
    print
    print targetname + ": " + target.name
    print "\t-@" + def_Deploy + " " + target.file + ' ' + directive
    print


def internal_build( casual_make_file):
    
    casual_make_file = os.path.abspath( casual_make_file)
    
    state().make_files_to_build.append( [ False, casual_make_file, path.makefile( casual_make_file)])
    

def internal_library_targets( libs):

    targets = []
    dummy_targets = []
    
    if not libs:
        return targets
    
    internal_validate_list( libs)
    
    #
    # empty targets to make it work
    #
    
    platform = internal_platform();
    
    
    
    for lib in libs:
        if not isinstance( lib, Target):
            library = internal_target_name( platform.library_name( lib))
            archive = internal_target_name( platform.archive_name( lib))
            
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


def internal_target_files( values):
    names = []
    for value in values:
        if isinstance( value, Target):
            names.append( value.file)
        else:
            names.append( value)
    
    return names;

def internal_target_base( values):
    names = []
    for value in values:
        if isinstance( value, Target):
            names.append( value.base)
        else:
            names.append( value)
    
    return names;



def internal_link( platform, target, objectfiles, libraries, linkdirectives = '', prefix = ''):

    internal_validate_list( objectfiles);
    internal_validate_list( libraries);
    
    
    
    print "#"
    print "# Links: " + os.path.basename( target.file)
    
    
    #
    # extract file if some is targets
    # 
    objectfiles = internal_target_files( objectfiles)
    
             
    dependent_targets = internal_library_targets( libraries)

    #
    # Convert library targets to names/files, 
    #
    libraries = internal_target_base( libraries)

    destination_path = os.path.dirname( target.file)
    
    print
    print "link: " + target.name
    print 
    print target.name + ': ' + target.file
    print
    print '   objects_' + target.name + ' = ' + internal_multiline( internal_object_name_list( objectfiles))
    print
    print '   libs_'  + target.name + ' = ' + internal_multiline( factory().link_directive( libraries))
    print
    print '   #'
    print '   # possible dependencies to other targets (in this makefile)'
    print '   depenency_' + target.name + ' = ' + internal_multiline( dependent_targets)
    print 
    print target.file + ': $(objects_' + target.name + ') $(depenency_' + target.name + ')' + " $(USER_CASUAL_MAKE_FILE) | " + destination_path
    print '\t' + prefix + platform( target.file, '$(objects_' + target.name + ')', '$(libs_' + target.name + ')', linkdirectives)
    print
    
    
    internal_register_file_for_clean( target.file)
    internal_register_path_for_create( destination_path)
    
    return target


def internal_link_resource_proxy( target, resource, libraries, directive):
    
    internal_validate_list( libraries);
    
    dependent_targets = internal_library_targets( libraries)
    
    #
    # Convert library targets to names/files, 
    #
    libraries = internal_target_base( libraries)
    
    destination_path = os.path.dirname( target.file)
    
    build_resource_proxy = internal_platform().executable_name( 'casual-build-resource-proxy')
    
    directive += ' ' + factory().link_directive( libraries)
    
    
     
    
    print
    print "link: " + target.name
    print 
    print target.name + ': ' + target.file
    print
    print '   #'
    print '   # possible dependencies to other targets (in this makefile)'
    print '   depenency_' + target.name + ' = ' + internal_multiline( dependent_targets)
    print
    print target.file + ': $(depenency_' + target.name + ')' + " $(USER_CASUAL_MAKE_FILE) | " + destination_path
    print '\t' + build_resource_proxy + ' --output ' + target.file + ' --resource-key ' + resource + ' --link-directives "' + directive + ' $(INCLUDE_PATHS) $(DEFAULT_INCLUDE_PATHS) $(LIBRARY_PATHS) $(DEFAULT_LIBRARY_PATHS) $(DEFAULT_LIBS) $(LINK_DIRECTIVES_EXE)"'
    
    internal_register_file_for_clean( target.file)
    internal_register_path_for_create( destination_path)

    return target

def internal_install(target, destination):
    
    if isinstance( target, list):
        for t in target:
            internal_install( t, destination)
    
    else:
        target.name = 'install_' + target.name
        
        internal_register_path_for_create( destination);
        
        print 'install: '+ target.name
        print
        print target.name + ": " + target.file + ' | ' + destination
        print "\t" + internal_platform().install( target.file, destination)
        print
        
        return target;

    



def internal_set_parallel_make( value):
    global parallel_make;
    
    # we only do stuff if we haven't set parallel before...
    if state().parallel_make is None:        
        state().parallel_make = value;
    





def debug( message):
    if os.getenv('PYTHONDEBUG'): sys.stderr.write( message + '\n')

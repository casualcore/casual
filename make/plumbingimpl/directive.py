'''
Created on 13 maj 2012

@author: hbergk
'''

import logging
logging.basicConfig( filename='.casual/cmk.log',level=logging.DEBUG)


import os
import sys
import re
import copy
from contextlib import contextmanager

import casual.make.platform.platform
from casual.make.internal.engine import engine

import casual.make.internal.directive as internal
import casual.make.internal.path as path

from casual.make.output import Output

_platform = casual.make.platform.platform.platform()

        



#
# Some global defines
#


#
# Set up casual_make-program. Normally casual_make
#
def_casual_make='casual-make-generate'


global_build_targets = [ 
          'link',
          'cross', 
          'clean_exe',
          'clean_object',
          'clean_dependency', 
          'compile', 
          'install',
          'test', 
          'print_include_paths' ];

global_targets = [ 
          'all',
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
        
        self.post_actions = dict()
        
        self.targets = dict()
        
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


def add_comment( comments, indent = ''):
    if( comments):
        print( indent + '#')
        print( indent + '# ' + ( '\n' + indent + '# ').join( comments.splitlines()))
        print( indent + '#')


def add_local_variable( dependencies, name = 'dependencies', comments = None):
    
    add_comment( comments, '  ')
    
    variable_name = unique_target_name( name)

    print( '  ' + variable_name + ' = ' + '\\\n     '.join( dependencies))
    print( '')
    
    return variable_name

def add_dependency( targets, prerequisites = [], comments = None, prefix = None, ordered_prerequisites = None):
    
    if( not targets):
        return
    
    add_comment(comments)
    
    
    prereq = ''
    
    if( len( prerequisites) > 3):
        prereq = '$(' + add_local_variable( internal.targets_name( prerequisites)) + ')'
    else:
        prereq = ' '.join( internal.targets_name( prerequisites))
    
    if( ordered_prerequisites):
         prereq += ' | ' + ' '.join( internal.targets_name( ordered_prerequisites))
    
    for target in internal.targets_name( targets):
        print( ( prefix or '') +  target + ': ' + prereq)

    print( '\n')


def add_includes( includes):
    
    if( isinstance( includes, basestring)):
        print( '-include ' + includes)
    else:
        for include in includes:
            print( '-include ' + include)
      

def add_rule( targets, prerequisites = None, recipes = None, ordered_prerequisites = None, comments = None):
  
        
    add_comment( comments)
    
    
    if( isinstance( targets, list) and len( targets) > 1 and prerequisites and recipes):
        return __add_multi_target_rule( targets, prerequisites, recipes, ordered_prerequisites)
    
    prereq = ' '.join( prerequisites or [])
    
    if( ordered_prerequisites):
        prereq += ' | ' + ' '.join( ordered_prerequisites)
        
    if( not prereq):
        if( isinstance( targets, list)):
            print( '.PNONY: ' + ' '.join( targets));  
        else:
             print( '.PNONY: ' + str( targets)); 
    
    if( isinstance( targets, list)):
        target = ' '.join( targets)
    else:
        target = targets
        

    print( target + ': ' + prereq) 
    
    for recipe in recipes:
       print( '\t' + str( recipe)); 
         
    print( '')


def add_sequential_ordering( targets, parents = None, comments = None):
    '''
    
    ''' 
    add_comment( comments)
    
    if( parents):
        
        prefix = 'dependency_'
        
        #
        # We only want sequentual ordering for some parents, we 
        # need to construct surregate names for the targets
        #
        for target, prerequisit in internal.pairwise( targets):
            add_dependency( targets = [ prefix + target], 
                            prerequisites = [ target, prefix + prerequisit])
        
        add_dependency( targets = [ prefix + targets[ -1]], 
                    prerequisites = [ targets[ -1]])
        
        add_dependency( targets = parents, 
                            prerequisites = [ prefix + targets[ 0]])
    
    else:
        
        for target, prerequisit in internal.pairwise( targets):
            add_dependency( targets = [target], 
                            ordered_prerequisites = [prerequisit])
    
    
    
    
def add_environment( name, value = '', export = True):
    if( export):
        print 'export ' + name + ' := ' + value
    else:
        print name + ' := ' + value
        


            
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
    #add_dependency( [ 'all'], [], 'If no target is given we assume \'all\'')
    
    
    add_dependency( targets = global_targets, 
                    prefix='.PHONY ', 
                    comments = ("Dummy targets to make sure they will run, even if there is a\n" 
                                "corresponding file with the same name"))
    
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
                
                add_rule( comments='targets to handle recursive stuff for ' + casual_make_file,
                          targets=make_file, 
                          prerequisites = [ casual_make_file], 
                          recipes=[
                              '@echo generates makefile from ' + casual_make_file,
                              '@' + _platform.change_directory( casual_make_directory) + ' && ' + def_casual_make + " " + casual_make_file
                          ])

                add_rule( targets = build_target_name, 
                          prerequisites = [ make_file], 
                          recipes=[
                              '@echo ' + casual_make_file + '  $(MAKECMDGOALS)',
                              '@$(MAKE) -C "' + casual_make_directory + '" $(MAKECMDGOALS) -f ' + make_file
                          ])

                casual_build_targets.append( build_target_name);

                add_rule( targets = make_target_name, 
                          recipes=[
                              '@echo generates makefile from ' + casual_make_file,
                              '@' + _platform.change_directory( casual_make_directory) + ' && ' + def_casual_make + " " + casual_make_file,
                              '@$(MAKE) -C "' + casual_make_directory + '" $(MAKECMDGOALS) -f ' + make_file
                          ])

                casual_make_targets.append( make_target_name)
                
            else:
                add_rule( comments='targets to handle scope build for ' + casual_make_file,
                          targets = build_target_name, 
                          recipes=[
                              '@$(MAKE) $(MAKECMDGOALS) -f ' + make_file
                          ])
                
                
                casual_build_targets.append( build_target_name);
                casual_make_targets.append( build_target_name);
                
            print
        
        add_sequential_ordering( casual_build_targets, [ 'test'], 'force sequential ordering for test')

        add_dependency( global_build_targets, casual_build_targets, 'set up dependences to build-targets');
         
        add_dependency( [ 'make'], casual_make_targets, 'set up dependences to make-targets');
         


def __post_actions():
    
    for key, action in state().post_actions.items():
        print( '\n# ' + key)
        action()



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
    
    add_dependency( [ 'all'], [ 'link'], 'de facto target \"all\"')
    
 
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
       
    print( '#\n# postactions \n#')  
    __post_actions()
       
    #
    # Targets for creating directories
    #
    for path in state().paths_to_create:
        add_rule( targets=path, recipes= [ _platform.make_directory( path)])
    
    #
    # Targets for clean
    #
    
    add_dependency( ['clean'], [ 'clean_object', 'clean_dependency', 'clean_exe'])
    
    add_rule( targets='clean_object', 
              recipes= map(lambda p: _platform.remove( p + "/*.o" ), [p for p in state().object_paths_to_clean]))

    add_rule( targets='clean_dependency', 
          recipes= map(lambda p: _platform.remove( p + "/*.d" ), [p for p in state().object_paths_to_clean]))
    
    add_rule( targets='clean_exe', 
              recipes= map(  _platform.remove, state().files_to_Remove))

#
# Registration of files that will be removed with clean
#

def register_target( category, target):
    ''' '''
    state().targets.setdefault( category, []).append( target)

def targets( category):
    return state().targets.get( category, [])


def register_post_action( name, action):
    state().post_actions.setdefault( name, action)


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

    path = os.path.abspath( extract_path(path))
    extracted_path = extract_path(path)
    if platformSpefic:
        return os.path.dirname(extracted_path) + '/' + platformSpefic( os.path.basename( extracted_path))
    else:
        return extracted_path

def extract_path( output):
    
    if isinstance( output, basestring):
        return output
    elif isinstance( output, Output):
        return output.name
    else:
        raise SystemError, "Unknown output type"

def shared_library_name_path( path):

    if isinstance( path, Output):
        path.file = normalize_path( path.name, _platform.library_name)
        return path
    else:
        return normalize_path( path, _platform.library_name)


def archive_name_path( path):

    return normalize_path( path, _platform.archive_name)


def executable_name_path( path):

    return normalize_path( path, _platform.executable_name)


def bind_name_path( path):

    return normalize_path( path, _platform.bind_name)

def unique_name(name):

    unique_name.sequence += 1
    return name + "_" + str( unique_name.sequence)

unique_name.sequence = 0

def unique_target_name(name):
    return unique_name( internal.target_name( name))


def object_name_list( objects):
    
    internal.validate_list( objects)
    
    result = list()
    
    for obj in objects:
        result.append( os.path.abspath( obj)) 

    return result


def normalize_string( string):
    return internal.normalize_string(string)

def target(output, source = '', name = None, operation = None):
    
    return internal.Target( output, source, name, operation)



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
    


def build( casual_make_file):
    
    casual_make_file = os.path.abspath( casual_make_file)
    
    state().make_files_to_build.append( [ False, casual_make_file, path.makefile( casual_make_file)])
    
def multiline( values):
    return internal.multiline( values)
    
def target_base( values):
    return internal.target_base( values)

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

            

    add_dependency( [ '.INTERMEDIATE'],
                    prerequisites = targets, 
                    comments = ("This is the only way I've got it to work. We got to have a INTERMEDIATE target\n"
                           "even if the target exist and have dependency to a file that is older. Don't really get make...")
                   )

    add_dependency( targets = dummy_targets, 
                    comments = 'dummy targets, that will be used if the real target is absent')
    
    return targets

def symlink(filename, linkname):
    
    print '\t' + platform().remove( linkname)
    print '\t' + platform().symlink( filename, linkname)
    

def link( operation, target, objectfiles, libraries, linkdirectives = '', prefix = ''):

    internal.validate_list( objectfiles);
    internal.validate_list( libraries);

    filename = target.file
                
    print "#"
    print "# Links: " + os.path.basename( filename)
    print 
     
    #
    # extract file if some is targets
    # 
    objectfiles = internal.target_files( objectfiles)
    
             
    dependent_targets = library_targets( libraries)

    #
    # Convert library targets to names/files, 
    #
    libraries = internal.target_base( libraries)

    destination_path = os.path.dirname( filename)
    
    add_dependency( [ 'link'], [ target.name], comments='link: ' + target.name)
    add_dependency( [ target.name], [ filename])
    
    objects_variable = add_local_variable( object_name_list( objectfiles), 'objects');
    libraries_variable = add_local_variable( platform().link_directive( libraries), 'libraries');
    dependency_variable = add_local_variable( dependent_targets, 'dependency', 'possible dependencies to other targets (in this makefile)');
    
    
    add_rule( filename, 
              prerequisites = [ '$(' + objects_variable + ')', '$(' + dependency_variable + ')', '$(USER_CASUAL_MAKE_FILE)'],
              ordered_prerequisites = [ destination_path],
              recipes = [ prefix + operation( filename, '$(' + objects_variable + ')', '$(' + libraries_variable + ')', linkdirectives)])
    
    if target.output:
        soname_fullpath = target.stem + '.' + target.output.version.soname_version()
        symlink(target.file, soname_fullpath)
        symlink(soname_fullpath, target.stem)
        
    print
    
    
    register_file_for_clean( filename)
    register_path_for_create( destination_path)
    
    return target




def install(target, destination):
    
    if isinstance( target, list):
        for t in target:
            install( t, destination)
            
    elif isinstance( target, tuple):
        install( target[ 0], destination + '/' + target[ 1])
        
    elif isinstance( target, basestring):
        install( internal.Target( target), destination)
    else:
        copied_target = copy.deepcopy( target)
        filename = copied_target.file

        copied_target.name = 'install_' + copied_target.target
        register_path_for_create( destination)
        
        print 'install: '+ copied_target.name
        print
        print copied_target.name + ": " + filename + ' | ' + destination
        print "\t" + platform().install( filename, destination)
        
        if copied_target.output:
            soname_fullpath = copied_target.stem + '.' + copied_target.output.version.soname_version()
            symlink(destination + '/' + os.path.basename(copied_target.file), destination + '/' + os.path.basename(soname_fullpath))
            symlink(destination + '/' + os.path.basename(soname_fullpath), destination + '/' + os.path.basename(copied_target.stem))
         
        return copied_target


def set_parallel_make( value):
    global parallel_make;
    
    # we only do stuff if we haven't set parallel before...
    if state().parallel_make is None:        
        state().parallel_make = value;
    
def platform():
    return _platform 



def internal_set_parallel_make( value):
    global parallel_make;
    
    # we only do stuff if we haven't set parallel before...
    if state().parallel_make is None:        
        state().parallel_make = value;
    
    

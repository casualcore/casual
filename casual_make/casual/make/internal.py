'''
Created on 13 maj 2012

@author: hbergk
'''

import os
import sys

from casual.make.platform.factory import factory

#
# Some global defines
#



#
# Set up casual_make-program. Normally casual_make
#
def_casual_make='casual_make.py'

def_Deploy='make.deploy.ksh'


parallel_make = None


objectPathsForClean=set()
filesToRemove=set()
pathsToCreate=set()
messages=set()



def internal_platform():
    return factory();


global_build_targets = [ 
          'link',
          'cross', 
          'clean_files',
          'clean_objectfiles',
          'clean_dependencyfiles', 
          'compile', 
          'deploy', 
          'install',
          'test', 
          'print_include_paths' ];

global_targets = [ 
          'make', 
          'clean',
           ] + global_build_targets;



internal_globalPreMakeStatements = []

def internal_add_pre_make_statement( statement):
    
    internal_globalPreMakeStatements.append( statement);

       
    

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
    
 
    if parallel_make is None or parallel_make:
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
       
    #
    # Targets for creating directories
    #
    for path in pathsToCreate:
        print
        print path + ":"        
        print "\t" + internal_platform().make_directory( path)
        print

    
    #
    # Target for clean
    #
    
    
    print
    print "clean: clean_objectfiles clean_dependencyfiles clean_files"
    
    print "clean_objectfiles:"
    for objectpath in objectPathsForClean:
        print "\t-" + internal_platform().remove( objectpath + "/*.o")
        
    print "clean_dependencyfiles:"
    for objectpath in objectPathsForClean:  
        print "\t-" + internal_platform().remove( objectpath + "/*.d")
    
    #
    # Remove all other known files.
    #
    print "clean_files:"
    for filename in filesToRemove:
        print "\t-" + internal_platform().remove( filename)
    
    #
    # Prints all messages that we've collected
    #
    if messages :
        sys.stderr.write( '$(USER_CASUAL_MAKE_FILE)' + ":\n" )
        sys.stderr.write( messages + "\n") 


#
# Registration of files that will be removed with clean
#

def internal_register_object_path_for_clean( objectpath):
    ''' '''
    objectPathsForClean.add( objectpath)


def internal_register_file_for_clean( filename):
    ''' '''
    filesToRemove.add( filename)


def internal_register_path_for_create( path):
    ''' '''
    pathsToCreate.add( path)






#
# Register warnings
#

def internal_register_message(message):

    messages.add("\E[33m" + message + "\E[m\n")



#
# Name and path's helpers
#

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

    return 'target_' + os.path.basename( name).replace( '.', '_').replace( '-', '_')



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
    




def internal_deploy( targetname, path, directive):
    
    targetname = 'deploy_' + targetname
    
    print
    print 'deploy: ' + targetname
    print
    print targetname + ":"
    print "\t-@" + def_Deploy + " " + path + ' ' + directive
    print


def internal_make_target_component( target, casualMakefile):

    user_casual_make_path = os.path.dirname( casualMakefile)
    user_make_file = os.path.splitext( casualMakefile)[0] + ".mk"
    

    target_name = target + '_' + internal_unique_target_name(casualMakefile)
    
    print target + ": " + target_name
    print
    print target_name + ": " + user_make_file
    print "\t@echo " + casualMakefile + " " + target
    print "\t@" + internal_platform().change_directory( user_casual_make_path)  + " && $(MAKE) -f " + user_make_file + " " + target
    

def internal_Build( casualMakefile):
    
    casualMakefile = os.path.abspath( casualMakefile)
    
    user_casual_make_path=os.path.dirname( casualMakefile)
    
    user_make_file = os.path.splitext( casualMakefile)[0] + ".mk"

    print "#"
    print "# If " + casualMakefile + " is newer than " + user_make_file + " , a new makefile is produced"
    print "#"
    print user_make_file + ": " + casualMakefile
    print "\t@echo generate makefile from " + casualMakefile
    print "\t@" + internal_platform().change_directory( user_casual_make_path) + ";" + def_casual_make + " " + casualMakefile
    print
    
    
    
    for target in global_build_targets:
        internal_make_target_component( target, casualMakefile)
        print   
   
   
    target_name = internal_unique_target_name( casualMakefile)
   
    print
    print "#"
    print "# Always produce " + user_make_file + " , even if " + casualMakefile + " is older."
    print "#"
    print "make: " + target_name
    print
    print target_name + ":"
    print "\t@echo generate makefile from " + casualMakefile
    print "\t@" + internal_platform().change_directory( user_casual_make_path) + ' && ' + def_casual_make + " " + casualMakefile
    print "\t@" + internal_platform().change_directory( user_casual_make_path) + ' && $(MAKE) -f ' + user_make_file + " make"
    print


def internal_library_targets( libs):

    targets = []
    
    if not libs:
        return targets
    
    internal_validate_list( libs)
    
    #
    # empty targets to make it work
    #
    
    platform = internal_platform();
    
    
    
    for lib in libs:
        targets.append( internal_target_name( platform.library_name( lib)))
        targets.append( internal_target_name( platform.archive_name( lib)))

    print
    print "#";
    print "# dummy targets, that will be used if the real target is absent";
    print "# so that we can have (dummy) dependencies even if the target is not produces in this makefile";
    print "#";
    for target in targets:    
        print ".INTERMEDIATE: " + target
    print
    for target in targets:    
        print target + ":"
    
    return targets





def internal_link( platform, name, filename, objectfiles, libs, linkdirectives = '', prefix = ''):

    internal_validate_list( objectfiles);
    internal_validate_list( libs);

    print "#"
    print "# Links: " + os.path.basename( filename)
    
    dependent_targets = internal_library_targets( libs)
    
    target_name = internal_target_name( filename)

    destination_path = os.path.dirname(filename)
    
    print
    print "link: " + target_name
    print 
    print target_name + ': ' + filename
    print
    print '   objects_' + target_name + ' = ' + internal_multiline( internal_object_name_list( objectfiles))
    print
    print '   libs_'  + target_name + ' = ' + internal_multiline( factory().link_directive( libs))
    print
    print '   #'
    print '   # possible dependencies to other targets (in this makefile)'
    print '   depenency_' + target_name + ' = ' + internal_multiline( dependent_targets)
    print 
    print filename + ': $(objects_' + target_name + ') $(depenency_' + target_name + ')' + " $(USER_CASUAL_MAKE_FILE) | " + destination_path
    print '\t' + prefix + platform( filename, '$(objects_' + target_name + ')', '$(libs_' + target_name + ')', linkdirectives)
    print
    
    
    internal_register_file_for_clean( filename)
    internal_register_path_for_create( destination_path)
    
    return target_name


def internal_install(target, source, destination):
    
    #
    # Add an extra $ in case of referenced environment variable.
    #
    if destination.startswith('$'):
        destination = '$' + destination
        
    print "install: " + target
    print
    print target + ": " + source
    print "\t@" + internal_platform().make_directory( os.path.dirname( destination))
    print "\t" + internal_platform().install( source, destination)
    print




def internal_set_parallel_make( value):
    global parallel_make;
    
    # we only do stuff if we haven't set parallel before...
    if parallel_make is None:        
        parallel_make = value;
    

def debug( message):
    if os.getenv('PYTHONDEBUG'): sys.stderr.write( message + '\n')

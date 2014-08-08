'''
Created on 28 apr 2012

@author: hbergk

Contains the basic syntax to produce makefile
'''

#
# Imports
#
from casual.make.internal import *

export=dict()
include=list()

#
# 
#


def NoDefaultLibs():
    """

 
 NoDefaultLibs()

 Makes sure nothing is linked default


    """

    print "DEFAULT_LIBS :=";
    print


def NoDefaultIncludePaths():
    """

 
 NoDefaultIncludePaths()

 Makes sure that no default include paths are set
 Do not interfere with INCLUDE_PATHS set by users 


    """
    print "DEFAULT_INCLUDE_PATHS :=";
    print


def NoDefaultLibraryPaths():
    """

 
 NoDefaultLibraryPaths()

 Makes sure that no default library paths are set
 Do not interfere with LIBRARY_PATHS set by users 


    """
    print "DEFAULT_LIBRARY_PATHS :="
    print





def Parallel( value = True):
    """
 Controls whether this casual-make file will be processed in parallel or not 

    """
    internal_set_parallel_make( value)




def Compile( sourcefile, objectfile, directive = ''):
    """
 Compiles a source file to an object file, with excplicit directives

 :param sourcefile:    name of the sourcefile (src/myfile.cpp)
 :param objectfile:    name of the output object file (obj/myfile.o)
 :param directive:   optional compile directive for this TU, default ''
 :return: the objectfile
    """

    
    source_file = internal_normalize_path( sourcefile)
    object_file = internal_normalize_path( objectfile)
    
    object_directory = os.path.dirname( object_file)
    
    
    
    dependency_file = internal_dependency_file_name( object_file)
    cross_object_file = internal_cross_object_name( object_file)
    
    
    print "#"
    print "# compiling {0} to {1}".format( sourcefile, objectfile)
    print
    print dependency_file + ':  | ' + object_directory
    print '\t' + internal_platform().header_dependency( source_file, [ cross_object_file, object_file], dependency_file)
    print
    print "-include " + dependency_file
    print
    print 'compile: ' + object_file
    print
    print object_file + ": " + source_file + ' ' + dependency_file + " | " + object_directory
    print '\t' + internal_platform().compile( source_file, object_file, directive)
    print
    print 'cross: ' + cross_object_file
    print 
    print cross_object_file + ": " + source_file + " | "  + object_directory
    print '\t' + internal_platform().cross_compile( source_file, cross_object_file, directive)
    print
    
    internal_register_object_path_for_clean( object_directory)
    internal_register_path_for_create( object_directory)

    return str( objectfile)




def LinkServer( name, objectfiles, libraries, serverdefinition, resources=None):
    """
 Links a XATMI-server

 param: name        name of the server with out prefix or suffix.
    
 param: objectfiles    object files that is linked

 param: libraries        dependent libraries

 param: serverdefinition  path to the server definition file that configure the public services, 
                         and semantics.
                         Can also be a list of public services. I e.  ["service1", "service2"]
                    
 param: resources  optional - a list of XA resources. I e ["db2-rm"] - the names shall 
                correspond to those defined in $CASUAL_HOME/configuration/resources.(yaml|json|...)
    """

    executable_path = internal_executable_name_path( name)
    
    target_name = internal_target_name( executable_path)
    

        
    
    
    
    if not resources:
        directive = "";
    else:
        directive = " -r " + ' '.join( resources)
    
    if isinstance( serverdefinition, basestring):
        # We assume it is a path to a server-definition-file
        directive += ' -p ' + serverdefinition
        print 
        print target_name + ': ' + serverdefinition        
    else:
        directive += ' -s ' + ' '.join( serverdefinition)

    
    internal_link( internal_platform().link_server, name, executable_path, objectfiles, libraries, directive)
    
    #target_name = internal_BASE_LinkATMI( "$(BUILDSERVER)", name, serverdefinition, "", objectfiles, libraries, resource_directive)
    
    return target_name
    

def LinkAtmiClient(name,objectfiles,libs = []):
    """
 Links a XATMI client

 param: name        name of the binary with out prefix or suffix.
 param: objectfiles    object files that is linked
 param: libs        dependent libraries
    """

    return internal_BASE_LinkATMI( "$(BUILDCLIENT)",name, "", objectfiles, libs, "")


def LinkLibrary(name,objectfiles,libs = []):
    """LinkLibrary(name,objectfiles,libs)
 Links a shared library
 
    :param name        name of the binary with out prefix or suffix.
    :param objectfiles    object files that is linked
    :param libs        dependent libraries

    :return: target name
    """
    
    library_path = internal_shared_library_name_path( name)
    
    target_name = internal_link( internal_platform().link_library, name, library_path, objectfiles, libs)
    
    internal_deploy( target_name, library_path, 'lib')
        
    return target_name;


def LinkArchive(name,objectfiles):
    """
 Links an archive

 :param: name        name of the binary with out prefix or suffix.  
 :param: objectfiles    object files that is linked
 :return: target name
    """
    
    archive_path = internal_archive_name_path( name)
    
    target_name = internal_link( internal_platform().link_archive, name, archive_path, objectfiles, [])
    
    return target_name;



def LinkExecutable(name,objectfiles,libs = []):
    """
  Links an executable

 :param name        name of the binary with out prefix or suffix.
    
 :param objectfiles    object files that is linked

 :param libs        dependent libraries
 
 :return: target name
    """
    executable_path = internal_executable_name_path( name)
    
    target_name = internal_link( internal_platform().link_executable, name, executable_path, objectfiles, libs)
    
    internal_deploy( target_name, executable_path, 'exe')
    
    return target_name;


def Dependencies( target, dependencies):
    """
    Set dependencies to arbitary targets. This is not needed in the
    general case, since casual-make takes care of most dependencies 
    automatic.
    
    This can be used when one wants to execute binaries in a unittest-scenario.
    To make sure that the actual binaries that will run is linked before the
    unittest is linked (and run).
    
    :param target: the target that has dependencies
    :param dependencies: target dependencies
    
    """
    
    print '#'
    print '# explicit dependencies'
    print target + ": " + ' '.join( dependencies)
    


def Build(casualMakefile):
    """
 "builds" another casual-make-file: jumps to the specific file and execute make

 :param casualMakefile    The file to build
    """
    
    #
    # Make sure we do this sequential 
    #
    Parallel( False)
    
    internal_build( casualMakefile);
    
    

def LinkIsolatedUnittest(name,objectfiles,libs):
    """


 LinkIsolatedUnittest(name,objectfiles,libs)

 param: name        name of the unittest executable
    
 param: objectfiles    object files that is linked

 param: libs        dependent libraries


    """
    
    executable_path = internal_executable_name_path(name)

    target_name = internal_link( internal_platform().link_executable, name, executable_path, objectfiles, libs, '$(ISOLATED_UNITTEST_LIB)')
    
    internal_deploy( target_name, executable_path, 'client')

    internal_set_ld_path()
    
    print "test: " + 'test_' + target_name    
    print
    print 'test_' + target_name + ": " +  target_name
    print "\t @LD_LIBRARY_PATH=$(LOCAL_LD_LIBRARY_PATH) $(VALGRIND_CONFIG) " + executable_path + " $(ISOLATED_UNITTEST_DIRECTIVES)"
    print 

    return target_name


def LinkDependentUnittest(name,objectfiles,libs):
    """


 LinkDependentUnittest(name,objectfiles,libs)

 param: name        name of the unittest executable
    
 param: objectfiles    object files that is linked

 param: libs        dependent libraries


    """
    return internal_BASE_LinkATMI("$(BUILDCLIENT)", name, "" , objectfiles, libs , "-f $(DEPENDENT_UNITTEST_LIB)")
    





def InstallLibrary(source, destination):
    
    source_path = internal_shared_library_name_path( source);
    target_name = 'install_' + internal_target_name( source_path)
    
    internal_install( target_name, source_path, destination)

def InstallExecutable(source, destination):
    
    source_path = internal_executable_name_path( source);
    target_name = 'install_' + internal_target_name( source_path)
    
    internal_install( target_name, source_path, destination)
    

def Install(source, destination):
    
    target_name = 'install_' + internal_target_name( source)
    internal_install( target_name, source, destination)

def Include( filename):
    
    internal_add_pre_make_statement( 'include ' + filename);
           
        
def SetIncludePaths( value ):
    
    internal_add_pre_make_statement( 'INCLUDE_PATHS = ' + ' '.join( value));
    
    
def SetLibraryPaths( value ):
    
    internal_add_pre_make_statement( 'LIBRARY_PATHS = ' + ' '.join( value));

    
    

        
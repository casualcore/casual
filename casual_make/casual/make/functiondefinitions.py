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


class Target:
    
    def __init__(self, filename):
        self.file = filename
        self.name = internal_target_name( filename)





def NoDefaultLibs():
    """

 
 NoDefaultLibs()

 Makes sure nothing is linked default


    """

    print "DEFAULT_LIBS :=";
    print


def DefaultIncludePaths( value = True):
    """ 
    If the default include paths is used or not.
    Default they are used
    """
    if not value:
        print "DEFAULT_INCLUDE_PATHS :=";
        print


def DefaultLibraryPaths( value = True):
    """
    If the default librarys paths is used or not.
    Default they are used
    """
    if not value:
        print "DEFAULT_LIBRARY_PATHS :="
        print





def Parallel( value = True):
    """
 Controls whether this casual-make file will be processed in parallel or not 

    """
    internal_set_parallel_make( value)


def Scope( inherit = True, name = None):
    """
    Creates a new scope, with new context. Think of it as a nested
    casual-make-file.
    
    Shall be used in a 'with statement", example:
    
    with Scope():
        Parallel()
        
        Build( someother-casual-make-file)
        
    User can use as many scopes as he or she wants. The scopes can be nested in
    arbitrary depth
    
    
    Under the hood the scope is generated in a isolated makefile, and
    invoked from the parent scope.
    
    :param inherit: If true, state is copied from the parent scope. Normally
        this correlates to IncludePaths and such.
        
    :param name: If set the scope is named to the value. It only effects the
        name of the isolated makefile   
    
    """
    return internal_scope( inherit, name)
    
    

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
    print dependency_file + ': ' + source_file + ' | ' + object_directory
    print '\t' + internal_platform().header_dependency( source_file, [ cross_object_file, object_file], dependency_file)
    print
    print "-include " + dependency_file
    print
    print 'compile: ' + object_file
    print
    print object_file + ": " + source_file + ' | ' + object_directory + ' ' + dependency_file
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

    target = Target( internal_executable_name_path( name))
    

    if not resources:
        directive = "";
    else:
        directive = " -r " + ' '.join( resources)
    
    if isinstance( serverdefinition, basestring):
        # We assume it is a path to a server-definition-file
        directive += ' -p ' + serverdefinition
        print 
        print target.name + ': ' + serverdefinition        
    else:
        directive += ' -s ' + ' '.join( serverdefinition)

    
    return internal_link( internal_platform().link_server, target, objectfiles, libraries, directive)
    

def LinkClient( name, objectfiles, libraries, resources=None):
    """
 Links a XATMI client

 param: name        name of the binary with out prefix or suffix.
 param: objectfiles    object files that is linked
 param: libs        dependent libraries
    """
    
    target = Target( internal_executable_name_path( name))    

    if not resources:
        directive = "";
    else:
        directive = " -r " + ' '.join( resources)
        
    internal_link( internal_platform().link_client, target, objectfiles, libraries, directive)


def LinkLibrary(name,objectfiles,libs = []):
    """LinkLibrary(name,objectfiles,libs)
 Links a shared library
 
    :param name        name of the binary with out prefix or suffix.
    :param objectfiles    object files that is linked
    :param libs        dependent libraries

    :return: target name
    """
    
    target = Target( internal_shared_library_name_path( name))   
    
    internal_link( internal_platform().link_library, target, objectfiles, libs)
    
    internal_deploy( target, 'lib')
        
    return target;


def LinkArchive(name,objectfiles):
    """
 Links an archive

 :param: name        name of the binary with out prefix or suffix.  
 :param: objectfiles    object files that is linked
 :return: target name
    """
    
    target = Target( internal_archive_name_path( name))
    
    return internal_link( internal_platform().link_archive, target, objectfiles, [])
    



def LinkExecutable( name, objectfiles, libraries = []):
    """
  Links an executable

 :param name        name of the binary with out prefix or suffix.
    
 :param objectfiles    object files that is linked

 :param libs        dependent libraries
 
 :return: target name
    """
    
    target = Target( internal_executable_name_path( name))
    
    
    target_name = internal_link( internal_platform().link_executable, target, objectfiles, libraries)
    
    internal_deploy( target, 'exe')
    
    return target;


def LinkResource( name, resource, libraries = [], directive = ''):
    
    target = Target( internal_executable_name_path( name))
    
    return internal_link_resource( target, resource, libraries, directive)
    


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
    dependant = list()
    for dependency in dependencies:
        dependant.append( dependency.file)
    
    
    print '#'
    print '# explicit dependencies'
    print target.name + ": " + ' '.join( dependant)
    


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
    
    

def LinkIsolatedUnittest(name,objectfiles,libraries):
    """


 LinkIsolatedUnittest(name,objectfiles,libs)

 param: name        name of the unittest executable
    
 param: objectfiles    object files that is linked

 param: libs        dependent libraries


    """
    
    target = Target( internal_executable_name_path( name))
    

    internal_link( internal_platform().link_executable, target, objectfiles, libraries, '$(ISOLATED_UNITTEST_LIB)')
    
    internal_deploy( target, 'client')

    internal_set_ld_path()
    
    print "test: " + 'test_' + target.name    
    print
    print 'test_' + target.name + ": " +  target.name
    print "\t @LD_LIBRARY_PATH=$(LOCAL_LD_LIBRARY_PATH) $(VALGRIND_CONFIG) " + target.file + " $(ISOLATED_UNITTEST_DIRECTIVES)"
    print 

    return target


def LinkDependentUnittest( name, objectfiles, libraries, resources = None):
    """


 LinkDependentUnittest(name,objectfiles,libs)

 param: name        name of the unittest executable
    
 param: objectfiles    object files that is linked

 param: libs        dependent libraries


    """
    
    target = Target( internal_executable_name_path( name))    

    if not resources:
        directive = "";
    else:
        directive = " -r " + ' '.join( resources)
        
    libraries.append( "$(DEPENDENT_UNITTEST_LIB)")
        
    internal_link( internal_platform().link_client, target, objectfiles, libraries, directive)
    
    return target;




def InstallLibrary(source, destination):
    """
    deprecated
    """
    
    source_path = internal_shared_library_name_path( source);
    target_name = 'install_' + internal_target_name( source_path)
    
    internal_install( target_name, source_path, destination)

def InstallExecutable(source, destination):
    
    """
    deprecated
    """
    
    source_path = internal_executable_name_path( source);
    target_name = 'install_' + internal_target_name( source_path)
    
    internal_install( target_name, source_path, destination)
    

def Install( target, destination):
    
    if isinstance( target, Target):
        internal_install( 'install_' + target.name, target.file, destination)
    
    elif isinstance( target, list):
        for t in target:
            Install( t, destination)
        
    else:
        target_name = 'install_' + internal_target_name( target)
        internal_install( target_name, target, destination)

def Include( filename):
    
    internal_add_pre_make_statement( 'include ' + filename);
           
        
def IncludePaths( paths):
    
    internal_add_pre_make_statement( 'INCLUDE_PATHS = ' + internal_platform().include_paths( paths));
    
    
def LibraryPaths( paths):
    
    internal_add_pre_make_statement( 'LIBRARY_PATHS = ' + internal_platform().library_paths( paths));

    
    

        
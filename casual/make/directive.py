'''
Created on 28 apr 2012

@author: hbergk

Contains the basic syntax to produce makefile
'''

#
# Imports
#
from casual.make.internal.directive import *


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
    

def Environment( name, value = '', export = True):
    """
    Sets an environment variable
    :param name: the name of the variable
    :param value: the value to be set
    :param export: if the variable shuld be exportet to sub-makefiles and shells
    
    """
    
    if( export):
        print 'export ' + name + ' = ' + value
    else:
        print name + ' = ' + value
    
    

def Compile( sourcefile, objectfile = None, directive = ''):
    """
 Compiles a source file to an object file, with excplicit directives

 :param sourcefile:    name of the sourcefile (src/myfile.cpp)
 :param objectfile:  optional name of the output object file (obj/myfile.o)
 :param directive:   optional compile directive for this TU, default ''
 :return: the target (which contains the name of the objectfile) 
    """

    if not objectfile:
        objectfile = internal_platform().default_object_name( sourcefile)
        

    target = Target( internal_normalize_path( objectfile), objectfile, 'target_' + internal_normalize_string( objectfile))
    target.source = internal_normalize_path( sourcefile);
    
    object_directory = os.path.dirname( target.file)
    
    dependency_file = internal_dependency_file_name( target.file)
    cross_object_file = internal_cross_object_name( target.file)
    
    print "#"
    print "# compiling {0} to {1}".format( sourcefile, objectfile)
    print
    print "-include " + dependency_file
    print
    print 'compile: ' + target.name
    print
    print target.name + ': ' + target.file
    print
    print target.file + ": " + target.source + ' | ' + object_directory
    print '\t' + internal_platform().compile( target.source, target.file, directive)
    print '\t' + internal_platform().header_dependency( target.source, [ cross_object_file, target.file], dependency_file)
    print
    print 'cross: ' + cross_object_file
    print 
    print cross_object_file + ": " + target.source + ' | ' + object_directory
    print '\t' + internal_platform().cross_compile( target.source, cross_object_file, directive)
    print '\t' + internal_platform().header_dependency( target.source, [ cross_object_file, target.file], dependency_file)
    print
    
    internal_register_object_path_for_clean( object_directory)
    internal_register_path_for_create( object_directory)

    return target




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

    target = Target( internal_executable_name_path( name), name)
    
    directive = "";

    if resources:
        directive += " -r " + ' '.join( resources)
    
    if isinstance( serverdefinition, basestring):
        # We assume it is a path to a server-definition-file
        directive += ' -p ' + serverdefinition
        
        print '# dependency to server definition file'
        print target.file + ': ' + serverdefinition
        print
        
        server_defintion_file = serverdefinition;
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
    
    target = Target( internal_executable_name_path( name), name)    

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
    
    target = Target( internal_shared_library_name_path( name), name)   
    
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
    
    target = Target( internal_archive_name_path( name), name)
    
    return internal_link( internal_platform().link_archive, target, objectfiles, [])
    



def LinkExecutable( name, objectfiles, libraries = []):
    """
  Links an executable

 :param name        name of the binary with out prefix or suffix.
    
 :param objectfiles    object files that is linked

 :param libs        dependent libraries
 
 :return: target name
    """
    
    target = Target( internal_executable_name_path( name), name)
    
    
    target_name = internal_link( internal_platform().link_executable, target, objectfiles, libraries)
    
    internal_deploy( target, 'exe')
    
    return target;


def LinkResourceProxy( name, resource, libraries = [], directive = ''):
    
    target = Target( internal_executable_name_path( name), name)
    
    return internal_link_resource_proxy( target, resource, libraries, directive)
    


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
    

    #
    # We add the unittest lib for the user...
    #
    internal_link( internal_platform().link_executable, target, objectfiles, libraries + ['$(ISOLATED_UNITTEST_LIB)'])
    
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


    

def Install( target, destination):
    
    internal_install( target, destination)
    

def Include( filename):
    
    internal_add_pre_make_statement( 'include ' + filename);
           
        
def IncludePaths( paths):
    
    internal_add_pre_make_statement( 'INCLUDE_PATHS = ' + ' '.join( paths));
    
    
def LibraryPaths( paths):
    
    internal_add_pre_make_statement( 'LIBRARY_PATHS = ' + ' '.join( paths));

    
    

        
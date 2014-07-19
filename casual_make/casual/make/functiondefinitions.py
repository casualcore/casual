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



def NoParallel():
    """

 
 NoParallel()

 Makes sure that noting is executed parallel in this makefile


    """
    if getParallelMake() == 0 :
        print
        print "#"
        print "# Parallelity is overridable"
        print "# NOTPARALLEL has precedence"
        print "#"
        print "ifdef FORCE_NOTPARALLEL"
        print ".NOTPARALLEL:"
        print "endif"
        print "ifndef FORCE_PARALLEL"
        print ".NOTPARALLEL:"
        print "endif"
        print
        #
        # Make sure not to printout this again
        #
        setParallelMake(2)


def Parallel():
    """

 
 Parallel()

 Makes sure that execution is parallel in this makefile
 This statement has to be first in this makefile


    """
    setParallelMake( 1)




def Compile( sourcefile, objectfile, directive = ''):
    """
 Compiles a source file to an object file, with excplicit directives

 :param sourcefile:    name of the sourcefile (src/myfile.cpp)
 :param objectfile:    name of the output object file (obj/myfile.o)
 :param directive:   optional compile directive for this TU, default ''
 :return: the objectfile
    """

    local_object_path=def_CurrentDirectory + "/" + internal_clean_directory_name( os.path.dirname( objectfile))
    
    local_source_file=def_CurrentDirectory + "/" + sourcefile
    local_object_file=def_CurrentDirectory + "/" + objectfile
    
    local_dependency_file=internal_dependency_file_name( local_object_file)
    local_cross_object_file= internal_cross_object_name( local_object_file)
    
    internal_map_target( objectfile , local_object_file);

    
    print "#"
    print "# compiling {0} to {1}".format( sourcefile, objectfile)
    print
    print local_dependency_file + ':'
    print "\t@$(HEADER_DEPENDENCY_COMMAND) -MT '{0} {1}' $(INCLUDE_PATHS) $(DEFAULT_INCLUDE_PATHS) {2} -MF {3}".format(local_cross_object_file, local_object_file, local_source_file, local_dependency_file)
    print
    print "-include " + local_dependency_file
    print 
    print local_object_file + ": " + local_source_file + ' ' + local_dependency_file + " | " + local_object_path                                                                        
    print "\t$(COMPILER) -o {0} {1}  $(INCLUDE_PATHS) $(DEFAULT_INCLUDE_PATHS) $(COMPILE_DIRECTIVES) {2}".format(objectfile, local_source_file, directive )
    print 
    print local_cross_object_file + ": " + local_source_file + " | "  + local_object_path                                                                      
    print "\t$(CROSSCOMPILER) $(CROSS_COMPILE_DIRECTIVES) -o " + local_cross_object_file + " " + local_source_file + " $(INCLUDE_PATHS) $(DEFAULT_INCLUDE_PATHS) "
    print
    
    internal_register_object_path_for_clean( local_object_path)
    internal_register_path_for_create( local_object_path)

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

    if not resources:
        resource_directive = "";
    else:
        resource_directive = " -r " + ' '.join( resources)

    return internal_BASE_LinkATMI( "$(BUILDSERVER)", name, serverdefinition, "", objectfiles, libraries, resource_directive)




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

    :return: library name
    """
    internal_base_link("$(LIBRARY_LINKER)", name, internal_shared_library_name_path( name), objectfiles, libs, " $(LINK_DIRECTIVES_LIB)")
    
    print internal_target_deploy_name( name) + ":"
    print "\t@" + def_Deploy + " " + internal_shared_library_name(name) + " lib"
    print 
        
    return name;


def LinkArchive(name,objectfiles):
    """
 Links an archive

 :param: name        name of the binary with out prefix or suffix.  
 :param: objectfiles    object files that is linked
 :return: archive name
    """

    objectfiles = ' '.join(objectfiles)
    
    print "#"
    print "#    Links "+ name
    
    filename=internal_archive_name_path(name)
    
    local_destination_path=internal_clean_directory_name( os.path.dirname( filename))
    
    print
    print "all: "+ internal_target_name(name)
    print
    print "cross: " + internal_cross_dependecies(objectfiles)
    print
    print "deploy: " + internal_target_deploy_name( name)
    print 
    print internal_target_name(name) + ": " + filename
    print
    print "objects_" + name + " = " + internal_object_name_list( objectfiles)
    print 
    print "compile: $(objects_" + name +")"
    print
    print filename + ": $(objects_" + name + ") | " + local_destination_path
    print "\t$(ARCHIVE_LINKER) " + filename + " $(objects_" + name + ")"
    print
    
    internal_register_file_for_clean(filename)
    internal_register_path_for_create(local_destination_path)

    print internal_target_deploy_name( name) + ":"

    return name;

def LinkExecutable(name,objectfiles,libs = []):
    """
  Links an executable

 :param name        name of the binary with out prefix or suffix.
    
 :param objectfiles    object files that is linked

 :param libs        dependent libraries
    """
    internal_base_link("$(EXECUTABLE_LINKER)", name, internal_executable_name_path( name), objectfiles, libs, "$(LINK_DIRECTIVES_EXE)")
    
    print internal_target_deploy_name(name) + ":"
    print "\t-@" + def_Deploy + " " + internal_executable_name(name) + " exe"
    print 

    return name;


def Dependencies( target, dependencies):
    """
    Set dependencies to arbitary targets. This is not needed in the
    general case, since casual-make takes care of most dependencies 
    automatic.
    
    This can be used when one wants to execute binaries in a unittest-scenario.
    To make sure that the actual binaries that will run is linked before the
    unittest is linked (and run).
    
    :param target: the target that has dependencies
    :param dependencies: targest dependencies
    
    """
    target_depdendencies = [ internal_target_name_from_user_name( d) for d in dependencies];
    
    print '#'
    print '# explicit dependencies'
    print internal_target_name_from_user_name( target) + ": " + ' '.join( target_depdendencies)
    


def Build(casualMakefile):
    """
 "builds" another casual-make-file: jumps to the spcific file and execute make

 :param casualMakefile    The file to build
    """
    
    #
    # Make sure we do this sequential 
    #
    NoParallel()
    
    internal_Build( casualMakefile);
    
    

def LinkIsolatedUnittest(name,objectfiles,libs):
    """


 LinkIsolatedUnittest(name,objectfiles,libs)

 param: name        name of the unittest executable
    
 param: objectfiles    object files that is linked

 param: libs        dependent libraries


    """

    internal_base_link( "$(EXECUTABLE_LINKER)", name, internal_executable_name_path(name), objectfiles, libs, "$(ISOLATED_UNITTEST_LIB) $(LINK_DIRECTIVES_EXE)")

    print internal_target_deploy_name(name) + ":"
    print "\t@" + def_Deploy + " " + internal_executable_name( name) + " client"
    print 
    
    internal_set_LD_LIBRARY_PATH()
    
    print "test: " + internal_target_isolatedunittest_name( name)    
    print
    print internal_target_isolatedunittest_name(name) + ": " +  internal_executable_name_path(name)
    print "\t @LD_LIBRARY_PATH=$(LOCAL_LD_LIBRARY_PATH) $(VALGRIND_CONFIG) " + internal_executable_name_path( name) + " $(ISOLATED_UNITTEST_DIRECTIVES)"
    print 

    internal_map_target( name, internal_target_isolatedunittest_name(name));




def LinkDependentUnittest(name,objectfiles,libs):
    """


 LinkDependentUnittest(name,objectfiles,libs)

 param: name        name of the unittest executable
    
 param: objectfiles    object files that is linked

 param: libs        dependent libraries


    """
    internal_BASE_LinkATMI("$(BUILDCLIENT)", name, "" , objectfiles, libs , "-f $(DEPENDENT_UNITTEST_LIB)")
    





def InstallLibrary(source, destination):
    
    local_target_name=internal_unique_target_name(source)
    
    internal_install( local_target_name, internal_shared_library_name_path(source), destination)

def InstallExecutable(source, destination):
    
    local_target_name=internal_unique_target_name(source)
    
    internal_install( local_target_name, internal_executable_name_path(source), destination)

def Install(source, destination):
    
    local_target_name=internal_unique_target_name(source)
    
    internal_install( local_target_name, source, destination)

def Include( filename):
    
    internal_add_pre_make_statement( 'include ' + filename);
           
        
def SetIncludePaths( value ):
    
    internal_add_pre_make_statement( 'INCLUDE_PATHS = ' + ' '.join( value));
    
    
def SetLibraryPaths( value ):
    
    internal_add_pre_make_statement( 'LIBRARY_PATHS = ' + ' '.join( value));

    
    

        
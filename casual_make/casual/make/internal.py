'''
Created on 13 maj 2012

@author: hbergk
'''

import os
import sys

#
# Some global defines
#
def_ObjectSuffix=".o"
def_ExeSuffix=""

def_LibraryPrefix="lib"

def_BNDsuf=".bnd"

def_LibrarySuffix=".so";

def_ArchiveSuffix=".a"

def_RM="rm -f ";
def_CD="cd";

def_RSYNC = "rsync --checksum -i"
def_MKDIR_RECURSIVE="mkdir -p";
def_CHMOD="chmod"

def_MV="mv";

def_Prep="somethinglocal.prep"

#
# Set up casual_make-program. Normally casual_make
#
def_casual_make="casual_make.py"

def_Deploy="make.deploy.ksh"

def_CurrentDirectory=os.getcwd()


def_PARALLEL_MAKE=0
def_LD_LIBRARY_PATH_SET = 0


objectPathsForClean=set()
filesToRemove=set()
pathsToCreate=set()
messages=set()


targetSequence=1

USER_CASUAL_MAKE_PATH=""
USER_CASUAL_MAKE_FILE=""


#
# Normalize name 
#
def internal_clean_directory_name(name):
    return str.replace(name, "./", "")

#
# replace / to _
#
def internal_convert_path_to_target_name(name):
    return "target_" + str.replace( name, "/", "_" )

#
# Anropas av engine efter det att imake-filen har parsats klart.
# Denna funktion ar alltsa till for att vi ska ha mojlighet att producera
# ackumulerad information.
#
def internal_post_make_rules():

 
    #
    # Skapa target for att ta reda pa include-paths och library-paths beroenden.
    # for att nyttjas i andra sammanhang.
    # vi skriver till filen som miljovariabeln INCLUDE_PATHS_FILE dikterar.
    # 
#     print
#     print "#"
#     print "# Skriver vilka include-paths som nyttjas till filen som miljovariabeln INCLUDE_PATHS_FILE dikterar."
#     print "#"
#     print "   clean_include_paths = $(subst -I,, $(strip $(INCLUDE_PATHS) $(DEFAULT_INCLUDE_PATHS)))"
#     print "print_include_paths:"
#     print "\t@for inc in $(clean_include_paths); do \\"
#     print "\t\tif [[ $$inc != \"./inc\" ]] ; then \\"
#     print "\t\tCURRENT_PATH=`pwd`; if cd $$inc 2>/dev/null; then pwd >> $$INCLUDE_PATHS_FILE; cd $$CURRENT_PATH; else echo \"error: invalid include path: $$inc in $(THIS_MAKEFILE)\" > /dev/stderr; fi\\"
#     print "\t\tfi ; \\"
#     print "\tdone "
#     print


    if def_PARALLEL_MAKE < 2:
        print
        print "#"
        print "# Sequential processing can be forced"
        print "#"
        print "ifdef FORCE_NOTPARALLEL"
        print ".NOTPARALLEL:"
        print "endif"
        print
       
    #
    # Targets for creating directories
    #
    for path in pathsToCreate:
        print
        print path + ":"
        #print "\t@echo Create directory: " + path
        print "\t" + def_MKDIR_RECURSIVE + " " + path
        print

    
    #
    # Target for clean
    #
    
    
    print
    print "clean:"
    
    for objectpath in objectPathsForClean:
        print "\t-" + def_RM + " " + objectpath + "/*.o"
        print "\t-" + def_RM + " " + objectpath + "/*.d"
    
    
    
    #
    # Remove all other known files.
    #
    for filename in filesToRemove:
        print "\t-" + def_RM + " " + filename
    
    
    #
    # Prints all messages that we've collected
    #
    if messages :
        sys.stderr.write( os.getcwd() + "/" + USER_CASUAL_MAKE_FILE + ":\n" )
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

def internal_normalize_path(path):

    if path[0:1] == "/" :
        return path
    else:
        return def_CurrentDirectory + "/" + path

def internal_header_name(name):
    
    return name


def internal_header_name_path(name):

    return internal_normalize_path("inc/" + name)


def internal_shared_library_name(name):

    return def_LibraryPrefix + name + def_LibrarySuffix


def internal_shared_library_name_path(name):

    return internal_normalize_path( "bin/" + def_LibraryPrefix + name + def_LibrarySuffix)


def internal_archive_name(name):

    return def_LibraryPrefix + name + def_ArchiveSuffix


def internal_archive_name_path(name):

    return internal_normalize_path( "bin/" + def_LibraryPrefix + name + def_ArchiveSuffix)

def internal_executable_name(name):

    return name + def_ExeSuffix


def internal_executable_name_path(name):

    return internal_normalize_path( "bin/" + name + def_ExeSuffix)


def internal_bind_name(name):

    return name + def_BNDsuf


def internal_bind_name_path(name):

    return internal_normalize_path( "bin/" + name + def_BNDsuf)


def internal_target_name(name):

    return "target_" + name


def internal_archive_target_name(name):

    return "target_archive_" + name


#
# Levererar ett unikt target name varje
# anrop.
# vilket inte riktigt fungerar nu... Blir fasen samma..
#
def internal_unique_target_name(name):

    global targetSequence
    tempname= internal_convert_path_to_target_name( name) + "_" + str(targetSequence)
    targetSequence = targetSequence + 1
    return tempname


def internal_object_name_list(objects):

    return "$(addprefix " + def_CurrentDirectory + "/," + objects + ")"


def internal_target_deploy_name(name):

    return "target_deploy_" + name


def internal_target_isolatedunittest_name(name):

    return "target_isolatedunittest_"  + name


def internal_cross_object_name(name):

    #
    # Ta bort '.o' och lagg till _crosscompile.o
    #
    return internal_normalize_path( str.replace( name, ".o", "_crosscompile" + def_ObjectSuffix))


def internal_cross_dependecies( objectfiles):
    
    return internal_object_name_list( objectfiles.replace(".o", "_crosscompile.o"))

def internal_dependency_file_name(objectfile):
    
    return objectfile.replace(".o", "") + ".d"

def internal_set_LD_LIBRARY_PATH():
    """ """
    if def_LD_LIBRARY_PATH_SET < 1:
        print "#"
        print "# Set LD_LIBRARY_PATH so that unittest has access to dependent libraries"
        print "# We only set this once"
        print "#"
        print "space :=  "
        print "space +=  "
        print "formattet_library_path = $(subst -L,,$(subst $(space),:,$(LIBRARY_PATHS) $(DEFAULT_LIBRARY_PATHS)))"
        print "LOCAL_LD_LIBRARY_PATH=$(formattet_library_path):$(PLATFORMLIB_DIR)"
        print 
        #
        # Se till sa vi inte skriver ut detta igen
        #
        #def_LD_LIBRARY_PATH_SET=1


def internal_prepare_old_objectlist(objects):

    for o in objects:
        print "obj/" + o + def_ObjectSuffix
 
 
def internal_print_services_file_dependency(name, service_file):

    print internal_executable_name_path( name) + ": " + service_file
    print 
               
#
# Intern hjalpfuntktion for att lanka atmi...
#
def internal_BASE_LinkATMI(atmibuild, name, serverdefintion, predirectives, objectfiles, libs, buildserverdirective):
    
    objectfiles = ' '.join(objectfiles)
    libs = ' '.join(libs)
    
    print "#"
    print "#  Links " +  name
    
    #
    # We have to make sure that the target name is "unique"
    #
    atmi_target_name=name + "_xatmi"
    
    internal_library_targets( libs)
    
    DEPENDENT_TARGETS=internal_library_dependencies( libs)
    
    
    if isinstance( serverdefintion, basestring):
        # We assume it is a path to a server-definition-file
        services_directive = ' -p ' + serverdefintion
        internal_print_services_file_dependency( name, serverdefintion)
    else:
        services_directive = ' -s ' + ' '.join( serverdefintion)
    
    
    local_destination_path = internal_clean_directory_name( os.path.dirname( internal_executable_name_path(name)))
    
    
    print 
    print "all: " + internal_target_name( atmi_target_name)
    print
    print "cross: " + internal_cross_dependecies( objectfiles)
    print 
    print "deploy: $(internal_target_deploy_name $atmi_target_name)"
    print 
    print internal_target_name(atmi_target_name) + ": " + internal_executable_name_path( name) + " $(USER_CASUAL_MAKE_FILE) | " + local_destination_path
    print
    print "objects_" + atmi_target_name + " = " + internal_object_name_list( objectfiles)
    print "libs_" + atmi_target_name + " = $(addprefix -l, " + libs + ")"
    print
    print "compile: $(objects_" + atmi_target_name + ")"
    
    print 
    print internal_executable_name_path(name) + ": $(objects_" + atmi_target_name + ") " + DEPENDENT_TARGETS
    print "\t" + atmibuild + " -o " + internal_executable_name_path( name) + " " + services_directive + " " + predirectives + " -f \"$(objects_" + atmi_target_name + ")\" -f \"$(LIBRARY_PATHS) $(DEFAULT_LIBRARY_PATHS) $(libs_" + atmi_target_name + ")  $(DEFAULT_LIBS)\"" + buildserverdirective + " -f \"$(LINK_DIRECTIVES_EXE)\" -f \"$(INCLUDE_PATHS)\""
    print
    print "$(internal_target_deploy_name $atmi_target_name):"
    print "\t-@$def_Deploy $(internal_executable_name $name) exe"
    print 
    internal_register_file_for_clean( internal_executable_name_path(name))
    internal_register_path_for_create( local_destination_path)

    return internal_target_name( atmi_target_name);



 

def internal_library_targets( libs):

    print
    
    #
    # Skapa dummys som ser till att ingen omlankning sker om
    # libbet inte byggs i denna makefil
    #
    print "#";
    print "# INTERMEDIATE-declaration for targets";
    print "# so that we can have (dummy) dependencies even if the target is not produces in this makefile";
    print "#";
    for lib in libs.split():
        print ".INTERMEDIATE: " + internal_target_name(lib)
        print ".INTERMEDIATE: " + internal_archive_target_name(lib)
    print
    
    #
    # empty targets to make it work?
    #
    print "#";
    print "# dummy targets, that will be used if the real target is absent";
    print "#";
    for lib in libs.split():
        print internal_target_name(lib) + ":"
        print internal_archive_target_name(lib) + ":"
    



def internal_library_dependencies( libs):
    result=""
    for lib in libs.split():
        result = result + internal_target_name(lib) + " " + internal_archive_target_name(lib) + " "
    return result


#
# Intern hjalpfunktion for att lanka
#
def internal_base_link(linker,name,filename,objectfiles,libs,linkdirectives):

    objectfiles = ' '.join(objectfiles)
    libs = ' '.join(libs)

    print "#"
    print "# Links: " + name
    
    internal_library_targets( libs)

    DEPENDENT_TARGETS=internal_library_dependencies(libs)
    

    local_destination_path=internal_clean_directory_name( os.path.dirname(filename))
    
    print
    print "all: " + internal_target_name( name)
    print
    print "cross: " + internal_cross_dependecies( objectfiles)
    print
    print "deploy: " + internal_target_deploy_name( name)
    print 
    print internal_target_name(name) + ": " + filename
    print
    print "   objects_" + name + " = " + internal_object_name_list( objectfiles)
    print "   libs_"+ name + " = $(addprefix -l, " + libs + ")"
    print 
    print "compile: " + "$(objects_" + name + ")"
    print 
    print filename + ": $(objects_" + name + ") " + DEPENDENT_TARGETS + " $(USER_CASUAL_MAKE_FILE) | " + local_destination_path

    if linker == "ARCHIVER":
        print "\t" + linker + " " + linkdirectives + " " + filename  + " $(objects_" + name + ")"
    else:
        print "\t" + linker + " -o " + filename + " $(objects_" + name + ") $(LIBRARY_PATHS) $(DEFAULT_LIBRARY_PATHS) $(libs_" + name + ") $(DEFAULT_LIBS) " + linkdirectives
    print
    
    internal_register_file_for_clean( filename)
    internal_register_path_for_create( local_destination_path)

def internal_install(target, source, destination):
    
    #
    # Add an extra $ in case of referenced environment variable.
    #
    if destination.startswith('$'):
        destination = '$' + destination
        
    print "install: " + target
    print
    print target + ": " + source
    
    print "\t@" + def_MKDIR_RECURSIVE + " " + os.path.dirname(destination)
    
    print "\t" + def_RSYNC + " " + source + " " + destination
    print

def internal_make_target_component(target,casualMakefile):

    casualMakefile = os.path.abspath( casualMakefile)

    USER_CASUAL_MAKE_PATH=os.path.dirname(casualMakefile)
    USER_CASUAL_MAKE_FILE=os.path.basename(casualMakefile)
    
    USER_MAKE_FILE=os.path.splitext(casualMakefile)[0] + ".mk"   
     
    local_unique_target=internal_convert_path_to_target_name(casualMakefile) + "_" + target
    
    
    print target + ": " + local_unique_target
    print
    print local_unique_target + ": " + USER_MAKE_FILE
    print "\t@echo " + casualMakefile + " " + target
    print "\t@" + def_CD + " " + USER_CASUAL_MAKE_PATH + "; $(MAKE) -f " + USER_MAKE_FILE + " " + target
    
def setParallelMake( value):
    def_PARALLEL_MAKE=value
    
def getParallelMake():
    return def_PARALLEL_MAKE

def debug( message):
    if os.getenv("PYTHONDEBUG"): sys.stderr.write( message + "\n")

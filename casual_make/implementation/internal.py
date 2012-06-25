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
def_MKDIR_RECURSIVE="mkdir -p";
def_CHMOD="chmod"

def_MV="mv";

def_Prep="somethinglocal.prep"

#
# Set up casual_make-program. Normally casual_make
#
def_casual_make="casual_make.py"

def_EXPORTCOMMAND="perl somethinglocal.pl";

def_Deploy="make.deploy.ksh"

def_CurrentDirectory=os.getcwd()

def_DependencyDirectory=def_CurrentDirectory + "/dependencies";

def_PARALLEL_MAKE=0
def_LD_LIBRARY_PATH_SET = 0


objectPathsForClean=set()
filesToRemove=set()
pathsToCreate=set()
messages=set()

exportHeaderCommands=set()
exportLibraryCommands=set()
exportFileCommands=set()
exportLibraryTargets=set()

exportSubTarget=set()

targetSequence=1

USER_CASUAL_MAKE_PATH=""
USER_CASUAL_MAKE_FILE=""


#
# Tar bort inledande './' Mest for att det ska 
# se snyggare ut i makefilen.
#
def internal_clean_directory_name(name):
    return str.replace(name, "./", "")

#
# byter ut alla / mot _
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
    print
    print "#"
    print "# Skriver vilka include-paths som nyttjas till filen som miljovariabeln INCLUDE_PATHS_FILE dikterar."
    print "#"
    print "   clean_include_paths = $(subst -I,, $(strip $(INCLUDE_PATHS) $(DEFAULT_INCLUDE_PATHS)))"
    print "print_include_paths:"
    print "\t@for inc in $(clean_include_paths); do \\"
    print "\t\tif [[ $$inc != \"./inc\" ]] ; then \\"
    print "\t\tCURRENT_PATH=`pwd`; if cd $$inc 2>/dev/null; then pwd >> $$INCLUDE_PATHS_FILE; cd $$CURRENT_PATH; else echo \"error: invalid include path: $$inc in $(THIS_MAKEFILE)\" > /dev/stderr; fi\\"
    print "\t\tfi ; \\"
    print "\tdone "
    print

    #
    # Tillhandhall mojlighet att tvinga sekventiell 
    # hantering
    #
    if def_PARALLEL_MAKE < 2:
        print
        print "#"
        print "# Sekventiell hantering gar att tvinga"
        print "#"
        print "ifdef FORCE_NOTPARALLEL"
        print ".NOTPARALLEL:"
        print "endif"
        print
       
    #
    # Se till att skapa targets for "katalog-skapning"
    #
    for path in pathsToCreate:
        print
        print path + ":"
        print "\t-" + def_MKDIR_RECURSIVE + " " + path
        print "\t-" + def_CHMOD + " 777 " + path
        print
    
    #
    # Skapa target for att skapa dependency-katalogen
    #
    print def_DependencyDirectory + ":"
    print "\t-" + def_MKDIR_RECURSIVE + " " + def_DependencyDirectory
    print "\t-" + def_CHMOD + " 777 " + def_DependencyDirectory
    print

    
    #
    # Se till att skapa targets for clean
    #
    
    print
    print "clean:"
    
    for objectpath in objectPathsForClean:
        print "\t-" + def_RM + " " + objectpath + "/*.o"
    
    
    #
    # Ta bort dependency-filerna
    #
    print "\t-" + def_RM + " " + def_DependencyDirectory + "/*.d"
    
    #
    # Ta bort de ovriga registrerade filerna.
    #
    for filename in filesToRemove:
        print "\t-" + def_RM + " " + filename
    
    
    #
    # Generera export-delarna
    #

    if exportHeaderCommands :
        print
        print "export_headers: export_begin"
        print exportHeaderCommands
        
        exportSubTarget.add("export_headers")
    
    if exportLibraryCommands :
        print
        print "export_libraries: export_begin $EXPORT_LIBRARIES_TARGETS"
        print exportLibraryCommands
        
        exportSubTarget.add("export_libraries")
    
    if exportFileCommands :
        print
        print "export_files: export_begin export_headers export_libraries"
        print exportFileCommands
        
        exportSubTarget.add("export_files")
    
    print
    print "export: ",
    for f in exportSubTarget: print f,
    print
    
    
    #
    # Skriver ut alla meddelanden som vi samlat pa oss.
    #
    if messages :
        sys.stderr.write( os.getcwd() + "/" + USER_CASUAL_MAKE_FILE + ":\n" )
        sys.stderr.write( messages + "\n") 


#
# Registreringa av filer och paths som ska skapas/tas bort 
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
# Registrera varningar, som sedan skrivs till stderr
# vid "post"
#

def internal_register_message(message):

    messages.add("\E[33m" + message + "\E[m\n")



#
# Funktioner for att abstrahera "namn-omvandling"
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

    
    tempname= internal_convert_path_to_target_name( name) + "_" + targetSequence
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


def internal_cross_dependecies(objects):

    #
    # Byt ut '.o' mot _crosscompile.o, och lagg dit full path
    #
    return "$(addprefix " + def_CurrentDirectory + "/, $(subst .o,_crosscompile" + def_ObjectSuffix + "," + objects + "))"

def internal_dependency_file_name(sourcefile):

    return def_DependencyDirectory + "/$(subst .cpp,.d,$(notdir " + sourcefile + "))"

def internal_set_LD_LIBRARY_PATH():
    """ """
    if def_LD_LIBRARY_PATH_SET < 1:
        print "#"
        print "# Satter LD_LIBRARY_PATH sa att isolated-tests kan hitta alla beroenden"
        print "# Vi satter bara denna en gang, aven om det ar flera isolated som ska testas"
        print "#"
        print "space :=  "
        print "space +=  "
        print "formattet_library_path = $(subst -L,,$(subst $(space),:,$(LIBRARY_PATHS) $(DEFAULT_LIBRARY_PATHS)))"
        print "LD_LIBRARY_PATH=$(formattet_library_path):$(PLATFORMLIB_DIR)"
        print 
        #
        # Se till sa vi inte skriver ut detta igen
        #
        #def_LD_LIBRARY_PATH_SET=1

def internal_export_file(filename,directory,targetdirectory):

    if directory != "" :
        FILE_PATH=directory + "/" + filename
    else:
        FILE_PATH=filename
    
    #
    # Lagg till commanddot for export. Vi skriver detta sist i makefilen (vid post...)
    #
    exportFileCommands.add("\t-@" + def_EXPORTCOMMAND + " " + filename + " " + FILE_PATH + " " + targetdirectory)

def internal_prepare_old_objectlist(objects):

    for o in objects:
        print "obj/" + o + def_ObjectSuffix
        
#
# Intern hjalpfuntktion for att lanka atmi...
#
def internal_BASE_LinkATMI(atmibuild, name, predirectives, objectfiles, libs, buildserverdirective):

    print "#"
    print "#    Lankar $name"
    
    #
    # Vi maste satta ett annat namn for exekverbara atmi binarers target
    # da det finns imakefiler som anvander samma namn for atmi-binarer och libs
    # i samma imakefil, och atmi-binaren har beroende till libbet -> cirkulara
    # beroenden... Det skulle vara onskvart om man sarskiljde namnen istallet...
    #
    atmi_target_name=name + "_atmi"
    
    internal_library_targets( libs)
    
    DEPENDENT_TARGETS=internal_library_dependencies( libs)
    
    
    
    local_destination_path=internal_clean_directory_name( os.path.dirname( internal_executable_name_path(name)))
    
    print 
    print "all: " + internal_target_name( atmi_target_name)
    print
    print "cross: " + internal_cross_dependecies( objectfiles)
    print 
    print "deploy: $(internal_target_deploy_name $atmi_target_name)"
    print 
    print internal_target_name(atmi_target_name) + ": " + internal_executable_name_path( name) + " | " + local_destination_path
    print
    print "objects_" + atmi_target_name + " = " + internal_object_name_list( objectfiles)
    print "libs_" + atmi_target_name + " = $(addprefix -l, " + libs + ")"
    print
    print "compile: $(objects_" + atmi_target_name + ")"
    print 
    print internal_executable_name_path(name) + ": $(objects_" + atmi_target_name + ") " + DEPENDENT_TARGETS
    print "\t" + atmibuild + " -o " + internal_executable_name_path( name) + " " + predirectives + " -f \"$(objects_" + atmi_target_name + ")\" -f \"$(LIBRARY_PATHS) $(DEFAULT_LIBRARY_PATHS) $(libs_" + atmi_target_name + ")  $(DEFAULT_LIBS)\"" + buildserverdirective + " -f \"$(LINK_DIRECTIVES_EXE)\""
    print
    print "$(internal_target_deploy_name $atmi_target_name):"
    print "\t-@$def_Deploy $(internal_executable_name $name) exe"
    print 
    internal_register_file_for_clean( internal_executable_name_path(name))
    internal_register_path_for_create( local_destination_path)




def internal_print_services_file_dependency(name, services):

    #
    # Vi kollar om services till servern ar angiven som fil. Om sa
    # ser vi till att fa ett beroende till denna fil, for att buildserver och 
    # omlankning ska ske om anvandaren forandrar tjansteutbudet.
    #
    if services == "@*" :
        print "$(internal_executable_name_path $name): $services#*@"
        print  

def internal_library_targets( libs):

    print
    
    #
    # Skapa dummys som ser till att ingen omlankning sker om
    # libbet inte byggs i denna makefil
    #
    print "# INTERMEDIATE-deklaration av targets, for att gmake ska acceptera targets";
    print "# om de inte finns definerade \"pa riktigt\" i denna makefil.";
    print "# Vi maste aven ta med archive_targets da vi inte vet om det ar ett arkiv eller inte";
    for lib in libs.split():
        print ".INTERMEDIATE: " + internal_target_name(lib)
        print ".INTERMEDIATE: " + internal_archive_target_name(lib)
    print
    
    #
    # Skapa tomma targets for att beroendendet ovan ska funka    
    #
    print "# dummy targets, som kickar in om inte beroendet finns i denna makefil";
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

    print "#"
    print "#    Lankar $name"
    
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
    print filename + ": $(objects_" + name + ") " + DEPENDENT_TARGETS + " | " + local_destination_path

    if linker == "ARCHIVER":
        print "\t" + linker + " " + linkdirectives + " " + filename  + " $(objects_" + name + ")"
    else:
        print "\t" + linker + " -o " + filename + " $(objects_" + name + ") $(LIBRARY_PATHS) $(DEFAULT_LIBRARY_PATHS) $(libs_" + name + ") $(DEFAULT_LIBS) " + linkdirectives
    print
    
    internal_register_file_for_clean( filename)
    internal_register_path_for_create( local_destination_path)


def internal_make_target_component(target,casualMakefile):

    USER_CASUAL_MAKE_PATH=os.path.dirname(casualMakefile)
    USER_CASUAL_MAKE_FILE=os.path.basename(casualMakefile)
    
    USER_MAKE_FILE=os.path.splitext(USER_CASUAL_MAKE_FILE)[0] + ".mk"   
     
    local_unique_target=internal_convert_path_to_target_name(casualMakefile) + "_" + target
    
    #
    # Fixar till sa det blir lite snyggare targets
    #
    #local_unique_target=$local_unique_target//\//_
    #local_unique_target=$local_unique_target//./
    
    print target + ": " + local_unique_target
    print
    print local_unique_target + ": " + USER_CASUAL_MAKE_PATH + "/" + USER_MAKE_FILE
    print "\t" + def_CD + " " + USER_CASUAL_MAKE_PATH + "; $(MAKE) -f " + USER_MAKE_FILE + " " + target
    
def setParallelMake( value):
    def_PARALLEL_MAKE=value
    
def getParallelMake():
    return def_PARALLEL_MAKE

def debug( message):
    if os.getenv("PYTHONDEBUG"): sys.stderr.write( message + "\n")

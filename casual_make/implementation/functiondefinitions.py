'''
Created on 28 apr 2012

@author: hbergk

Contains the basic syntax to produce makefile
'''

#
# Imports
#
from internal import *



#
# 
#


def NoDefaultLibs():
    """
######################################################################
## 
## NoDefaultLibs()
##
## Makes sure nothing is linked default
##
######################################################################
    """

    print "DEFAULT_LIBS :=";
    print


def NoDefaultIncludePaths():
    """
######################################################################
## 
## NoDefaultIncludePaths()
##
## Makes sure that no default include paths are set
## Do not interfere with INCLUDE_PATHS set by users 
##
######################################################################
    """
    print "DEFAULT_INCLUDE_PATHS :=";
    print


def NoDefaultLibraryPaths():
    """
######################################################################
## 
## NoDefaultLibraryPaths()
##
## Makes sure that no default library paths are set
## Do not interfere with LIBRARY_PATHS set by users 
##
######################################################################
    """
    print "DEFAULT_LIBRARY_PATHS :="
    print



def NoParallel():
    """
######################################################################
## 
## NoParallel()
##
## Makes sure that noting is executed parallel in this makefile
##
######################################################################
    """
    if getParallelMake() == 0 :
        #
        # Detta kan tyckas lite markligt, men vi vill att "not-parallel", dvs
        # sekventiellt, ska alltid kicka in om anvandaren har valt det.
        #
        print
        print "#"
        print "# Parallelitet gar att overrida"
        print "# NOTPARALLEL har alltid fortur, om vi far"
        print "# indikation pa bade parallel och not-parallel"
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
######################################################################
## 
## Parallel()
##
## Makes sure that execution is parallel in this makefile
## This statement has to be first in this makefile
##
######################################################################
    """
    setParallelMake( 1)



def CompileDirective(sourcefile,objectfile,directive):
    """
#####################################################################
##
## CompileDirective(sourcefile,objectfile,directive)
##
## kompilerar en kllkodsfil till en objektfil
##
## sourcefile    Namnet kllkodsfilen, inklusive path och filndelse (src/minfil.cpp)
##    
## objectfile    den resulterade objektfilen inklusinve path och filndelse (obj/minfil.o)
##
## directive   Kompileringsdirektiv just for detta TU
##
######################################################################
    """

    local_object_path=def_CurrentDirectory + "/" + internal_clean_directory_name( os.path.dirname( objectfile))
    local_dependency_file=internal_dependency_file_name( sourcefile)
    
    local_cross_object_file=internal_cross_object_name( objectfile)
    
    local_source_file=def_CurrentDirectory + "/" + sourcefile
    local_object_file=def_CurrentDirectory + "/" + objectfile
    
    print "#"
    print "# compiling {0} to {1}".format( sourcefile, objectfile)
    print
    print "-include " + local_dependency_file
    print 
    print local_object_file + ": " + local_source_file + " | " + def_DependencyDirectory +" " + local_object_path                                                                        
    print "\t$(COMPILER) -o {0} {1}  $(INCLUDE_PATHS) $(DEFAULT_INCLUDE_PATHS) $(COMPILE_DIRECTIVES) {2}".format(objectfile, local_source_file, directive )
    print "\t@$(HEADER_DEPENDENCY_COMMAND) -MT '{0} {1}' $(INCLUDE_PATHS) $(DEFAULT_INCLUDE_PATHS) {2} -MF {3}".format(local_cross_object_file, local_object_file, local_source_file, local_dependency_file)
    print 
    print local_cross_object_file + ": " + local_source_file + " | " + def_DependencyDirectory + " " + local_object_path                                                                      
    print "\t$(CROSSCOMPILER) $(CROSS_COMPILE_DIRECTIVES) -o " + local_cross_object_file + " " + local_source_file + "$(INCLUDE_PATHS) $(DEFAULT_INCLUDE_PATHS) "
    print
    
    internal_register_object_path_for_clean( local_object_path)
    internal_register_path_for_create( local_object_path)

    

def Compile(sourcefile,objectfile):
    """
#####################################################################
##
## Compile(sourcefile,objectfile)
##
## kompilerar en kllkodsfil till en objektfil
##
## sourcefile    Namnet kllkodsfilen, inklusive path och filndelse (src/minfil.cpp)
##    
## objectfile    den resulterade objektfilen inklusinve path och filndelse (obj/minfil.o)
##
######################################################################
    """

    CompileDirective( sourcefile, objectfile, "")

    




def LinkAtmiServer(name,objectfiles,libs,services):
    """
######################################################################
##
## LinkAtmiServer(name,objectfiles,libs,services)
##
## Lnkar en ATMI-server
##
## name        Namnet p exekverbara utan fil-suffix.
##    
## objects    Objektfiler som ska lnkas
##
## libs        libs som exekverbara har beroende mot.
##
## services De tjnster som servern exponerar. Ex "tjanst1 tjanst2"
##
######################################################################
    """

    internal_BASE_LinkATMI( "$(BUILDSERVER)", name, "-s " + services, "$(DEFAULTSRVMGR)", objectfiles, libs, "")
    internal_print_services_file_dependency( name, services)




def LinkAtmiServerResource(name,objectfiles,libs,services,resource):
    """
######################################################################
##
## LinkAtmiServerResource( name, objectfiles, libs, services, resource)
##
## Lankar en ATMI-server mot en resurs
##
## name      Namnet pa exekverbara utan fil-suffix.
## 
## objects  Objektfiler som ska lankas inklusive path och filandelser (obj/bla.o)
##
## libs      libs som exekverbara har beroende mot.
##
## services tjanster som servern har realiserat
##
## resource    Resurs som servern ska interagera med.
##
######################################################################
    """
    internal_BASE_LinkATMI( "$(BUILDSERVER) ", name, " -s " + services + " $(DEFAULTSRVMGR) ", objectfiles, libs, " -r " + resource)
    internal_print_services_file_dependency( name, services)


def LinkAtmiServerMultipleResources(name,objectfiles,libs,services,resources):
    """
######################################################################
##
## LinkAtmiServerMultipleResources( name, objectfiles, libs, services, resources)
##
## Lankar en ATMI-server mot multipla resurser
##
## name      Namnet pa exekverbara utan fil-suffix.
## 
## objects  Objektfiler som ska lankas inklusive path och filandelser (obj/bla.o)
##
## libs      libs som exekverbara har beroende mot.
##
## services tjanster som servern har realiserat
##
## resources    [1..N] (unika) Resurser som servern ska interagera med. Alltsa
##                 om man interagerar med tva db2-resurser sa ska endast en resurs anges (TMSUDB2)
##
######################################################################
    """
    internal_BASE_LinkATMI("$(BUILDSERVER) ", name, " -s " + services + " $(DEFAULTSRVMGR) ", objectfiles, libs, " -M $(addprefix -r , " + resources + ")")
    internal_print_services_file_dependency( name, services)





def LinkAtmiClient(name,objectfiles,libs):
    """
######################################################################
##
## LinkAtmiClient(name,objectfiles,libs)
##
## Lankar en ATMI-klient
##
## name        Namnet pa exekverbara utan fil-suffix.
##    
## objects    Objektfiler som ska lankas
##
## libs        libs som exekverbara har beroende mot.
##
######################################################################
    """

    internal_BASE_LinkATMI( "$(BUILDCLIENT)",name, "", objectfiles, libs, "")


def LinkLibrary(name,objectfiles,libs):
    """
######################################################################
## 
## LinkLibrary(name,objectfiles,libs)
##
## Lankar ett lib
##
## name        Namnet pa libbet utan lib-prefix och .so - andelse
##    
## objectfiles    Objektfiler som ska lankas, inklusive path och suffix (/obj/bla.o)
##
## libs        Andra libbs som libbet har beroende mot.
##
######################################################################
    """
    internal_base_link("$(LIBRARY_LINKER)", name, internal_shared_library_name_path( name), objectfiles, libs, " $(LINK_DIRECTIVES_LIB)")
    
    print internal_target_deploy_name( name) + ":"
    print "\t@" + def_Deploy + " " + internal_shared_library_name(name) + " lib"
    print 
        


def LinkArchive(name,objectfiles):
    """
######################################################################
## 
##  LinkArchive(name,objectfiles)
##
## Lankar ett arkiv.
##
## name        Namnet pa arkivet utan lib-prefix och .a - andelse
##    
## objectfiles    Objektfiler som ska lankas, inklusive path och suffix (/obj/bla.o)
##
## libs        Andra libbs som arkivet har beroende mot.
##
######################################################################
    """


    print "#"
    print "#    Lankar "+ name
    
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


def LinkExecutable(name,objectfiles,libs):
    """
######################################################################
## 
## LinkExecutable(name,objectfiles,libs)
##
## Lankar en exekverbar
##
## name        Namnet pa exekverbara utan fil-suffix.
##    
## objectfiles Objektfiler som ska lankas, inklusive path och suffix (/obj/bla.o)
##
## libs        libs som exekverbara har beroende mot.
##
######################################################################
    """
    internal_base_link("$(EXECUTABLE_LINKER)", name, internal_executable_name_path( name), objectfiles, libs, "$(LINK_DIRECTIVES_EXE)")
    
    print internal_target_deploy_name(name) + ":"
    print "\t-@" + def_Deploy + " " + internal_executable_name(name) + " exe"
    print 


def Build(casualMakefile):
    """
######################################################################
## 
## Build(casualMakefile)
##
## "builds" another casual-make-file: jumps to the spcific file and execute make
##
## casualMakefile    The file to build
##
######################################################################
    """
    
    #
    # Se till sa vi kor sekventiellt default.
    #
    NoParallel()
    
    USER_CASUAL_MAKE_PATH=os.path.dirname( casualMakefile)
    USER_CASUAL_MAKE_FILE=os.path.basename( casualMakefile)
    
    USER_MAKE_FILE=os.path.splitext(USER_CASUAL_MAKE_FILE)[0] + ".mk"
    
    local_make_target=internal_convert_path_to_target_name(casualMakefile)
    

    print "#"
    print "# If " + USER_CASUAL_MAKE_FILE + " is newer than " + USER_MAKE_FILE + " , a new makefile is produced"
    print "#"
    print USER_CASUAL_MAKE_PATH +"/" + USER_MAKE_FILE + ": " + USER_CASUAL_MAKE_PATH + "/" + USER_CASUAL_MAKE_FILE
    print "\t" + def_CD + " " + USER_CASUAL_MAKE_PATH + ";" + def_casual_make + " " + USER_CASUAL_MAKE_FILE
    print
    internal_make_target_component("all", casualMakefile)
    print
    internal_make_target_component("cross", casualMakefile)
    print
    internal_make_target_component("prep", casualMakefile)
    print
    internal_make_target_component("deploy", casualMakefile)
    print
    internal_make_target_component("test", casualMakefile)
    print
    internal_make_target_component("clean", casualMakefile)
    print
    internal_make_target_component("compile", casualMakefile)
    print
    internal_make_target_component("print_include_paths", casualMakefile)
   
    #
    # Fixar till sa det blir lite snyggare targets
    #
    #TODO:
    #local_make_target=local_make_target + "//\//_"
   
    print
    print "#"
    print "# Always produce " + USER_MAKE_FILE + " , even if " + USER_CASUAL_MAKE_FILE + " is older."
    print "#"
    print "make: " + local_make_target
    print
    print local_make_target + ":"
    print "\t" + def_CD + " " + USER_CASUAL_MAKE_PATH + ";" + def_casual_make + " " + USER_CASUAL_MAKE_FILE
    print "\t" + def_CD + " " + USER_CASUAL_MAKE_PATH + ";" +  "$(MAKE) -f " + USER_MAKE_FILE + " make"
    print
    
    




def BuildComponent(component,exportflags):
    """
######################################################################
## 
## BuildComponent(component,exportflags)
##
## Bygger "komponent" beskriven i en annan makefile
##
## component    "path" till komponenten (kan vara ".")
##
## exportflags        Vet inte riktigt vad detta ar...
##
######################################################################
    """
    pass
    #BuildPartOfComponent(component, "", exportflags)



def ExportHeaderFileWithDirectory(filename,directory,targetdirectory):
    """
######################################################################
## 
## ExportHeaderFileWithDirectory(filename,directory,targetdirectory)
##
## Exports the component to the path specified
##
## sourceFilePath    relative file path
##
## targetdirectory same as ExportHeaderFile::path
##
######################################################################
    """
    #internal_export_file "$filename" "$directory" "$targetdirectory/inc"
    exportHeaderCommands.add("\t-@" + def_EXPORTCOMMAND + " " + filename +  directory + "/" + filename + " " + targetdirectory + "/inc")





def ExportSharedLibrary(name,targetdirectory):
    """
######################################################################
##
## ExportSharedLibrary(name,path)
##
## Exporterar libbet till path
##
## name    namnet pa libbet
##
## targetdirectory    path dit "name" exporteras
##
######################################################################    """
    exportLibraryCommands.add("\t-@" + def_EXPORTCOMMAND +" " + internal_shared_library_name( name) + "bin/" + internal_shared_library_name(name) + targetdirectory +"/lib")
    
    #
    # Vi spar pa oss target-names for de libs som ska exporteras
    # sa att vi pa sa satt har mojlighet att enbart bygga dessa
    # Detta framst for att kunna bygga samtliga subdomaner utan att 
    # behova bry oss beroenden mellan dessa
    #
    exportLibraryTargets.add( internal_target_name( name))


def LinkIsolatedUnittest(name,objectfiles,libs):
    """
######################################################################
##
## Bygger unittest som ar tankt att koras pa compile-maskinen.
## Alltsa, det ska vara isolerade tester for att testa av funkitonalitet
## som inte har nagon koppling till nagon testmiljo eller liknande.
##
## Det skapas ett target for att kora testen
##
## name: namn pa unittestbinaren
##
## objectfiles: Objektfilerna som ska lankas. Fulla objektfilsnamn ex. obj/minobejektfil.o
##
## libs: Libs som binaren ar beroende av.
##
######################################################################
    """

    internal_base_link( "$(EXECUTABLE_LINKER)", name, internal_executable_name_path(name), objectfiles, libs, "$(ISOLATED_UNITTEST_LIB) $(LINK_DIRECTIVES_EXE)")

    print internal_target_deploy_name(name) + ":"
    print "\t@" + def_Deploy + " " + internal_executable_name( name) + " client"
    print 
    
    internal_set_LD_LIBRARY_PATH()
    
    print "test: " + internal_target_isolatedunittest_name( name)    
    print
    print internal_target_isolatedunittest_name(name) + ": " +  internal_executable_name_path(name)
    print "\t @LD_LIBRARY_PATH=$(LD_LIBRARY_PATH) " + internal_executable_name_path( name) + " $(ISOLATED_UNITTEST_DIRECTIVES)"
    print 






def LinkDependentUnittest(name,objectfiles,libs):
    """
######################################################################
##
## Bygger unittest som ar beroende av "miljo"-delar.
## Alltsa, det ska vara tester for att testa av verksamhetsfunkitonalitet
## som har nagon koppling till nagon testmiljo eller liknande.
##
## Mer eller mindre motsattsen till LinkIsolatedUnittest
##
## name: namn pa unittestbinaren
##
## objectfiles: Objektfilerna som ska lankas  Fulla objektfilsnamn ex. obj/minobejektfil.o
##
## libs: Libs som binaren ar beroende av.
##
######################################################################
    """
    internal_BASE_LinkATMI("$(BUILDCLIENT)", name, "" , objectfiles, libs , "-f $(DEPENDENT_UNITTEST_LIB)")
    


######################################################################
##
## TargetExecute
##
## Skapar ett target (vilket kan var nagot som aven produceras fran
## andra 'macron', ex test) 
## Nar target kors sa exekveras den exekverbara.
##
## target: namn for target. Detta bor vara nogot av de befintliga target:s
##
## executable: fullt kvalificerande namn pa den exekverbara
##
## options: eventuellt options till den exekverbara
##
######################################################################
def TargetExecute(target,executable,options):
    """
######################################################################
##
## TargetExecute
##
## Skapar ett target (vilket kan var nagot som aven produceras fran
## andra 'macron', ex test) 
## Nar target kors sa exekveras den exekverbara.
##
## target: namn for target. Detta bor vara nogot av de befintliga target:s
##
## executable: fullt kvalificerande namn pa den exekverbara
##
## options: eventuellt options till den exekverbara
##
######################################################################
    """

    internal_set_LD_LIBRARY_PATH()
    
    local_target_name=internal_unique_target_name(executable)
    

    print target + ": " + local_target_name    
    print
    
    print local_target_name + ": " + internal_normalize_path(executable)
    print "\t@" + executable, options
    print 


 
def DatabasePrepare(database,username,password,filename,bindname):
    """
######################################################################
##
## DatabasePrepare(database,username,password,filename,bindname)
##
##
## Special (hack) handling of bind files!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
## the path name must be short, so the bind files are copied to \$(TMP)
## before bind. Restriction in bind.
##
##
## Special (temporary) fix to handle .pc files that really should be
## .sqc files. .pc file are copied to .sqc
##
##
## Precompiles a pc file, produces a bind file and binds it agains
## the database.
##
## database     Name of the database
## username     Name of the user
## password     The users password
## filename     The name of the pc file, without extension
## bindname     The package name without extension
##
## 2001-06-20/lsu
## BindFileName now make use of a shell script, rfvprep. The only
## reason for this change is that rfvprep can take care of db2 prep
## warnings (error code 2).
##
######################################################################
    """
    local_bind_path=os.path.dirname( internal_bind_name_path( bindname))
    print
    print "prep all: $(internal_bind_name_path $bindname)"
    print
    print "$(internal_bind_name_path $bindname): $def_CurrentDirectory/src/$filename.cpp"
    print
    print "$def_CurrentDirectory/src/$filename.cpp: $def_CurrentDirectory/src/$filename.sqc | $local_bind_path"
#    print "    $def_Prep "$database" "$username" "$password" $def_CurrentDirectory/src/$filename.sqc $(internal_bind_name_path $bindname) \$(PREXTRA_HOST_PATHS)"
    print "    $def_MV $def_CurrentDirectory/src/$filename.c $def_CurrentDirectory/src/$filename.cpp"
    print

    internal_register_file_for_clean(internal_bind_name_path(bindname))
    internal_register_file_for_clean( def_CurrentDirectory + "/src/" + filename + ".cpp")
    
    print
    print "deploy: $(internal_target_deploy_name $bindname)"
    print 
    print "$(internal_target_deploy_name $bindname):"
    print "    -@$def_Deploy $(internal_bind_name $bindname) bnd"    


######################################################################
## 
## Prepare(filename,bindname)
##
## Producerar bind-filen med den kompilerade sql-koden och 
## den genererade kallkodsfilen utifran sqc-filen. 
## Kallkodsfilen kommer att ha samma namn och path som sqc-filen, men med 
## filandelsen .cpp.
##
## filename     sqc-filnamnet, inklusive path och filandelse.
##                Ex. src/perpersoninfosfhanteraredb.sqc
##
## packagename Paketnamnet som den kompilearde sql-koden ligger inom.
##                        bind-filen kommer att ha samma namn, men med filandelsen
##                        bnd. Ex. perpersoninfo, vilket resulterar i bind-filen
##                obj/perpersoninfo.bnd.
##
######################################################################
## TODO Implementera



        
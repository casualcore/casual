

#
# Default libs
#
DEFAULT_LIBS :=  


######################################################################
## 
## compilation and link configuration
##
######################################################################



COMPILER = CC
CROSSCOMPILER = g++

#
# We make sure we use bash
#
SHELL = bash

#
# Vi särskiljer mellan lib och exe enbart för purify och co.
#
LIBRARY_LINKER = CC

ifndef EXECUTABLE_LINKER
EXECUTABLE_LINKER = CC
endif

#
# Nu stter vi ihop de faktiska kompilator/lnk-direktiven
#
ifdef DEBUG
   COMPILE_DIRECTIVES = -mt -g -c -KPIC -DRFV_DLL -DRFV_DEBUG 
   LINK_DIRECTIVES_LIB = -mt -g -G -KPIC -z now
   LINK_DIRECTIVES_EXE = -mt -g -KPIC -z now
   LINK_DIRECTIVES_ARCHIVE = -mt -g -KPIC -z now -xar
else
   COMPILE_DIRECTIVES = -mt -c -O3 -KPIC -DRFV_DLL
   LINK_DIRECTIVES_LIB = -mt -G -O3 -KPIC -z now
   LINK_DIRECTIVES_EXE = -mt -O3 -KPIC -z now
   LINK_DIRECTIVES_ARCHIVE = -mt -O3 -KPIC -z now -xar
endif

#
# Addera det användaren har definerat
#
COMPILE_DIRECTIVES := $(COMPILE_DIRECTIVES) $(EXTRA_CC_FLAGS)
LINK_DIRECTIVES_LIB := $(LINK_DIRECTIVES_LIB) $(EXTRA_LINK_FLAGS)
LINK_DIRECTIVES_EXE := $(LINK_DIRECTIVES_EXE) $(EXTRA_LINK_FLAGS)

CROSS_COMPILE_DIRECTIVES = -g -c -Wall -pedantic -Wno-long-long -DNOWHAT -DRFV_DLL -DRFV_DEBUG

BUILDSERVER = CC='$(EXECUTABLE_LINKER)' $(TUXDIR)/bin/buildserver -v
BUILDCLIENT = CC='$(EXECUTABLE_LINKER)' $(TUXDIR)/bin/buildclient -v

#
# Purify
#
PURIFYOPTIONS = -always-use-cache-dir=yes -lazy-load=no
PURIFY_LINKER = purify purecov $(PURIFYOPTIONS) $(EXECUTABLE_LINKER) -lc -R$(TUXLIB_DIR)
QUANTIFY_LINKER = quantify -always-use-cache-dir $(EXECUTABLE_LINKER) -R$(TUXLIB_DIR)


#
# Användarens definerade paths läggs först för ha företräde
# före default.
# DB2 måste inkluderas före Tuxedo.
#
INCLUDE_PATHS := $(EXTRA_INCLUDE_PATHS) $(DB2INCLUDE_DIR) $(TUXINCLUDE_DIR) $(RFVFML)
LIBRARY_PATHS := $(EXTRA_LIB_PATHS) /usr/lib /usr/local/lib $(DB2LIB_DIR) $(TUXLIB_DIR)

#
# Default include/library-paths
#
DEFAULT_INCLUDE_PATHS := ./inc $(EXPORT_INF)/inc $(EXPORT_KUI)/inc $(EXPORT_KND)/inc $(EXPORT_RGL)/inc
DEFAULT_LIBRARY_PATHS := ./bin $(EXPORT_INF)/lib $(EXPORT_KUI)/lib $(EXPORT_KND)/lib $(EXPORT_RGL)/lib $(XERCES_LIBRARY_PATHS)
 


#
# Sätt inledande växel för kompilatorn och länkaren.
# 
INCLUDE_PATHS := $(addprefix -I, $(INCLUDE_PATHS) )
LIBRARY_PATHS := $(addprefix -L, $(LIBRARY_PATHS) )
DEFAULT_INCLUDE_PATHS := $(addprefix -I, $(DEFAULT_INCLUDE_PATHS) )
DEFAULT_LIBRARY_PATHS := $(addprefix -L, $(DEFAULT_LIBRARY_PATHS) )

#
# För att generera "header-dependencies"
# Ser till så vi inte får med beroende till db2 och Tuxedo.
#
HEADER_DEPENDENCY_COMMAND = -/usr/sfw/bin/g++ -MP -MM -DRFV_DLL -isystem $(DB2INCLUDE_DIR) -isystem $(TUXINCLUDE_DIR)






######################################################################
## 
## Det transformerade innehållet i imakefilen följer:
##
######################################################################


    


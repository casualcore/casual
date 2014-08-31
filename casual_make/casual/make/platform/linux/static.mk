
#
# Common stuff
#
include $(CASUALMAKE_PATH)/casual_make/casual/make/platform/common.mk

#
# Default libs
#
DEFAULT_LIBS :=  


######################################################################
## 
## compilation and link configuration
##
######################################################################

THIS_MAKEFILE = $(CURDIR)/$(word $(words $(MAKEFILE_LIST)),$(MAKEFILE_LIST))



COMPILER = g++
CROSSCOMPILER = clang++

#
# Vi �r beroende av att vi anv�nder bash
#
SHELL = bash

#
# Vi s�rskiljer mellan lib och exe enbart f�r purify och co.
#
LIBRARY_LINKER = g++
ARCHIVE_LINKER = ar rcs

ifndef EXECUTABLE_LINKER
EXECUTABLE_LINKER = g++
endif

#
# Nu stter vi ihop de faktiska kompilator/lnk-direktiven
#
ifdef DEBUG
   COMPILE_DIRECTIVES = -g -pthread -c  -fpic -Wall -pedantic -Wno-long-long -Wno-variadic-macros -std=c++11
   LINK_DIRECTIVES_LIB = -g -pthread -shared  -fpic
   LINK_DIRECTIVES_EXE = -g -pthread  -fpic
   LINK_DIRECTIVES_ARCHIVE = -g  

   ifdef ANALYZE
      COMPILE_DIRECTIVES := $(COMPILE_DIRECTIVES) -O0 -coverage
      LINK_DIRECTIVES_LIB := $(LINK_DIRECTIVES_LIB) -O0 -coverage
      LINK_DIRECTIVES_EXE := $(LINK_DIRECTIVES_EXE) -O0 -coverage
   endif

else
   COMPILE_DIRECTIVES = -pthread -c -O3 -fpic -std=c++11
   LINK_DIRECTIVES_LIB = -pthread -shared -O3 -fpic
   LINK_DIRECTIVES_EXE = -pthread -O3 -fpic 
   LINK_DIRECTIVES_ARCHIVE = 
endif

#
# Addera det anv�ndaren har definerat
#
COMPILE_DIRECTIVES := $(COMPILE_DIRECTIVES) $(EXTRA_CC_FLAGS)
LINK_DIRECTIVES_LIB := $(LINK_DIRECTIVES_LIB) $(EXTRA_LINK_FLAGS)
LINK_DIRECTIVES_EXE := $(LINK_DIRECTIVES_EXE) $(EXTRA_LINK_FLAGS)

CROSS_COMPILE_DIRECTIVES = -g -c -Wall -pedantic -fcolor-diagnostics -Wno-long-long -Wno-variadic-macros -DNOWHAT -std=c++11

BUILDSERVER = casual-build-server -c $(EXECUTABLE_LINKER) 
BUILDCLIENT = CC='$(EXECUTABLE_LINKER)' $(CASUALMAKE_PATH)/bin/buildclient -v

#
# Purify
#
PURIFYOPTIONS = -always-use-cache-dir=yes -lazy-load=no
PURIFY_LINKER = purify purecov $(PURIFYOPTIONS) $(EXECUTABLE_LINKER) -lc -R$(TUXLIB_DIR)
QUANTIFY_LINKER = quantify -always-use-cache-dir $(EXECUTABLE_LINKER) -R$(TUXLIB_DIR)

#
# VALGRIND
#
ifdef VALGRIND
VALGRIND_CONFIG=valgrind --xml=yes --xml-file=valgrind.xml
else
VALGRIND_CONFIG=
endif

#
# Anv�ndarens definerade paths l�ggs f�rst f�r ha f�retr�de
# f�re default.
# DB2 m�ste inkluderas f�re Tuxedo.
#
INCLUDE_PATHS := $(INCLUDE_PATHS) 
BASE_LIBRARY_PATHS := $(LIBRARY_PATHS) /lib64 /usr/lib64 /usr/local/lib64

#
# Default include/library-paths
#
DEFAULT_INCLUDE_PATHS := ./inc 
BASE_DEFAULT_LIBRARY_PATHS := ./bin  


#
# S�tt inledande v�xel f�r kompilatorn och l�nkaren.
# 
LIBRARY_PATH_DIRECTIVE=-Wl,-rpath-link=


HEADER_DEPENDENCY_COMMAND = -g++ -MP -MM -std=c++11



######################################################################
## 
## Det transformerade inneh�llet i imakefilen f�ljer:
##
######################################################################
    
   



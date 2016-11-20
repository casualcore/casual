
#
# Default libs
#
DEFAULT_LIBS :=  


######################################################################
## 
## compilation and link configuration
##
######################################################################


ifndef CXX
CXX = g++
endif


COMPILER = $(CXX)
CROSSCOMPILER = clang++

WARNING_DIRECTIVE = -Wall -pedantic -Wsign-compare -Werror=return-type -Wextra -Winit-self -Woverloaded-virtual -Wno-unused-parameter -Wno-missing-field-initializers

#
# Linkers
#
LIBRARY_LINKER = $(CXX)
ARCHIVE_LINKER = ar rcs


#
# We make sure we use bash
#
SHELL = bash


ifndef EXECUTABLE_LINKER
EXECUTABLE_LINKER = g++
endif

#
# Compile and link directives
#
ifdef DEBUG
   COMPILE_DIRECTIVES = -g -pthread -c  -fpic $(WARNING_DIRECTIVE) -Wno-variadic-macros -std=c++11
   LINK_DIRECTIVES_LIB = -g -pthread -shared  -fpic
   LINK_DIRECTIVES_EXE = -g -pthread  -fpic
   LINK_DIRECTIVES_ARCHIVE = -g  

   ifdef ANALYZE
      COMPILE_DIRECTIVES := $(COMPILE_DIRECTIVES) -O0 -coverage
      LINK_DIRECTIVES_LIB := $(LINK_DIRECTIVES_LIB) -O0 -coverage
      LINK_DIRECTIVES_EXE := $(LINK_DIRECTIVES_EXE) -O0 -coverage
   endif

else
   COMPILE_DIRECTIVES = -pthread -c -O3 -fpic -std=c++11 $(WARNING_DIRECTIVE)
   LINK_DIRECTIVES_LIB = -pthread -shared -O3 -fpic $(WARNING_DIRECTIVE)
   LINK_DIRECTIVES_EXE = -pthread -O3 -fpic $(WARNING_DIRECTIVE)
   LINK_DIRECTIVES_ARCHIVE = 
endif


CROSS_COMPILE_DIRECTIVES = -g -c -Wall -pedantic -fcolor-diagnostics -Wno-long-long -Wno-variadic-macros -DNOWHAT -std=c++11

BUILDSERVER = casual-build-server -c $(EXECUTABLE_LINKER) 
#BUILDCLIENT = CC='$(EXECUTABLE_LINKER)' $(CASUALMAKE_PATH)/bin/buildclient -v


#
# VALGRIND
#
ifdef VALGRIND
PRE_UNITTEST_DIRECTIVE=valgrind --xml=yes --xml-file=valgrind.xml
endif


#
# Format the include-/library- paths
# 
LIBRARY_PATH_OPTION=-Wl,-rpath-link=

INCLUDE_PATHS_DIRECTIVE := $(addprefix -I, $(INCLUDE_PATHS) )
LIBRARY_PATHS_DIRECTIVE := $(addprefix -L, $(LIBRARY_PATHS) ) $(addprefix $(LIBRARY_PATH_OPTION), $(LIBRARY_PATHS) )
DEFAULT_INCLUDE_PATHS_DIRECTIVE := $(addprefix -I, $(DEFAULT_INCLUDE_PATHS) )
DEFAULT_LIBRARY_PATHS_DIRECTIVE := $(addprefix -L, $(DEFAULT_LIBRARY_PATHS) ) $(addprefix $(LIBRARY_PATH_OPTION), $(DEFAULT_LIBRARY_PATHS) )


#
# Header dependency stuff
#
HEADER_DEPENDENCY_COMMAND = -g++ -MP -MM -std=c++11


#
# Directive for setting SONAME
#
LINKER_SONAME_DIRECTIVE = -Wl,-soname,
    
   



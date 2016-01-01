


#
# Default libs
#
DEFAULT_LIBS :=  


######################################################################
## 
## compilation and link configuration
##
######################################################################



COMPILER = g++
CROSSCOMPILER = clang++

#
# Linkers
#
LIBRARY_LINKER = g++
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
   COMPILE_DIRECTIVES = -ggdb -c -fPIC -Wall -pedantic -std=c++11
   LINK_DIRECTIVES_LIB =  -ggdb -dynamiclib -fPIC
   LINK_DIRECTIVES_EXE =  -ggdb -fPIC
   LINK_DIRECTIVES_ARCHIVE =  -ggdb -fPIC
   
   ifdef ANALYZE
      COMPILE_DIRECTIVES := $(COMPILE_DIRECTIVES) -fprofile-arcs -ftest-coverage -std=c++11
      LINK_DIRECTIVES_LIB := $(LINK_DIRECTIVES_LIB) -fprofile-arcs
      LINK_DIRECTIVES_EXE := $(LINK_DIRECTIVES_EXE) -lgcov -fprofile-arcs
   endif
   
else
   COMPILE_DIRECTIVES =  -c -O3 -fPIC -Wall -pedantic -Wno-unused-parameter -std=c++11 -pthread
   LINK_DIRECTIVES_LIB =  -dynamiclib -O3 -fPIC -Wall -pedantic -std=c++11
   LINK_DIRECTIVES_EXE =  -O3 -fPIC -Wall -pedantic -std=c++11
   LINK_DIRECTIVES_ARCHIVE = -O3 -fPIC -Wall -pedantic -std=c++11 -pthread
endif

CROSS_COMPILE_DIRECTIVES = -c -g -Wall -pedantic -fcolor-diagnostics -DNOWHAT -std=c++11 -stdlib=libc++ -U__STRICT_ANSI__ -DGTEST_USE_OWN_TR1_TUPLE=1




#
# VALGRIND
#
ifdef VALGRIND
PRE_UNITTEST_DIRECTIVE=valgrind --xml=yes --xml-file=valgrind.xml
endif



#
# Format the include-/library- paths
# 
INCLUDE_PATHS := $(addprefix -I, $(INCLUDE_PATHS) )
LIBRARY_PATHS := $(addprefix -L, $(LIBRARY_PATHS) )
DEFAULT_INCLUDE_PATHS := $(addprefix -I, $(DEFAULT_INCLUDE_PATHS) )
DEFAULT_LIBRARY_PATHS := $(addprefix -L, $(DEFAULT_LIBRARY_PATHS) )

#
# Header dependency stuff
#
HEADER_DEPENDENCY_COMMAND = -g++ -MP -M -std=c++11

#
# Directive for setting SONAME
#
LINKER_SONAME_DIRECTIVE = -Wl,-dylib_install_name,


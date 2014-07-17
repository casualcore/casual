

def casual_make_platform_configuration():

    print """

######################################################################
## 
## "global" variables that can be used and altered by the user
##
######################################################################



#
# Unittest
UNITTEST_INCLUDE_PATH = $(CASUALMAKE_PATH)/unittest/gtest/include
UNITTEST_LIBRARY_PATH = $(CASUALMAKE_PATH)/unittest/gtest/bin
ISOLATED_UNITTEST_LIB = $(UNITTEST_LIBRARY_PATH)/libgtest.a

ISOLATED_UNITTEST_DIRECTIVES := $(ISOLATED_UNITTEST_DIRECTIVES) --gtest_color=yes

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
   COMPILE_DIRECTIVES =  -c -O3 -fPIC -Wall -pedantic -std=c++11
   LINK_DIRECTIVES_LIB =  -dynamiclib -O3 -fPIC 
   LINK_DIRECTIVES_EXE =  -O3 -fPIC 
   LINK_DIRECTIVES_ARCHIVE = -O3 -fPIC 
endif

CROSS_COMPILE_DIRECTIVES = -g -Wall --analyze -pedantic -fcolor-diagnostics -DNOWHAT -std=c++11 -stdlib=libc++ -U__STRICT_ANSI__ -DGTEST_USE_OWN_TR1_TUPLE=1


#
# Add user directive
#
COMPILE_DIRECTIVES := $(COMPILE_DIRECTIVES) $(EXTRA_CC_FLAGS)
LINK_DIRECTIVES_LIB := $(LINK_DIRECTIVES_LIB) $(EXTRA_LINK_FLAGS)
LINK_DIRECTIVES_EXE := $(LINK_DIRECTIVES_EXE) $(EXTRA_LINK_FLAGS)


BUILDSERVER = casual-build-server -c $(EXECUTABLE_LINKER) 
BUILDCLIENT = CC='$(EXECUTABLE_LINKER)' $(CASUALMAKE_PATH)/bin/buildclient -v

#
# VALGRIND
#
ifdef VALGRIND
VALGRIND_CONFIG=valgrind --xml=yes --xml-file=valgrind.xml
else
VALGRIND_CONFIG=
endif


#
# Default include/library-paths
#
DEFAULT_INCLUDE_PATHS := ./include
DEFAULT_LIBRARY_PATHS := ./bin 
 


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
HEADER_DEPENDENCY_COMMAND = -g++ -MP -MM -std=c++11 -isystem $(DB2INCLUDE_DIR) -isystem $(TUXINCLUDE_DIR)



######################################################################
## 
## Transformed casual make file follows below
##
######################################################################
    """;
    
    



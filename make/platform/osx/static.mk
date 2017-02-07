


#
# Default libs
#
DEFAULT_LIBS =  


######################################################################
## 
## compilation and link configuration
##
######################################################################



COMPILER = g++ -fcolor-diagnostics
CROSSCOMPILER = clang++
WARNING_DIRECTIVE = -Werror -Wall -Wextra -pedantic -Wsign-compare -Wuninitialized -Winit-self -Woverloaded-virtual -Wmissing-declarations -Wno-unused-parameter 

#
# Linkers
#
LIBRARY_LINKER = g++
ARCHIVE_LINKER = ar rcs

STD_DIRECTIVE = -std=c++14

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

# how can we get emmidate binding like -Wl,-z,now ?
GENERAL_LINK_DIRECTIVE = -fPIC

ifdef DEBUG
   COMPILE_DIRECTIVES = -ggdb -c -fPIC $(WARNING_DIRECTIVE) $(STD_DIRECTIVE)
   LINK_DIRECTIVES_LIB =  -ggdb -dynamiclib $(WARNING_DIRECTIVE) $(GENERAL_LINK_DIRECTIVE)
   LINK_DIRECTIVES_EXE =  -ggdb $(WARNING_DIRECTIVE) $(GENERAL_LINK_DIRECTIVE)
   LINK_DIRECTIVES_ARCHIVE =  -ggdb $(WARNING_DIRECTIVE) $(GENERAL_LINK_DIRECTIVE)
   
   ifdef ANALYZE
      COMPILE_DIRECTIVES += -fprofile-arcs -ftest-coverage
      LINK_DIRECTIVES_LIB += -fprofile-arcs
      LINK_DIRECTIVES_EXE += -lgcov -fprofile-arcs
   endif
   
else
   COMPILE_DIRECTIVES =  -c -O3 -fPIC $(WARNING_DIRECTIVE) $(STD_DIRECTIVE) -pthread
   LINK_DIRECTIVES_LIB =  -dynamiclib -O3 $(GENERAL_LINK_DIRECTIVE) $(WARNING_DIRECTIVE) $(STD_DIRECTIVE)
   LINK_DIRECTIVES_EXE =  -O3 -fPIC $(GENERAL_LINK_DIRECTIVE) $(WARNING_DIRECTIVE) $(STD_DIRECTIVE)
   LINK_DIRECTIVES_ARCHIVE = -O3 -fPIC $(GENERAL_LINK_DIRECTIVE) $(WARNING_DIRECTIVE) -$(STD_DIRECTIVE) -pthread
endif

CROSS_COMPILE_DIRECTIVES = -c -g $(WARNING_DIRECTIVE) -fcolor-diagnostics -DNOWHAT $(STD_DIRECTIVE) -stdlib=libc++ -U__STRICT_ANSI__ -DGTEST_USE_OWN_TR1_TUPLE=1




#
# VALGRIND
#
ifdef VALGRIND
PRE_UNITTEST_DIRECTIVE=valgrind --xml=yes --xml-file=valgrind.xml
endif



#
# Format the include-/library- paths
# 
INCLUDE_PATHS_DIRECTIVE = $(addprefix -I, $(INCLUDE_PATHS) )
LIBRARY_PATHS_DIRECTIVE = $(addprefix -L, $(LIBRARY_PATHS) )
DEFAULT_INCLUDE_PATHS_DIRECTIVE = $(addprefix -I, $(DEFAULT_INCLUDE_PATHS) )
DEFAULT_LIBRARY_PATHS_DIRECTIVE = $(addprefix -L, $(DEFAULT_LIBRARY_PATHS) )

#
# Header dependency stuff
#
HEADER_DEPENDENCY_COMMAND = -g++ -MP -M $(STD_DIRECTIVE)

#
# Directive for setting SONAME
#
LINKER_SONAME_DIRECTIVE = -Wl,-dylib_install_name,


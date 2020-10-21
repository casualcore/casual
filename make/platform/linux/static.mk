
######################################################################
## 
## compilation and link configuration
##
######################################################################


ifndef CXX
CXX = g++
endif


COMPILER = $(CXX)


# -Woverloaded-virtual add when g++ fixes bug
# -Wno-noexcept-type  we can probably remove this on g++ 8
# -Wno-implicit-fallthrough remove and add [[fallthrough]] attribute, when we're on c++17
#
WARNING_DIRECTIVE = -Wall \
 -Wextra \
 -Werror \
 -Wsign-compare \
 -Wuninitialized  \
 -Winit-self \
 -Wno-unused-parameter \
 -Wno-missing-declarations \
 -Wno-noexcept-type \
 -Wno-implicit-fallthrough

VERSION_DIRECTIVE = -DCASUAL_BUILD_VERSION=\"$(CASUAL_BUILD_VERSION)\"


STD_DIRECTIVE = -std=c++17

# Linkers
LIBRARY_LINKER = $(CXX)
ARCHIVE_LINKER = ar rcs

# lint stuff
ifndef LINT_COMMAND
LINT_COMMAND = clang-tidy
endif 

ifndef LINT_PRE_DIRECTIVES
LINT_PRE_DIRECTIVES = -quiet -config '' --
endif


ifndef EXECUTABLE_LINKER
EXECUTABLE_LINKER = g++
endif

export EXECUTABLE_LINKER

# Compile and link directives
ifdef DEBUG
   COMPILE_DIRECTIVES = -g -pthread -c  -fpic $(VERSION_DIRECTIVE) $(WARNING_DIRECTIVE) $(STD_DIRECTIVE)
   LINK_DIRECTIVES_LIB = -g -pthread -shared  -fpic
   LINK_DIRECTIVES_EXE = -g -pthread  -fpic
   LINK_DIRECTIVES_ARCHIVE = -g  

   ifdef ANALYZE
      COMPILE_DIRECTIVES += -O0 -coverage
      LINK_DIRECTIVES_LIB += -O0 -coverage
      LINK_DIRECTIVES_EXE += -O0 -coverage
   endif

else
   COMPILE_DIRECTIVES = -pthread -c -O3 -fpic $(VERSION_DIRECTIVE) $(WARNING_DIRECTIVE) $(STD_DIRECTIVE)
   LINK_DIRECTIVES_LIB = -pthread -shared -O3 -fpic $(WARNING_DIRECTIVE) $(STD_DIRECTIVE)
   LINK_DIRECTIVES_EXE = -pthread -O3 -fpic $(WARNING_DIRECTIVE) $(STD_DIRECTIVE)
   LINK_DIRECTIVES_ARCHIVE = 
endif


BUILDSERVER = casual-build-server -c $(EXECUTABLE_LINKER) 

# VALGRIND
ifdef VALGRIND
PRE_UNITTEST_DIRECTIVE=valgrind --xml=yes --xml-file=valgrind.xml
endif

# Format the include-/library- paths
LIBRARY_PATH_OPTION=-Wl,-rpath-link=

INCLUDE_PATHS_DIRECTIVE = $(addprefix -I, $(INCLUDE_PATHS) )
LIBRARY_PATHS_DIRECTIVE = $(addprefix -L, $(LIBRARY_PATHS) ) $(addprefix $(LIBRARY_PATH_OPTION), $(LIBRARY_PATHS) )

# Header dependency stuff
HEADER_DEPENDENCY_COMMAND = -g++ -MP -MM $(STD_DIRECTIVE)

# Directive for setting SONAME
LINKER_SONAME_DIRECTIVE = -Wl,-soname,
    
   



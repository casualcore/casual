

#
# Unittest lib, probably only work for gtest.
#
ISOLATED_UNITTEST_LIB = gtest 

ISOLATED_UNITTEST_DIRECTIVES := $(ISOLATED_UNITTEST_DIRECTIVES) --gtest_color=yes




BUILDSERVER = casual-build-server -c $(EXECUTABLE_LINKER) 
BUILDCLIENT = CC='$(EXECUTABLE_LINKER)' $(CASUALMAKE_PATH)/bin/buildclient -v


#
# Default include/library-paths
#
DEFAULT_INCLUDE_PATHS = ./include
DEFAULT_LIBRARY_PATHS = ./bin




THIS_MAKEFILE = $(CURDIR)/$(word $(words $(MAKEFILE_LIST)),$(MAKEFILE_LIST))




#
# Unittest lib, probably only work for gtest.
#


ISOLATED_UNITTEST_DIRECTIVES += --gtest_color=yes




BUILDSERVER = casual-build-server -c $(EXECUTABLE_LINKER) 
BUILDCLIENT = CC='$(EXECUTABLE_LINKER)' $(CASUALMAKE_PATH)/bin/buildclient -v




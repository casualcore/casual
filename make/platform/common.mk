

# make sure we use bash
SHELL = bash

ISOLATED_UNITTEST_DIRECTIVES += --gtest_color=yes

ifndef CASUAL_REPOSITORY_ROOT
export CASUAL_REPOSITORY_ROOT = $(shell git rev-parse --show-toplevel)
endif



BUILDSERVER = casual-build-server -c $(EXECUTABLE_LINKER) 
BUILDCLIENT = CC='$(EXECUTABLE_LINKER)' $(CASUALMAKE_PATH)/bin/buildclient -v


compiler-version: 
	@$(COMPILER) --version



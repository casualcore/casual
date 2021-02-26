#
##
#

COBCPY=.
export COBCPY

BUILDSRV=${CASUAL_HOME}/bin/casual-build-server

CC=gcc
CPP=g++
CFLAGS=-I${CASUAL_HOME}/include
CPPFLAGS=-Wall -Wextra -I${CASUAL_HOME}/include
LDFLAGS=-L${CASUAL_HOME}/lib -lcasual-cobol-binding

COB=cob
COBFLAGS=-xg -std=cobol85
#COBFLAGS=-x -Wall -std=cobol85 $(shell cob-config --cflags --libs) $(CFLAGS)
COBCPY=.

all:	simpserv simpcl CSIMPCL

CSIMPCL:  CSIMPCL.cbl
	$(COB) CSIMPCL.cbl $(COBFLAGS) $(LDFLAGS)

clean:
	- rm *.o *.int *.idy CSIMPCL


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
# COBFLAGS=`cob-config --cflags --libs`
COBFLAGS=$(shell cob-config --cflags --libs)

all:	EXAMPLE1


EXAMPLE1:  EXAMPLE1.cbl
	cobc -x -Wall -std=cobol85 EXAMPLE1.cbl $(COBFLAGS) $(CFLAGS) $(LDFLAGS)


clean:
	- rm *.o EXAMPLE1


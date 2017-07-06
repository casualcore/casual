#
##
#

COBCPY=.
export COBCPY

BUILDSRV=$(CASUAL_HOME)/bin/casual-build-server

LOCAL_ROOT=../..

COPY_INCLUDE=$(LOCAL_ROOT)/copy

CC=gcc
CPP=g++
CFLAGS=-I$(CASUAL_HOME)/include -I$(LOCAL_ROOT)/include
CPPFLAGS=-Wall -Wextra -I$(LOCAL_ROOT)/include -I$(CASUAL_HOME)/include
LDFLAGS=-L$(CASUAL_HOME)/lib -lcasual-cobol-binding
# COBFLAGS=`cob-config --cflags --libs`
COBFLAGS=$(shell cob-config --cflags --libs) -I$(COPY_INCLUDE)

all:	simpserv simpcl CSIMPCL

CSIMPCL:  CSIMPCL.cbl
	cobc -x -Wall -std=cobol85 CSIMPCL.cbl $(COBFLAGS) $(CFLAGS) $(LDFLAGS)

simpcl:  simpcl.c
	$(CPP) -I/usr/local/casual/include -o simpcl simpcl.c $(LDFLAGS)

simpserv: simpserv.c
	$(BUILDSRV) -v -c g++ -f simpserv.c -o simpserv -s TOUPPER

clean:
	- rm *.o simpserv simpcl CSIMPCL


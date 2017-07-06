#
##
#

COBCPY=.
export COBCPY

BUILDSRV=$(CASUAL_HOME)/bin/casual-build-server

CC=gcc
CPP=g++
CFLAGS=-I$(CASUAL_HOME)/include
CPPFLAGS=-Wall -Wextra -I$(CASUAL_HOME)/include
LDFLAGS=-L$(CASUAL_HOME)/lib -lcasual-xatmi

COB=cob
COBFLAGS=-xg -std=cobol85
#COBFLAGS=-x -Wall -std=cobol85 $(shell cob-config --cflags --libs) $(CFLAGS)
COBCPY=.

all:	simpserv simpcl CSIMPCL

CSIMPCL:  CSIMPCL.cbl xatmi_cobol.o
	$(COB) CSIMPCL.cbl xatmi_cobol.o $(COBFLAGS) $(LDFLAGS)

xatmi_cobol.o:  xatmi_cobol.c xatmi_cobol.h
	$(CPP) $(CPPFLAGS) -c xatmi_cobol.c

simpcl:  simpcl.c
	$(CPP) $(CPPFLAGS) -o simpcl simpcl.c $(LDFLAGS)

simpserv: simpserv.c
	$(BUILDSRV) -v -c $(CPP) -f simpserv.c -o simpserv -s TOUPPER

clean:
	- rm *.o *.int *.idy simpserv simpcl CSIMPCL


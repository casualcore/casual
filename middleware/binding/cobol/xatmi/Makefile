#
##
#

COBCPY=.
export COBCPY

BUILDSRV=$(CASUAL_HOME)/bin/casual-build-server

CC=gcc
CPP=g++
CFLAGS=-I/usr/local/casual/include
CPPFLAGS=-Wall -Wextra -I/usr/local/casual/include
LDFLAGS=-L/usr/local/casual/lib -lcasual-xatmi
# COBFLAGS=`cob-config --cflags --libs`
COBFLAGS=$(shell cob-config --cflags --libs)

all:	simpserv simpcl CSIMPCL

CSIMPCL:  CSIMPCL.cbl xatmi_cobol.o
	cobc -x -Wall -std=cobol85 CSIMPCL.cbl xatmi_cobol.o $(COBFLAGS) $(CFLAGS) $(LDFLAGS)

xatmi_cobol.o:  xatmi_cobol.c xatmi_cobol.h
	$(CPP) $(CPPFLAGS) -c xatmi_cobol.c

simpcl:  simpcl.c
	$(CPP) -I/usr/local/casual/include -o simpcl simpcl.c $(LDFLAGS)

simpserv: simpserv.c
	$(BUILDSRV) -v -c g++ -f simpserv.c -o simpserv -s TOUPPER

clean:
	- rm *.o simpserv simpcl CSIMPCL


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
LDFLAGS=-L/usr/local/casual/lib -lcasual-xatmi
# COBFLAGS=`cob-config --cflags --libs`
COBFLAGS=$(shell cob-config --cflags --libs)

all:	EXAMPLE1

EXAMPLE1:  EXAMPLE1.cbl tx_cobol.o
	cobc -x -Wall -std=cobol85 EXAMPLE1.cbl tx_cobol.o $(COBFLAGS) $(CFLAGS) $(LDFLAGS)

tx_cobol.o:  tx_cobol.c tx_cobol.h
	$(CPP) $(CPPFLAGS) -c tx_cobol.c

clean:
	- rm *.o EXAMPLE1


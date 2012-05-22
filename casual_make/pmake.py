#!/usr/bin/python

'''
Created on 14 maj 2012

@author: hbergk
'''

SCRIPTVERSION="2.0"
SENASTUPPDATERAD="20120514"

#
# Imports
#
import sys
import os
import multiprocessing
from casual_make import Casual_Make
import subprocess
import re

#
# Globals to use
#
GMAKE_OPTIONS=""
FORCE_MAKEMAKE=0
MAKE_PATH="/usr/bin/make"
USER_MAKE_FILE=""


#
# Funktion for att kontrollera om det korresponderande argumentet ar
# korrekt
#
def arg2kontroll ( arg1, arg2):
    """ """
   
    if arg2 == "" or arg2[0:1] == "-" : 
        print "\nError:\tWrong usage\n\tno value set to option \"$1\"\n"
        #printhelp()
        sys.exit(1)

#
# Finding out number of CPUs
#
def numberOfCPUs():
    """ """    
    return multiprocessing.cpu_count()

def handleArguments():
    """Supposed to handle arguments. Not implemented yet."""
    pass
    #
#    # Ta reda pa vad anvandaren forsker gora
#    #
#    while [ $# -gt 0 ]
#    do
#       arg1=$1
#       arg2=$2
#       
#       case "$arg1" in
#       "-f" | "-file" | "--file" )
#          arg2kontroll $arg1 $arg2
#          
#          USER_MAKE_FILE=$arg2          
#          shift
#          shift
#       ;;
#       "-debug" )
#       
#          GMAKE_OPTIONS="$GMAKE_OPTIONS DEBUG=1"     
#          shift
#       ;;
#       "-release" )
#          GMAKE_OPTIONS="$GMAKE_OPTIONS RELEASE=1" 
#          shift
#       ;;
#       "-force-parallel" )
#          GMAKE_OPTIONS="$GMAKE_OPTIONS FORCE_PARALLEL=1" 
#          shift
#       ;;
#        "-force-notparallel" )
#          GMAKE_OPTIONS="$GMAKE_OPTIONS FORCE_NOTPARALLEL=1" 
#          shift
#       ;;
#       "make" )
#          #
#          # Anvandaren tvingar en omgenerering av makefiler.
#            # Kors jarnet parallelt
#          #
#          FORCE_MAKEMAKE=1
#          GMAKE_OPTIONS="$GMAKE_OPTIONS FORCE_PARALLEL=1 $arg1"
#          shift
#       ;;
#       "clean" | "prep" | "compile" | "export" | "export_headers" | "cross")
#          #
#          # clean, prep och compile kan koras i parallel.
#          #
#          GMAKE_OPTIONS="$GMAKE_OPTIONS FORCE_PARALLEL=1 $arg1"
#          shift
#       ;;
#       *)
#          #
#          # Vi later det vi inte kanner igen ga vidare till
#          # gmake i slutandan...
#          #
#          GMAKE_OPTIONS="$GMAKE_OPTIONS $arg1"    
#          shift 
#       ;; 
#       esac
#    done

def reformat( line):
    """ reformat output from make and add som colours"""
    #
    # All regex at once. Takes no time...
    #
    compiling = re.search(r'(^(CC)|(g\+\+)) -o (?:\S+\.o) ((\S+\.cc)|(\S+\.cpp)|(\S+\.c)).*', line)
    archive = re.search(r'(^ar) \S+ (\S+\.a).*', line)
    link = re.search(r'(^(CC)|(g\+\+)) -o (\S+) (?:(\S+\.o) ).*', line)
    make = re.search(r'^[\S\s]+ -f (\S+.*) .*', line)
    if compiling:
        return '\033[92m' + 'Compile (' + compiling.group(1) + '): ' + '\033[0m' + compiling.group(4) + '\n'
    elif archive:
        return '\033[94m' + 'Archive (' + archive.group(1) + '): ' + '\033[0m' + archive.group(2) + '\n'
    elif link:
        return '\033[94m' + 'Link (' + link.group(1) + '): ' + '\033[0m' + link.group(4) + '\n'
    elif make:
        return 'Current makefile: ' + make.group(1) + '\n'
    else:
        return line

if __name__ == '__main__':
    #
    # Kolla om anvandaren har skickat med ngon imake/make-fil
    #
    if USER_MAKE_FILE == "" :
        USER_MAKE_FILE="makefile.mk"
    
    USER_MAKE_FILE=os.path.basename( USER_MAKE_FILE)
    
    #
    # TODO:
    #
    CORRESPONDING_CASUAL_MAKE_FILE="makefile.cmk"; 
    
    if len(sys.argv) < 2:
        GMAKE_OPTIONS="all"
    else:
        GMAKE_OPTIONS=sys.argv[1]


    #
    # Kolla om vi ska generera om
    #
    if FORCE_MAKEMAKE == 1 or not os.path.isfile( USER_MAKE_FILE)  or os.path.getmtime(USER_MAKE_FILE) < os.path.getmtime(CORRESPONDING_CASUAL_MAKE_FILE) :
        if not os.path.isfile( CORRESPONDING_CASUAL_MAKE_FILE ):
            sys.stderr.write( "error: Could not find the casaul make file " + CORRESPONDING_CASUAL_MAKE_FILE)
            sys.exit( 1)

    print "info: executes 'casual_make " + CORRESPONDING_CASUAL_MAKE_FILE + "'" 

    try:
        Casual_Make( CORRESPONDING_CASUAL_MAKE_FILE).run()
        
    except:
        sys.stderr.write("error: Could not generate " + USER_MAKE_FILE + " from " + CORRESPONDING_CASUAL_MAKE_FILE + '\n')
        raise
        sys.exit(1)

#
# Find number of CPU:s to set jobs and load
#

    CPU_COUNT=numberOfCPUs()


    if CPU_COUNT == 0:
        sys.stderr.write( "warning: Could not detect how many CPU's the machine has - guess on 2\n")
        CPU_COUNT=2


    JOB_COUNT=CPU_COUNT * 1.5

#
# Vi kor aven max load till 1.5x antal CPU:er. Har ingen aning om detta
# ar en bra siffra...
#
    MAX_LOAD=JOB_COUNT


    print "info: executes " + MAKE_PATH + " -j " + str(int(JOB_COUNT)) + " -l " + str(int(MAX_LOAD)) + " --no-builtin-rules --no-keep-going " + GMAKE_OPTIONS + " -f " + USER_MAKE_FILE 


    #
    # Assemble command
    #
    command = MAKE_PATH + " -j " + str(int(JOB_COUNT)) + " -l " + str(int(MAX_LOAD)) + " " + GMAKE_OPTIONS + " --no-builtin-rules --no-keep-going -f " + USER_MAKE_FILE + '\n' 
    
    #
    # Call commnd
    #
    process = subprocess.Popen( command.split(), stdout=subprocess.PIPE, stderr=subprocess.PIPE)
    
    output = process.stdout
    #
    # Handle stdout from make
    #
    while True:
        next_line = output.readline()

        if not next_line:
            break
        sys.stdout.write( reformat( next_line))
        sys.stdout.flush()

    #
    # Handle stderr from make
    #
    output = process.stderr
    while True:
        next_line = output.readline()
        if not next_line:
            break
        #
        # Writing in red
        #
        sys.stderr.write( '\033[91m' + next_line + '\033[0m')
        sys.stderr.flush()
    
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

class  Bcolors:
    HEADER = '\033[95m'
    BLUE = '\033[94m'
    GREEN = '\033[92m'
    WARNING = '\033[93m'
    FAIL = '\033[91m'
    ENDC = '\033[0m'
    
    def disable(self):
        self.HEADER = ''
        self.BLUE = ''
        self.GREEN = ''
        self.WARNING = ''
        self.FAIL = ''
        self.ENDC = ''

bcolors=Bcolors()

#
# Finding out number of CPUs
#
def numberOfCPUs():
    """ """    
    return multiprocessing.cpu_count()

def handleArguments():
    """Supposed to handle arguments. Not implemented yet."""
    from optparse import OptionParser
    usage = "usage: %prog [options] arg"
    parser = OptionParser(usage)
    
    parser.set_defaults(COLORS=True)
    parser.set_defaults(USER_MAKE_FILE="makefile.mk")
    parser.add_option("-f", "--file", dest="USER_MAKE_FILE") 
    parser.add_option("-d", "--debug", action="store_true", dest="DEBUG")
    parser.add_option("-r", "--release", action="store_true", dest="RELEASE") 
    parser.add_option("-p", "--force-parallel", action="store_true", dest="FORCE_PARALLEL") 
    parser.add_option("-n", "--force-notparallel", action="store_true", dest="FORCE_NOTPARALLEL")
    parser.add_option("--no-colors", action="store_false", dest="COLORS") 
    
    (options, args) = parser.parse_args()
    
    if args and args[0] in ("make"):
        FORCE_MAKEMAKE=1
        
    if args and args[0] in ("clean" , "prep" , "compile" , "export" , "export_headers" ,"cross"):
        options.FORCE_PARALLEL=True
    
    return (options, args)

def optionsAsString( options):
    """Return option list as string"""
    result=""
    if options.FORCE_PARALLEL:
        result = result + " FORCE_PARALLEL=1 "
    if options.FORCE_NOTPARALLEL:
        result = result + " FORCE_NOTPARALLEL=1 "                  
    if options.DEBUG:
        result = result + " DEBUG=1 "                  
    if options.RELEASE:
        result = result + " RELEASE=1 "
        
    return result.strip()                

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
        return bcolors.GREEN + 'Compile (' + compiling.group(1) + '): ' + bcolors.ENDC + compiling.group(4) + '\n'
    elif archive:
        return bcolors.BLUE + 'Archive (' + archive.group(1) + '): ' + bcolors.ENDC + archive.group(2) + '\n'
    elif link:
        return bcolors.BLUE + 'Link (' + link.group(1) + '): ' + bcolors.ENDC + link.group(4) + '\n'
    elif make:
        return bcolors.HEADER + 'Current makefile: ' + bcolors.ENDC + make.group(1) + '\n'
    else:
        return line

if __name__ == '__main__':
    #
    # Kolla om anvandaren har skickat med ngon imake/make-fil
    #
    
    (options, args) = handleArguments()
    
    USER_MAKE_FILE=options.USER_MAKE_FILE
            
    CORRESPONDING_CASUAL_MAKE_FILE=os.path.splitext(USER_MAKE_FILE)[0] + ".cmk"; 
    
    if len(args) != 1:
        GMAKE_OPTIONS=optionsAsString( options) + " all"
    else:
        GMAKE_OPTIONS=optionsAsString( options) + " " + args[0]


    if not options.COLORS:
        bcolors.disable()
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
        sys.stderr.write( bcolors.FAIL + next_line + bcolors.ENDC)
        sys.stderr.flush()
    
    process.poll() 
    sys.exit( process.returncode)
    
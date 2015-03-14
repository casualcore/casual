'''
Created on 13 maj 2012

@author: hbergk
'''

import os
import sys
import re

#
# Defines the return from "all" functions, that can be used
# in other function, as Install or whatnot...
# 
class Target:
    
    def __init__(self, filename, source = '', name = None):
        self.file = filename
        if name:
            self.name = name
        else:
            self.name = target_name( filename)
        self.source = source
        self.base = os.path.basename( source);

def normalize_string( string):
    return re.sub( '[^\w]+', '_', string)


def target_name( name):

    return 'target_' + normalize_string( os.path.basename( name))


def multiline( values):
    if isinstance( values, basestring):
        values = values.split()

    return ' \\\n      '.join( values)


def validate_list( value):
    
    if isinstance( value, basestring):
        raise SyntaxError( 'not a list - content: ' + value);


def target_files( values):
    names = []
    for value in values:
        if isinstance( value, Target):
            names.append( value.file)
        else:
            names.append( value)
    
    return names;

def target_base( values):
    names = []
    for value in values:
        if isinstance( value, Target):
            names.append( value.base)
        else:
            names.append( value)
    
    return names;



def debug( message):
    if os.getenv('PYTHONDEBUG'): sys.stderr.write( message + '\n')

'''
Created on 13 maj 2012

@author: hbergk
'''

import os
import sys
import re

from itertools import izip, tee

from casual.make.output import Output
#
# Defines the return from "all" functions, that can be used
# in other function, as Install or whatnot...
# 
class Target:
    
    def __init__(self, output, source = '', name = None, operation = None):
        
        #
        # Storage of eventual Output instance
        #
        if isinstance( output, Output):
            self.output = output
        else:
            self.output = None
        
        #
        # Stem and filename
        #    
        self.stem = None
        filename = extract_name( output)
        if operation:
            filename = operation( filename)
            self.stem = filename
            if self.output:
                filename += '.' + self.output.version.full_version()
        self.file = filename

        #
        # dirname
        #
        self.dirname = os.path.dirname( filename)
        
        #
        # name
        #
        if name:
            self.name = name
        else:
            self.name = target_name( filename)
        
        #
        # target
        #
        self.target = 'target_' + normalize_string( filename)
        
        #
        # source
        #
        if isinstance( source, Output):
            self.source = source.name
        else:
            self.source = source
        
        #
        # base
        #   
        self.base = os.path.basename( self.source);




def targets_name( targets):
    
    def target_name( target):
        if isinstance( target, basestring):
            return target
        else:
            return target.name
    
    if( isinstance( targets, list)):
        return map( target_name, targets)   
    else:
        return target_name( targets) 


def pairwise( iterable):
    "s -> (s0,s1), (s1,s2), (s2, s3), ..."
    a, b = tee(iterable)
    next(b, None)
    return izip(a, b)            

def base_extract( output, parameter): 
    
    if isinstance( output, Output):
        return parameter
    else:
        raise SystemError, "Unknown output type"
       
def extract_name( output):
    
    if isinstance( output, basestring):
        return output
    else:
        return base_extract( output, output.name)
        

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

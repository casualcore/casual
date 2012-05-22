'''
Created on 28 apr 2012

@author: hbergk
'''

#
# Imports
#
import os
import sys

class Configuration(object):
    '''
    Responsible for selecting correct configuration depending of platform
    '''
    
    def platform(self):
        ''' Decide on which platform this runs '''
        thisPlatform = os.uname()[0]  
        if thisPlatform == "Darwin":
            thisPlatform = "osx"
        return thisPlatform

    def __init__(self):
        '''
        Constructor
        '''
        configfile = open(os.path.dirname(os.path.abspath(sys.argv[0])) + "/implementation/config_" + self.platform() , "r")
        self.content = configfile.read()
        configfile.close()
    
        
    def printConfig(self):
        print self.content
        

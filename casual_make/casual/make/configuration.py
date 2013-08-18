'''
Created on 28 apr 2012

@author: hbergk
'''

#
# Imports
#
import os

class Configuration(object):
    '''
    Responsible for selecting correct configuration depending of platform
    '''
    
    def platform(self):
        ''' Decide on which platform this runs '''
        thisPlatform = os.uname()[0].lower()  
        if thisPlatform == "darwin":
            thisPlatform = "osx"
        return thisPlatform

    def __init__(self):
        '''
        Constructor
        '''
        configfile = open(os.getenv("CASUAL_TOOLS_HOME") + "/casual_make/casual/make/config_" + self.platform() , "r")
        self.content = configfile.read()
        configfile.close()
    
        
    def printConfig(self):
        print self.content
        

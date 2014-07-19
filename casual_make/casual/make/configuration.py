'''
Created on 28 apr 2012

@author: hbergk
'''

#
# Imports
#
import os


def configuration():
    '''
    returns the corresponding configuration file-object for this platform
    '''
    
    # Decide on which platform this runs
    platform = os.uname()[0].lower()  
    if platform == "darwin":
        platform = "osx"
    
    return open( os.getenv("CASUAL_TOOLS_HOME") + "/casual_make/casual/make/platform/" + platform + '/dynamic.py' , "r")
    
    




import os

registry = {}

class RegisterPlatform(object):
    '''
    classdocs
    '''

    def __init__(self, platform):
        '''
        Constructor
        '''
        self.platform = platform
        
    def __call__(self, clazz):
        registry[self.platform] = clazz
  

def platform():
    # Decide on which platform this runs
    platform = os.uname()[0].lower()  
    if platform == "darwin":
        platform = "osx"
    if not registry: 
        raise SyntaxError, "No platforms are registered."
    return registry[ platform]();



    



import os

#from casual.make.platform.osx.platform import *

class Platform:
    
    def prologue(self): pass
    
    def epilogue(self): pass
    
    
    def link_directive(self, libs): pass
    
    
#subclass_mapping = { 'osx': casual.make.platform.osx.platform.OSX};
_subclass_mapping = {}


_instance = None;

def _factory():
    # Decide on which platform this runs
    platform = os.uname()[0].lower()  
    if platform == "darwin":
        platform = "osx"
        
    return _subclass_mapping[ platform]();

def factory():
    
    global _instance
    
    if not _instance:
        _instance = _factory();
    
    return _instance;
    
     

def register( key, type):
    _subclass_mapping[ key] = type;
    



    

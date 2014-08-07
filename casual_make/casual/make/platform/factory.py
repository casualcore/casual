

import os

#from casual.make.platform.osx.platform import *

class Platform:
    
    def pre_make(self): pass
    
    def post_make(self): pass
    
    
    
    def link_directive(self, libs): pass
    
    def library_name(self, baseFilename): pass
    def archive_name(self, baseFilename): pass
    def executable_name(self, baseFilename): pass
    def bind_name(self, baseFilename): pass
    
    
    def header_dependency(self, sourcefile, objectfiles, dependencyfile): pass
    def compile(self, sourcefile, objectfile, directive): pass
    def cross_compile(self, sourcefile, objectfile, directive): pass
    
    def link_generic(self, linker, filename, objectfiles, libs, directive, extraDirective): pass
    def link_library(self, filename, objectfiles, libs, directive): pass
    def link_executable(self, filename, objectfiles, libs, directive): pass
    def link_archive(self, filename, objectfiles, libs, directive): pass
    
    def install(self, source, destination): pass
    
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
    



    

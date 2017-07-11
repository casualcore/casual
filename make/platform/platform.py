import casual.make.platform.registry

import os

class Platform:
    
    def pre_make(self): pass
    
    def post_make(self): pass
    
    
    
    def link_directive(self, libs): pass
    def include_paths(self, paths): pass
    def library_paths(self, paths): pass
    
    def library_name(self, baseFilename): pass
    def archive_name(self, baseFilename): pass
    def executable_name(self, baseFilename): pass
    def bind_name(self, baseFilename): pass
    
    
    def header_dependency(self, sourcefile, objectfiles, dependencyfile): pass
    def compile(self, sourcefile, objectfile, directive): pass
    
    def link_generic(self, linker, output, objectfiles, libs, directive, extraDirective): pass
    def link_library(self, output, objectfiles, libs, directive): pass
    def link_executable(self, output, objectfiles, libs, directive): pass
    def link_archive(self, output, objectfiles, libs, directive): pass
    
    def install(self, source, destination): pass
    
def platform():
    # Decide on which platform this runs
    return casual.make.platform.registry.platform()

   
import os

from casual.make.platform.platform import Platform


class CommonUNIX( Platform):
    
    def __init__(self):
        
        self.unittest_libraries = [ 'gtest', 'gtest_main' ]
        
        

    def link_directive(self, libs):            
        if not libs:
            return ''
            
        return ' -l' + ' -l'.join( libs);
    
    def include_paths(self, paths):
        if not paths:
            return ''
        
        if isinstance( paths, basestring):
            paths = paths.split( ' ');
        
        return ' -I' + ' -I'.join( paths)
    
    def library_paths(self, paths):
        if not paths:
            return ''
        
        if isinstance( paths, basestring):
            paths = paths.split( ' ');
        
        return ' -L' + ' -L'.join( paths)
    
    
    
    def library_name(self, baseFilename):
        return 'lib' + baseFilename + '.so'
        
    def archive_name(self, baseFilename):
        return 'lib' + baseFilename + '.a'
    
    def executable_name(self, baseFilename):
        return baseFilename
    
    def bind_name(self, baseFilename):
        return baseFilename + '.bnd'
    
    def default_object_name(self, sourcefile):
        return 'obj/' + os.path.splitext( sourcefile)[0] + '.o'
    
    
    def header_dependency(self, sourcefile, objectfiles, dependencyfile):
        return '@$(HEADER_DEPENDENCY_COMMAND) -MT ' + ' -MT '.join( objectfiles) + ' $(INCLUDE_PATHS) $(DEFAULT_INCLUDE_PATHS) ' + sourcefile + ' -MF ' +  dependencyfile 
    
    def compile(self, sourcefile, objectfile, directive):
        return '$(COMPILER) -o ' + objectfile + ' ' + sourcefile + ' $(INCLUDE_PATHS) $(DEFAULT_INCLUDE_PATHS) $(COMPILE_DIRECTIVES) ' +  directive
    
    def cross_compile(self, sourcefile, objectfile, directive):
        return '$(CROSSCOMPILER) $(CROSS_COMPILE_DIRECTIVES) -o ' + objectfile + ' ' + sourcefile + ' $(INCLUDE_PATHS) $(DEFAULT_INCLUDE_PATHS) ' + directive



    def link_generic(self, linker, filename, objectfiles, libs, directive, extraDirective):
        return linker + ' -o ' + filename + ' ' + objectfiles + ' $(LIBRARY_PATHS) $(DEFAULT_LIBRARY_PATHS) ' + libs + ' $(DEFAULT_LIBS) ' + directive + ' ' + extraDirective

    def link_library(self, filename, objectfiles, libs, directive, soname):
        if not soname:
            return self.link_generic( '$(LIBRARY_LINKER)' , filename, objectfiles, libs, '$(LINK_DIRECTIVES_LIB)', directive)
        else:
            return '$(LIBRARY_LINKER)' + ' -o ' + filename + ' ' + '$(LINKER_SONAME_DIRECTIVE)' + soname + ' ' + objectfiles + ' $(LIBRARY_PATHS) $(DEFAULT_LIBRARY_PATHS) ' + libs + ' $(DEFAULT_LIBS) ' + '$(LINK_DIRECTIVES_LIB)' + directive
    
    
    def link_executable(self, filename, objectfiles, libs, directive, version):
        return self.link_generic( '$(EXECUTABLE_LINKER)' , filename, objectfiles, libs, '$(LINK_DIRECTIVES_EXE)', directive)
    
    def link_archive(self, filename, objectfiles, libs, directive, version):
        return '$(ARCHIVE_LINKER) ' + filename + ' ' + objectfiles + ' ' + directive
    
    
    # should not be here...
    def link_server(self, filename, objectfiles, libs, directive, version):
        return '$(BUILDSERVER) -o ' + filename + directive + ' -f "' + objectfiles + '" -f "' + libs + '" -f "$(LIBRARY_PATHS) $(DEFAULT_LIBRARY_PATHS) $(LINK_DIRECTIVES_EXE) $(INCLUDE_PATHS)" ' 
    
        
    
    def make_directory(self, directory):
        return 'mkdir -p ' + directory
    
    def change_directory(self, directory):
        return 'cd ' + directory
    
    def remove(self, filename):
        return 'rm -f ' + filename
    
    def install(self, source, destination):
        return 'rsync --checksum -i ' + source + ' ' +  destination
        
    def unix_link(self, source, linkname):
        return 'ln -s ' + source + ' ' +  linkname
   
    
    
    
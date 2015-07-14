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



    def link_generic(self, linker, output, objectfiles, libs, directive, extraDirective):
        return linker + ' -o ' + output + ' ' + objectfiles + ' $(LIBRARY_PATHS) $(DEFAULT_LIBRARY_PATHS) ' + libs + ' $(DEFAULT_LIBS) ' + directive + ' ' + extraDirective

    def link_library(self, output, objectfiles, libs, directive):
        if isinstance( output, basestring):
            return self.link_generic( '$(LIBRARY_LINKER)', output, objectfiles, libs, '$(LINK_DIRECTIVES_LIB)', directive)
        elif isinstance( output, Output):
            return '$(LIBRARY_LINKER)' + ' -o ' + output.name + ' ' + '$(LINKER_SONAME_DIRECTIVE)' + output.soname() + ' ' + objectfiles + ' $(LIBRARY_PATHS) $(DEFAULT_LIBRARY_PATHS) ' + libs + ' $(DEFAULT_LIBS) ' + '$(LINK_DIRECTIVES_LIB)' + directive
        else:
            raise SyntaxError, 'Unknown type for output'
    
    def link_executable(self, output, objectfiles, libs, directive):
        return self.link_generic( '$(EXECUTABLE_LINKER)' , output, objectfiles, libs, '$(LINK_DIRECTIVES_EXE)', directive)
    
    def link_archive(self, output, objectfiles, libs, directive):
        return '$(ARCHIVE_LINKER) ' + output + ' ' + objectfiles + ' ' + directive
    
    
    # should not be here...
    def link_server(self, output, objectfiles, libs, directive):
        return '$(BUILDSERVER) -o ' + output + directive + ' -f "' + objectfiles + '" -f "' + libs + '" -f "$(LIBRARY_PATHS) $(DEFAULT_LIBRARY_PATHS) $(LINK_DIRECTIVES_EXE) $(INCLUDE_PATHS)" ' 
    
        
    
    def make_directory(self, directory):
        return 'mkdir -p ' + directory
    
    def change_directory(self, directory):
        return 'cd ' + directory
    
    def remove(self, filename):
        return 'rm -f ' + filename
    
    def install(self, source, destination):
        return 'rsync --checksum -i ' + source + ' ' +  destination
    
    def install_link(self, source, destination):
        return 'rsync --checksum -i --links ' + source + ' ' +  destination
     
    def symlink(self, filename, linkname):
        return 'ln -s ' + filename + ' ' +  linkname
   
    
    
    
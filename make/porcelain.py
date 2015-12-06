
import casual.make.porcelainimpl.directive

_porcelain = None


class Porcelain(object):
    """
    Porcelain dispatcher
    """
    
    def __init__(self, version = None):
        #
        # Handle versions. Currently only one
        #
        self.version = version
        self.implementation = casual.make.porcelainimpl.directive
    
    def __getattr__(self, name):
        return getattr( self.implementation, name)

def porcelain(version = None):
    global _porcelain
    if not _porcelain or _porcelain.version != version:
        _porcelain = Porcelain(version)
     
    return _porcelain

#
# "Interface to implementation methods
#
def NoDefaultLibs(): 
    """
    NoDefaultLibs()

    Makes sure nothing is linked default
    """
    return porcelain().NoDefaultLibs()

def DefaultIncludePaths( value = True): 
    """ 
    If the default include paths is used or not.
    Default they are used
    """    
    return porcelain().DefaultIncludePaths( value)

def DefaultLibraryPaths( value = True): 
    """
    If the default librarys paths is used or not.
    Default they are used
    """
    return porcelain().DefaultLibraryPaths( value)

def Parallel( value = True): 
    """
    Controls whether this casual-make file will be processed in parallel or not 

    """    
    return porcelain().Parallel( value)
def Scope( inherit = True, name = None):
    """
    Creates a new scope, with new context. Think of it as a nested
    casual-make-file.
    
    Shall be used in a 'with statement", example:
    
    with Scope():
        Parallel()
        
        Build( someother-casual-make-file)
        
    User can use as many scopes as he or she wants. The scopes can be nested in
    arbitrary depth
    
    
    Under the hood the scope is generated in a isolated makefile, and
    invoked from the parent scope.
    
    :param inherit: If true, state is copied from the parent scope. Normally
        this correlates to IncludePaths and such.
        
    :param name: If set the scope is named to the value. It only effects the
        name of the isolated makefile   
    """
    return porcelain().Scope( inherit)

def Environment( name, value = '', export = True): 
    """
    Sets an environment variable
    :param name: the name of the variable
    :param value: the value to be set
    :param export: if the variable shuld be exportet to sub-makefiles and shells
    """
    return porcelain().Environment(name, value, export)

def Compile( sourcefile, objectfile = None, directive = ''):
    """
    Compiles a source file to an object file, with excplicit directives

    :param sourcefile:    name of the sourcefile (src/myfile.cpp)
    :param objectfile:  optional name of the output object file (obj/myfile.o)
    :param directive:   optional compile directive for this TU, default ''
    :return: the target (which contains the name of the objectfile) 
    """
    return  porcelain().Compile( sourcefile, objectfile, directive)

def LinkLibrary(name,objectfiles,libs = []): 
    """
    Links a shared library
 
    :param name        name of the binary with out prefix or suffix.
    :param objectfiles    object files that is linked
    :param libs        dependent libraries

    :return: target name
    """    
    return porcelain().LinkLibrary(name,objectfiles,libs)

def LinkArchive(name,objectfiles): 
    """
    Links an archive

    :param: name        name of the binary with out prefix or suffix.  
    :param: objectfiles    object files that is linked
    :return: target name
    """
    return porcelain().LinkArchive(name,objectfiles)

def LinkExecutable( name, objectfiles, libraries = []): 
    """
    Links an executable

    :param name        name of the binary with out prefix or suffix.
    
    :param objectfiles    object files that is linked

    :param libs        dependent libraries
 
    :return: target name
    """
    return porcelain().LinkExecutable( name, objectfiles, libraries)

def LinkResourceProxy( name, resource, libraries = [], directive = ''): return porcelain().LinkResourceProxy( name, resource, libraries, directive)
def Dependencies( target, dependencies): 
    """
    Set dependencies to arbitary targets. This is not needed in the
    general case, since casual-make takes care of most dependencies 
    automatic.
    
    This can be used when one wants to execute binaries in a unittest-scenario.
    To make sure that the actual binaries that will run is linked before the
    unittest is linked (and run).
    
    :param target: the target that has dependencies
    :param dependencies: target dependencies
    
    """
    return porcelain().Dependencies( target, dependencies)
def Build(casualMakefile): 
    """
    "builds" another casual-make-file: jumps to the specific file and execute make

    :param casualMakefile    The file to build
    """
    return porcelain().Build(casualMakefile)
def LinkUnittest(name,objectfiles,libraries = [], test_target = True): 
    """
    LinkIsolatedUnittest(name,objectfiles,libs)
    
    :param: name        name of the unittest executable
    :param: objectfiles    object files that is linked
    :param: libraries        dependent libraries (optional)
    :param: tets_target   if true, a test target is generated. (True is the default)
    """    
    return porcelain().LinkUnittest(name,objectfiles,libraries, test_target)
def Install( target, destination): 
    
    return porcelain().Install( target, destination)
def Include( filename): return porcelain().Include( filename)
def IncludePaths( paths): return porcelain().IncludePaths( paths)
def LibraryPaths( paths): return porcelain().LibraryPaths( paths)

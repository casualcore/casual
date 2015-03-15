
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
def NoDefaultLibs(): return porcelain().NoDefaultLibs()
def DefaultIncludePaths( value = True): return porcelain().DefaultIncludePaths( value)
def DefaultLibraryPaths( value = True): return porcelain().DefaultLibraryPaths( value)
def Parallel( value = True): return porcelain().Parallel( value)
def Scope( inherit = True, name = None): return porcelain().Scope( inherit)
def Environment( name, value = '', export = True): return porcelain().Environment(name, value, export)
def Compile( sourcefile, objectfile = None, directive = ''): return  porcelain().Compile( sourcefile, objectfile, directive)
def LinkLibrary(name,objectfiles,libs = []): return porcelain().LinkLibrary(name,objectfiles,libs)
def LinkArchive(name,objectfiles): return porcelain().LinkArchive(name,objectfiles)
def LinkExecutable( name, objectfiles, libraries = []): return porcelain().LinkExecutable( name, objectfiles, libraries)
def LinkResourceProxy( name, resource, libraries = [], directive = ''): return porcelain().LinkResourceProxy( name, resource, libraries, directive)
def Dependencies( target, dependencies): return porcelain().Dependencies( target, dependencies)
def Build(casualMakefile): return porcelain().Build(casualMakefile)
def LinkUnittest(name,objectfiles,libraries = [], test_target = True): return porcelain().LinkUnittest(name,objectfiles,libraries, test_target)
def Install( target, destination): return porcelain().Install( target, destination)
def Include( filename): return porcelain().Include( filename)
def IncludePaths( paths): return porcelain().IncludePaths( paths)
def LibraryPaths( paths): return porcelain().LibraryPaths( paths)

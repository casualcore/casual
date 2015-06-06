'''
    Created on 28 apr 2012
    
    @author: hbergk
    
    Contains the basic syntax to produce makefile
    '''

#
# Imports
#
import casual.make.plumbing
import casual.make.porcelain
from casual.make.version import Version

#
# Select specific version or leave empty for latest
#
_plumbing = casual.make.plumbing.plumbing(version = 1.0)
_porcelain = casual.make.porcelain.porcelain(version = 1.0)

#
# New functions adding functionality
#
def LinkServer( name, objectfiles, libraries, serverdefinition, resources=None):
    """
 Links a XATMI-server
 
 param: name        name of the server with out prefix or suffix.
     
 param: objectfiles    object files that is linked
 
 param: libraries        dependent libraries
 
 param: serverdefinition  path to the server definition file that configure the public services, 
                         and semantics.
                         Can also be a list of public services. I e.  ["service1", "service2"]
                     
 param: resources  optional - a list of XA resources. I e ["db2-rm"] - the names shall 
                correspond to those defined in $CASUAL_HOME/configuration/resources.(yaml|json|...)
    """
 
    path = _plumbing.executable_name_path(name)
    target = _plumbing.target(path, name)
     
    directive = "";
 
    if resources:
        directive += " -r " + ' '.join( resources)
     
    if isinstance( serverdefinition, basestring):
        # We assume it is a path to a server-definition-file
        directive += ' -p ' + serverdefinition
         
        print '# dependency to server definition file'
        print path + ': ' + serverdefinition
        print
        
    else:
        directive += ' -s ' + ' '.join( serverdefinition)
 
     
    return _plumbing.link( _plumbing.platform().link_server, target, objectfiles, libraries, directive)    
 
def LinkClient( name, objectfiles, libraries, resources=None):
    """
 Links a XATMI client
 
 param: name        name of the binary with out prefix or suffix.
 param: objectfiles    object files that is linked
 param: libs        dependent libraries
    """
     
    target = _plumbing.target( _plumbing.executable_name_path( name), name)    
 
    if not resources:
        directive = "";
    else:
        directive = " -r " + ' '.join( resources)
         
    _plumbing.link( _plumbing.platform().link_client, target, objectfiles, libraries, directive)

#
# Dispatched to porcelain functions in casual make
# If one don't want to add import statement you have to reimplement functions
#
def NoDefaultLibs(): 
    return _porcelain.NoDefaultLibs()
def DefaultIncludePaths( value = True): 
    return _porcelain.DefaultIncludePaths( value)
def DefaultLibraryPaths( value = True): 
    return _porcelain.DefaultLibraryPaths( value)
def Parallel( value = True): 
    return _porcelain.Parallel( value)
def Scope( inherit = True, name = None): 
    return _porcelain.Scope( inherit)
def Environment( name, value = '', export = True): 
    return _porcelain.Environment(name, value, export)
def Compile( sourcefile, objectfile = None, directive = ''): 
    return  _porcelain.Compile( sourcefile, objectfile, directive)
def LinkLibrary(name,objectfiles,libs = [], version = None): 
    return _porcelain.LinkLibrary(name, objectfiles, libs, version)
def LinkArchive(name,objectfiles): 
    return _porcelain.LinkArchive(name,objectfiles)
def LinkExecutable( name, objectfiles, libraries = []): 
    return _porcelain.LinkExecutable( name, objectfiles, libraries)
def LinkResourceProxy( name, resource, libraries = [], directive = ''): 
    return _porcelain.LinkResourceProxy( name, resource, libraries, directive)
def Dependencies( target, dependencies): 
    return _porcelain.Dependencies( target, dependencies)
def Build(casualMakefile): 
    return _porcelain.Build(casualMakefile)
def LinkUnittest(name,objectfiles,libraries = [], test_target = True): 
    return _porcelain.LinkUnittest(name, objectfiles,libraries, test_target)
def Install( target, destination): 
    return _porcelain.Install( target, destination)
def Include( filename): 
    return _porcelain.Include( filename)
def IncludePaths( paths): 
    return _porcelain.IncludePaths( paths)
def LibraryPaths( paths): 
    return _porcelain.LibraryPaths( paths)



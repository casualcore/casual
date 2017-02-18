'''
    Created on 28 apr 2012
    
    @author: hbergk
    
    Contains the basic syntax to produce makefile
    '''

#
# Imports
#
import os

import casual.make.plumbing
import casual.make.porcelain
from casual.make.output import Output

#
# Select specific version or leave empty for latest
#
_plumbing = casual.make.plumbing.plumbing(version = 1.0)
_porcelain = casual.make.porcelain.porcelain(version = 1.0)


# def _set_ld_library_path():
#     
#     build_home = os.getenv('CASUAL_BUILD_HOME', '$HOME/git/casual')
#     
#     if not build_home.endswith( 'middleware'):
#         build_home = build_home + '/middleware'
#     
#     ld_library_path = build_home + '/common/bin:' +
#         build_home + '/xatmi/bin:' +
#         build_home + '/xatmi/bin:'   
#     
#     
#     print "#"
#     print "# Set LD_LIBRARY_PATH so that unittest has access to dependent libraries"
#     print "#"
#     print "space :=  "
#     print "space +=  "
#     print "formattet_library_path = $(subst -L,,$(subst $(space),:,$(LIBRARY_PATHS) $(DEFAULT_LIBRARY_PATHS)))"
#     print "LOCAL_LD_LIBRARY_PATH=$(formattet_library_path):$(PLATFORMLIB_DIR)"
#     print 
#     

#
# New functions adding functionality
#
def LinkServer( name, objectfiles, libraries, serverdefinition, resources=None, configuration=None):
    """
 Links an XATMI-server
 
 param: name        name of the server with out prefix or suffix.
     
 param: objectfiles    object files that is linked
 
 param: libraries        dependent libraries
 
 param: serverdefinition  path to the server definition file that configure the public services, 
                         and semantics.
                         Can also be a list of public services. I e.  ["service1", "service2"]
                     
 param: resources  optional - a list of XA resources. I e ["db2-rm"] - the names shall 
                correspond to those defined in $CASUAL_HOME/configuration/resources.(yaml|json|...)
                
  param: configuration optional - path to the resource configuration file
                 this should only be used when building casual it self.
    """
 
    path = _plumbing.executable_name_path(name)
    target = _plumbing.target(path, name)
     
    directive = "";
 
    if resources:
        directive += " --resource-keys " + ' '.join( resources)
        
    if configuration:
        directive += " --properties-file " + configuration
     
    if isinstance( serverdefinition, basestring):
        # We assume it is a path to a server-definition-file
        directive += ' --server-definition ' + serverdefinition
        
        _plumbing.add_dependency( [ path], [ serverdefinition], comments='dependency to server definition file') 
    else:
        directive += ' -s ' + ' '.join( serverdefinition)
 
    _plumbing.set_ld_path()
    
    return _plumbing.link( _plumbing.platform().link_server, target, objectfiles, libraries, directive, 'LD_LIBRARY_PATH=$(LOCAL_LD_LIBRARY_PATH) ')    
 
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
        
    _plumbing.set_ld_path()
    _plumbing.link( _plumbing.platform().link_client, target, objectfiles, libraries, directive, 'LD_LIBRARY_PATH=$(LOCAL_LD_LIBRARY_PATH) ')



def LinkResourceProxy( name, resource, libraries = [], directive = ''):
    """
 Links a resource proxy
 
 param: name        name of the result
 param: resource    key of the resource one wants to link
 param: directive   extra directives
    """
    
    target = _plumbing.target( _plumbing.executable_name_path( name), name)
    
    dependent_targets = _plumbing.library_targets( libraries)
    
    #
    # Convert library targets to names/files, 
    #
    libraries = _plumbing.target_base( libraries)
    
    destination_path = os.path.dirname( target.file)
    
    build_resource_proxy = _plumbing.platform().executable_name( 'casual-build-resource-proxy')
    
    directive += ' --link-directives "' + _plumbing.platform().link_directive( libraries) + ' $(INCLUDE_PATHS_DIRECTIVE) $(DEFAULT_INCLUDE_PATHS_DIRECTIVE) $(LIBRARY_PATHS_DIRECTIVE) $(DEFAULT_LIBRARY_PATHS_DIRECTIVE) $(DEFAULT_LIBS) $(LINK_DIRECTIVES_EXE)"'
    
    _plumbing.set_ld_path()
    
    print
    print "link: " + target.name
    print 
    print target.name + ': ' + target.file
    print
    print '   #'
    print '   # possible dependencies to other targets (in this makefile)'
    print '   depenency_' + target.name + ' = ' + _plumbing.multiline( dependent_targets)
    print
    print target.file + ': $(depenency_' + target.name + ')' + " $(USER_CASUAL_MAKE_FILE) | " + destination_path
    print '\t@LD_LIBRARY_PATH=$(LOCAL_LD_LIBRARY_PATH) ' + build_resource_proxy + ' --output ' + target.file + ' --resource-key ' + resource + ' ' + directive
    
    _plumbing.register_file_for_clean( target.file)
    _plumbing.register_path_for_create( destination_path)

    return target


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
def LinkLibrary(name,objectfiles,libs = []): 
    return _porcelain.LinkLibrary(name, objectfiles, libs)
def LinkArchive(name,objectfiles): 
    return _porcelain.LinkArchive(name,objectfiles)
def LinkExecutable( name, objectfiles, libraries = []): 
    return _porcelain.LinkExecutable( name, objectfiles, libraries)
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



import inspect
import os
import pprint

from casual.make.entity.target import Target, Recipe
import casual.make.entity.recipe as recipe
import casual.make.entity.model as model
import casual.make.entity.state as state

import casual.make.api as api
import casual.make.tools.executor as executor
import casual.make.platform.common as common

import importlib
compiler_handler = state.settings.compiler_handler()
selector = importlib.import_module( compiler_handler)

def BUILD_SERVER():
   return "casual-build-server"

# global setup for operations
link_server_target = model.register( 'link-server')
api.link_target.add_dependency( [link_server_target])

def caller():
   
   name = inspect.getouterframes( inspect.currentframe())[2][1]
   path = os.path.abspath( name)
   return model.register(name=path, filename=path, makefile = path)


def paths():
   class Paths:
      thirdparty = os.getenv('CASUAL_THIRDPARTY') if os.getenv('CASUAL_THIRDPARTY') else api.source_root() + "/../casual-thirdparty"

      install = os.getenv('CASUAL_HOME') if os.getenv('CASUAL_HOME') else '/opt/casual'
      
      class Include:
         def __init__( self, thirdparty):
            self.serialization = [ 
               thirdparty + '/rapidjson/include',
               thirdparty + '/yaml-cpp/include',
               thirdparty + '/pugixml/src'
            ]
            self.cppcodec = [ thirdparty + '/cppcodec/include']
            self.gtest = [ thirdparty + '/googletest/include']


      include = Include( thirdparty)

      class Library:
         def __init__( self, thirdparty):
            self.serialization = [ 
               thirdparty + '/yaml-cpp/bin',
               thirdparty + '/pugixml/bin'
            ]
            self.gtest = [ thirdparty + '/googletest/bin']

      library = Library( thirdparty)

   paths.state = Paths()
   return paths.state

# New functions adding functionality

def LinkServer( name, objects, libraries, serverdefinition, resources=None, configuration=None):
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

   makefile = caller()
   directory, dummy = os.path.split( makefile.filename())

   full_executable_name = selector.expanded_executable_name(name, directory)
   executable_target = model.register( full_executable_name, full_executable_name, makefile = makefile.filename())

   directive = []
   if resources:
      directive.append( "--resource-keys")
      directive += resources
      
   if configuration:
      directive.append( "--properties-file")
      directive.append( configuration)
   
   if isinstance( serverdefinition, str):
      # We assume it is a path to a server-definition-file
      directive.append( "--server-definition")
      directive.append( serverdefinition)      
   else:
      directive.append( "-s")
      directive += serverdefinition

   arguments = {
      'destination' : executable_target, 
      'objects' : objects, 
      'libraries': libraries, 
      'library_paths': model.library_paths( makefile.filename()), 
      'include_paths': model.include_paths( makefile.filename()), 
      'directive': directive
      }

   executable_target.add_recipe( 
      Recipe( link_server, arguments)
   ).add_dependency( objects + api.normalize_library_target( libraries))
   
   build_server = model.get( BUILD_SERVER())

   if build_server:
      executable_target.add_dependency(build_server)

   link_server_target.add_dependency( executable_target)

   api.make_clean_target( [executable_target], makefile)

   return executable_target


def link_server( input):
   """
   Recipe for linking objects to casual-servers
   """
   destination = input['destination']
   objects = recipe.retrieve_filenames( input['objects'])
   context_directory = os.path.dirname( input['destination'].makefile())
   directive = input['directive']
   libraries = input['libraries']
   library_paths = input['library_paths']
   include_paths = input['include_paths']

   BUILDSERVER = [ BUILD_SERVER(), "-c"] + selector.build_configuration['executable_linker']

   cmd = BUILDSERVER + \
         ['-o', destination.filename()] + directive + ['-f'] + [" ".join( objects)] + \
         selector.library_directive(libraries) +  \
         selector.library_paths_directive( library_paths) + \
         selector.build_configuration['link_directives_exe'] + \
         common.add_item_to_list( include_paths, '-I')

   executor.command( cmd, destination, context_directory)



//! 
//! Copyright (c) 2015, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!

#include "tools/common.h"
#include "tools/build/task.h"
#include "tools/build/generate.h"
#include "tools/build/transform.h"

#include "common/string.h"
#include "common/process.h"
#include "common/argument.h"
#include "common/file.h"
#include "common/execute.h"
#include "common/environment.h"
#include "common/server/service.h"
#include "common/exception/system.h"
#include "common/exception/handle.h"

#include "configuration/build/server.h"
#include "configuration/build/resource.h"
#include "configuration/resource/property.h"

#include <vector>
#include <string>
#include <iostream>
#include <fstream>
#include <map>
#include <algorithm>


namespace casual
{
   using namespace common;
   namespace tools
   {
      namespace build
      {
         namespace local
         {
            namespace
            {
               struct Settings
               {
                  build::Directive directive;

                  struct 
                  {
                      std::vector< std::string> names;
                     struct 
                     {
                        std::string mode;
                     } transaction;
                  } service;

                  struct
                  {
                     std::string definition;
                  } server;

                  struct 
                  {
                     std::string properties;
                     std::vector< std::string> keys;
                  } resource;



                  struct
                  {
                     std::string file;
                     bool keep = false;
                  } source;
               };

               namespace service
               {
                  auto argument( std::vector< std::string>& names)
                  {
                     return [&names]( const std::string& value, const std::vector< std::string>& values)
                     {
                        auto normalize_names = [&names]( auto& value)
                        {
                           auto splittet = string::split( value, ',');
                           
                           algorithm::append( 
                              algorithm::transform( splittet, []( auto& value){ return string::trim( std::move( value));}),
                              names);
                        };
                        normalize_names( value);
                        algorithm::for_each( values, normalize_names);
                     };
                  }
               } // service
               
               namespace complete
               {
                  namespace transaction
                  {
                     auto mode()
                     {
                        return []( auto&, bool) -> std::vector< std::string>
                        {
                           return { "automatic", "join", "none", "atomic"};
                        };
                     }
                  } // transaction
               } // complete

               struct State
               {
                  std::vector< model::Service> services;
                  std::vector< model::Resource> resources;
               };

               namespace transform
               {
                  auto state( const Settings& settings)
                  {
                     auto properties = settings.resource.properties.empty() ?
                        configuration::resource::property::get() : configuration::resource::property::get( settings.resource.properties);

                     auto definition = settings.server.definition.empty() ? 
                        decltype( configuration::build::server::get( "")){} : configuration::build::server::get( settings.server.definition);

                     State result;

                     result.resources = build::transform::resources( 
                        definition.resources,
                        settings.resource.keys,
                        properties);

                     result.services = build::transform::services(
                        definition.services,
                        settings.service.names,
                        settings.service.transaction.mode);

                     return result;
                  };

               } // transform

               namespace source
               {
                  common::file::scoped::Path file( const Settings& settings)
                  {
                     if( settings.source.file.empty())
                        return { common::file::name::unique( "server_", ".cpp")};
                     
                     return { settings.source.file};
                  }
               } // source

               void generate( const common::file::scoped::Path& path, const local::State& state)
               {
                  std::ofstream out{ path};
                  generate::server( out, state.resources, state.services);
               }

               void build( const common::file::scoped::Path& path, Settings settings)
               {
                  trace::Exit exit( "build server", settings.directive.verbose);

                  common::log::line( log, "path: ", path);

                  auto state = local::transform::state( settings);
                  local::generate( path, state);
                  
                  if( settings.directive.use_defaults)
                  {
                     // add "known" dependencies
                     common::algorithm::push_back_unique( "casual-xatmi", settings.directive.libraries);

                     if( common::environment::variable::exists( "CASUAL_HOME"))
                     {
                        auto casual_home = common::environment::variable::get( "CASUAL_HOME");

                        if( common::directory::exists( casual_home + "/include"))
                           common::algorithm::push_back_unique( casual_home + "/include", settings.directive.paths.include);

                        if( common::directory::exists( casual_home + "/lib"))
                           common::algorithm::push_back_unique( casual_home + "/lib", settings.directive.paths.library);
                     }

                     // add resource stuff
                     algorithm::append_unique( build::transform::libraries( state.resources), settings.directive.libraries);
                     algorithm::append_unique( build::transform::paths::include( state.resources), settings.directive.paths.include);
                     algorithm::append_unique( build::transform::paths::library( state.resources), settings.directive.paths.library);
                  }

                  build::task( path, settings.directive);
               }

               void main( int argc, char **argv)
               {
                  Settings settings;

                  {
                     trace::Exit log( "parse arguments", false);

                     using namespace casual::common::argument;

                     Parse{ "builds a casual xatmi server",
                        Option( std::tie( settings.directive.output), { "-o", "--output"}, "name of server to be built"),
                        Option( service::argument( settings.service.names), {"-s", "--service"}, "service names")( cardinality::any{}),
                        Option( std::tie( settings.server.definition), { "-d", "--definition", "--server-definition"}, "path to server definition file\n\ndeprecated: --server-definition"),
                        Option( option::one::many( settings.resource.keys), {"-r", "--resource-keys"}, "key of the resource")( cardinality::any{}),
                        Option( std::tie( settings.directive.compiler), {"-c", "--compiler"}, "compiler to use"),
                        Option( build::Directive::split( settings.directive.directives), {"-f", "--build-directives", "--link-directives"}, "additional compile and link directives\n\ndeprecated: --link-directives")( cardinality::any{}),
                        Option( std::tie( settings.resource.properties), {"-p", "--properties-file"}, "path to resource properties file"),
                        Option( std::tie( settings.service.transaction.mode), complete::transaction::mode(), {  "--default-transaction-mode"}, "the transaction mode for services specified with --service|-s"),
                        Option( option::toggle( settings.directive.use_defaults), { "--no-defaults"}, "do not add any default compiler/link directives\n\nuse --build-directives to add your own"),
                        Option( std::tie( settings.source.file), { "--source-file"}, "name of the intermediate source file"),
                        Option( option::toggle( settings.source.keep), {"-k", "--keep"}, "keep the intermediate source file"),
                        Option( option::toggle( settings.directive.verbose), {"-v", "--verbose"}, "verbose output"),
                     }( argc, argv);

                  }

                  // Generate file

                  auto source = local::source::file( settings);

                  auto source_keep = common::execute::scope( [keep = settings.source.keep, &source]()
                  { 
                     if( keep)
                        source.release();
                  });

                  build( source, std::move( settings));
               }        

            } // <unnamed>
         } // local

      } // build
   } // tools
} // casual

int main( int argc, char **argv)
{
   return casual::common::exception::guard( std::cerr, [=]()
   {
      casual::tools::build::local::main( argc, argv);
   });
}

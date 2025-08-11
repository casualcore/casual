//! 
//! Copyright (c) 2015, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!

#include "tools/common.h"
#include "tools/build/task.h"
#include "tools/build/setting.h"
#include "tools/build/generate.h"
#include "tools/build/transform.h"

#include "common/string.h"
#include "common/process.h"
#include "casual/argument.h"
#include "common/file.h"
#include "common/execute.h"
#include "common/environment.h"
#include "common/exception/guard.h"

#include "configuration/system.h"
#include "configuration/build/model/load.h"

#include <vector>
#include <string>
#include <iostream>
#include <fstream>
#include <map>
#include <algorithm>


namespace casual
{
   using namespace common;
   namespace tools::build
{
      namespace local
      {
         namespace
         {
            struct Settings
            {
               setting::Directive directive;

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
                  std::filesystem::path definition;
               } server;

               struct 
               {
                  std::vector< std::string> keys;
               } resource;

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
                        
                        algorithm::container::append( 
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
                     return []( bool test, auto values) -> std::vector< std::string>
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

               CASUAL_LOG_SERIALIZE(
                  CASUAL_SERIALIZE( services);
                  CASUAL_SERIALIZE( resources);
               )
            };

            namespace transform
            {
               auto state( const Settings& settings)
               {
                  Trace trace{ "tools::build::local::transform::state"};

                  auto system = settings.directive.system.configuration.empty() ?
                     configuration::system::get() : configuration::system::get( settings.directive.system.configuration);

                  auto definition = settings.server.definition.empty() ? 
                     decltype( configuration::build::model::load::server( {})){} : configuration::build::model::load::server( settings.server.definition);

                  State result;

                  result.resources = build::transform::resources( 
                     definition,
                     settings.resource.keys,
                     system);

                  result.services = build::transform::services(
                     definition,
                     settings.service.names,
                     settings.service.transaction.mode);

                  log::debug( "result: ", result);

                  return result;
               };

            } // transform

            namespace source
            {
               common::file::scoped::Path file( const Settings& settings)
               {
                  if( settings.directive.source.file.empty())
                     return { common::file::name::unique( "server_", ".cpp")};
                  
                  return { settings.directive.source.file};
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

               common::log::debug( "path: ", path);

               auto state = local::transform::state( settings);
               local::generate( path, state);
               
               if( settings.directive.use_defaults)
               {
                  // add "known" dependencies
                  common::algorithm::append_unique_value( "casual-xatmi", settings.directive.libraries);

                  auto append = []( const auto& path, auto& target)
                  {
                     if( std::filesystem::exists( path))
                        common::algorithm::append_unique_value( path.string(), target);
                  };

                  if( auto home = common::environment::variable::get< std::filesystem::path>( common::environment::variable::name::directory::install))
                  {
                     append( *home / "include", settings.directive.paths.include);
                     append( *home / "lib", settings.directive.paths.library);
                  }

                  // add resource stuff
                  algorithm::append_unique( build::transform::libraries( state.resources), settings.directive.libraries);
                  algorithm::append_unique( build::transform::paths::include( state.resources), settings.directive.paths.include);
                  algorithm::append_unique( build::transform::paths::library( state.resources), settings.directive.paths.library);
               }

               build::task( path, settings.directive);
            }

            namespace option
            {
               auto server_definition( Settings& settings)
               {
                  return argument::Option( 
                     std::tie( settings.server.definition),
                     argument::option::Names{ { "-d", "--definition"}, { "--server-definition"}},
                     "path to server definition file");
               }
               
            } // option

            void main( int argc, const char** argv)
            {
               Settings settings;

               {
                  trace::Exit log( "parse arguments", false);

                  auto outcome = argument::parse( "builds a casual xatmi server", common::algorithm::container::compose( 
                     local::option::server_definition( settings),
                     build::setting::options( settings.directive),
                     argument::Option( service::argument( settings.service.names), {"-s", "--service"}, "service names")( argument::cardinality::any()),
                     argument::Option( argument::option::one::many( settings.resource.keys), {"-r", "--resource-keys"}, "key of the resource")( argument::cardinality::any()),
                     argument::Option( std::tie( settings.service.transaction.mode), complete::transaction::mode(), {  "--default-transaction-mode"}, "the transaction mode for services specified with --service|-s")
                  ), argc, argv);

                  if( outcome != argument::Outcome::parsed)
                     return;
               }

               // Generate file
               auto source = local::source::file( settings);
               
               auto source_keep = common::execute::scope( [keep = settings.directive.source.keep, &source]()
               { 
                  if( keep)
                     source.release();
               });

               build( source, std::move( settings));
            }        

         } // <unnamed>
      } // local

   } // tools::build
} // casual

int main( int argc, const char** argv)
{
   return casual::common::exception::main::cli::guard( [=]()
   {
      casual::tools::build::local::main( argc, argv);
   });
}

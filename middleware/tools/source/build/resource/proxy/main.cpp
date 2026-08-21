//! 
//! Copyright (c) 2015, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!

#include "tools/common.h"
#include "tools/build/settings.h"
#include "tools/build/transform.h"
#include "tools/build/model.h"
#include "tools/build/generate.h"
#include "tools/build/task.h"


#include "casual/argument.h"
#include "common/environment.h"
#include "common/environment/expand.h"
#include "common/file.h"
#include "common/uuid.h"
#include "common/string.h"
#include "common/process.h"
#include "common/serialize/log.h"
#include "common/algorithm/container.h"

#include "common/exception/guard.h"
#include "common/code/raise.h"
#include "common/code/casual.h"
#include "common/execute.h"

#include "configuration/system.h"

#include <string>
#include <iostream>
#include <fstream>


namespace casual
{
   namespace tools::build::resource::proxy
   {
      namespace local
      {
         namespace
         {

            struct Settings 
            {
               build::Settings directive;
               build::settings::Resource resource;

               CASUAL_LOG_SERIALIZE(
                  CASUAL_SERIALIZE( directive);
                  CASUAL_SERIALIZE( resource);
               )
            };


            struct State
            {
               std::vector< model::Resource> resources;
            };

            namespace transform
            {
               auto state( const Settings& settings)
               {
                  auto system = build::settings::system( settings.directive);

                  State result;

                  result.resources = build::transform::resources( 
                     settings.resource.keys,
                     system);

                  return result;
               };

            } // transform

            void generate( std::ostream& out, const local::State& state)
            {
               if( state.resources.size() != 1)
                  common::code::raise::error( common::code::casual::invalid_argument, "expected exactly one resource, got: ", state.resources.size());

               auto& resource = state.resources.front();

               out << license::c << R"(

#include <casual/transaction/resource/proxy/server.h>
#include <xa.h>

#ifdef __cplusplus
extern "C" {
#endif

)";

               // Declare the xa_strut                     
               out << "extern struct xa_switch_t " << resource.xa_struct_name << ";";
               out << R"(

#ifdef __cplusplus
}
#endif

int main( int argc, const char** argv)
{

   struct casual_xa_switch_mapping xa_mapping[] = {
)";

               out << R"(      { ")" << resource.key << R"(", &)" << resource.xa_struct_name << "},";

               out << R"(
      { 0, 0} /* null ending */
   };

   struct casual_resource_proxy_service_argument serverArguments = {
         argc,
         argv,
         xa_mapping
   };


   /* Start the server */
   return casual_start_resource_proxy( &serverArguments);
}

)";

               // make sure we flush
               out << std::flush;
            }
 
            void build( const State& state, const std::filesystem::path& source, Settings settings)
            {
               verbose::log( settings, "build resource proxy");
               common::log::debug( "source", source);
               
               if( settings.directive.use_defaults)
               {
                  // add "known" dependencies
                  common::algorithm::append_unique_value( "casual-xatmi", settings.directive.libraries);
                  common::algorithm::append_unique_value( "casual-resource-proxy-server", settings.directive.libraries);

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
                  common::algorithm::append_unique( build::transform::libraries( state.resources), settings.directive.libraries);
                  common::algorithm::append_unique( build::transform::paths::include( state.resources), settings.directive.paths.include);
                  common::algorithm::append_unique( build::transform::paths::library( state.resources), settings.directive.paths.library);
               }

               build::task( source, settings.directive);
            }

            namespace source
            {
               common::file::scoped::Path file( const Settings& settings)
               {
                  if( settings.directive.source.file.empty())
                     return { common::file::name::unique( "rm_proxy_", ".cpp")};
                  
                  return { settings.directive.source.file};
               }
            } // source

            void main( int argc, const char** argv)
            {
               Settings settings;

               {
                  auto bind_append = []( std::vector< std::string>& option)
                  {
                     return [ &option]( std::vector< std::string> value)
                     {
                        common::algorithm::container::append( std::move( value), option);
                     };
                  };

                  auto outcome = argument::parse( "builds a resource proxy", 
                     common::algorithm::container::compose(
                        build::settings::resource::key::option( settings.resource),
                        build::settings::options( settings.directive),

                        // deprecated options
                        argument::Option( bind_append( settings.directive.directives), { {}, { "-c", "--compile-directives"}}, "additional compile directives"),
                        argument::Option( bind_append( settings.directive.directives), { {}, { "-l", "--link-directives"}}, "additional link directives"),
                        argument::Option( std::tie( settings.directive.source.keep), { {}, { "-s", "--keep-source"}}, "keep the generated source file")
                     ), argc, argv);

                  if( outcome != argument::Outcome::parsed)
                     return;
               }

               verbose::log( settings, CASUAL_NAMED_VALUE( settings));

               auto state = local::transform::state( settings);

               if( settings.directive.only_generate)
               {
                  local::generate( std::cout, state);
                  return;
               }

               // Generate source file
               auto source = local::source::file( settings);
               auto source_guard = tools::source::keep::guard( settings, source);

               {
                  std::ofstream out{ source};
                  local::generate( out, state);
                  verbose::log( settings, "generated source file: ", source);
               }

               local::build( state, source, std::move( settings));
            }

         } // <unnamed>
      } // local
   } // tools::build::resource::proxy
} // casual

int main( int argc, const char** argv)
{
   return casual::common::exception::main::cli::guard( [=]()
   {
      casual::tools::build::resource::proxy::local::main( argc, argv);
   });
}


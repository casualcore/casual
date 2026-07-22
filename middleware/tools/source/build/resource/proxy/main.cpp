//! 
//! Copyright (c) 2015, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!

#include "tools/common.h"
#include "tools/build/setting.h"
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
               setting::Mandatory directive;

               struct 
               {
                  std::string key;
               } resource;


               CASUAL_LOG_SERIALIZE(
                  CASUAL_SERIALIZE( directive);
                  CASUAL_SERIALIZE( resource.key);
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
                  auto system = settings.directive.system.configuration.empty() ?
                     configuration::system::get() : configuration::system::get( settings.directive.system.configuration);


                  State result;

                  result.resources = build::transform::resources( 
                     { settings.resource.key}, // raw keys from command line
                     system);

                  return result;
               };

            } // transform

            void generate( const common::file::scoped::Path& path, const local::State& state)
            {
               if( state.resources.size() != 1)
                  common::code::raise::error( common::code::casual::invalid_argument, "expected exactly one resource, got: ", state.resources.size());

               auto& resource = state.resources.front();

               std::ofstream out{ path};

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
 
            void build( const common::file::scoped::Path& path, Settings settings)
            {
               verbose::log( settings, "build resource proxy");
               common::log::debug( "path: ", path);

               auto state = local::transform::state( settings);

               local::generate( path, state);
               verbose::log( settings, "generated source file: ", path);

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

               build::task( path, settings.directive);
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
                        build::setting::mandatory::options( settings.directive),
                        argument::Option( std::tie( settings.resource.key), {{ "-r", "--resource-key"}, { "-k"}}, "key of the resource"),

                        // deprecated options
                        argument::Option( bind_append( settings.directive.directives), { {}, { "-c", "--compile-directives"}}, "additional compile directives"),
                        argument::Option( bind_append( settings.directive.directives), { {}, { "-l", "--link-directives"}}, "additional link directives"),
                        argument::Option( std::tie( settings.directive.source.keep), { {}, { "-s", "--keep-source"}}, "keep the generated source file")
                     ), argc, argv);

                  if( outcome != argument::Outcome::parsed)
                     return;
               }

               verbose::log( settings, CASUAL_NAMED_VALUE( settings));

               // Generate file
               common::file::scoped::Path path = local::source::file( settings);
               
               // make sure we keep the file if user has requested it
               auto path_keep_scope = common::execute::scope( [keep = settings.directive.source.keep, &path]()
               { 
                  if( keep)
                     path.release();
               });

               local::build( path, std::move( settings));
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


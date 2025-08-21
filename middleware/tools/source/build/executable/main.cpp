//!
//! Copyright (c) 2018, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!

#include "tools/build/task.h"
#include "tools/build/generate.h"
#include "tools/build/setting.h"
#include "tools/build/model.h"
#include "tools/build/transform.h"
#include "tools/common.h"

#include "casual/argument.h"
#include "common/environment.h"
#include "common/execute.h"
#include "common/file.h"

#include "common/exception/guard.h"
#include "common/code/raise.h"
#include "common/code/casual.h"

#include "configuration/system.h"
#include "configuration/build/model/load.h"


#include <iostream>

namespace casual
{
   using namespace common;

   namespace tools::build::executable
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
                  std::string definition;
               } executable;
               

               friend void validate( const Settings& settings)
               {
                  auto raise_if_empty = []( const auto& value, auto error)
                  {
                     if( value.empty())
                        code::raise::error( code::casual::invalid_argument, error);
                  };
                  raise_if_empty( settings.executable.definition, "no definition file provided");
               }
            };

            struct State
            {
               std::string entrypoint;
               std::vector< model::Resource> resources;
            };


            namespace transform
            {
               auto state( const Settings& settings)
               {
                  auto system = settings.directive.system.configuration.empty() ?
                     configuration::system::get() : configuration::system::get( settings.directive.system.configuration);

                  auto definition = configuration::build::model::load::executable( settings.executable.definition);

                  State result;

                  result.resources = build::transform::resources( 
                     definition.executable.resources,
                     {}, // no raw keys
                     system);

                  result.entrypoint = definition.executable.entrypoint;

                  return result;
               };

            } // transform

            namespace source
            {
               common::file::scoped::Path file( const Settings& settings)
               {
                  if( settings.directive.source.file.empty())
                     return { common::file::name::unique( "executable_", ".cpp")};

                  return { settings.directive.source.file};
               }
            } // source

            void generate( const common::file::scoped::Path& path, const local::State& state)
            {
               Trace trace{ "tools::build::executable::local::generate"};

               std::ofstream out{ path};
               generate::executable( out, state.resources, state.entrypoint);
            }

            void build( Settings settings)
            {
               Trace trace{ "tools::build::executable::local::build"};

               auto state = local::transform::state( settings);

               auto path = source::file( settings);

               auto source_keep = common::execute::scope( [ &path, keep = settings.directive.source.keep]()
               { 
                  if( keep)
                     path.release();
               });
               
               
               generate( path, state);

 
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

            
            void main( int argc, const char** argv)
            {
               Settings settings;

               {
                  auto outcome = argument::parse( "builds a casual executable",  common::algorithm::container::compose( 
                     argument::Option{ std::tie( settings.executable.definition), { "-d", "--definition"}, "path of the definition file"},
                     build::setting::mandatory::options( settings.directive)
                  ), argc, argv);

                  if( outcome != argument::Outcome::parsed)
                     return;
               }

               validate( settings);

               build( std::move( settings));
            }
         } // <unnamed>
      } // local
   } // tools::build::executable
} // casual

int main( int argc, const char** argv)
{
   return casual::common::exception::main::cli::guard( [=]()
   {
      casual::tools::build::executable::local::main( argc, argv);
   });
}
  
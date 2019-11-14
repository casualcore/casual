//!
//! Copyright (c) 2018, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!

#include "tools/build/task.h"
#include "tools/build/generate.h"
#include "tools/build/model.h"
#include "tools/build/transform.h"
#include "tools/common.h"

#include "common/exception/handle.h"
#include "common/argument.h"
#include "common/environment.h"
#include "common/execute.h"

#include "configuration/resource/property.h"
#include "configuration/build/executable.h"

#include <iostream>

namespace casual
{
   using namespace common;

   namespace tools
   {
      namespace build
      {
         namespace executable
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
                        std::string file;
                        bool keep = false;
                     } source;

                     struct
                     {
                        std::string definition;
                     } executable;

                     struct 
                     {
                        std::string properties;
                     } resource;

                     

                     friend void validate( const Settings& settings)
                     {
                        auto throw_if_empty = []( const auto& value, auto error)
                        {
                           if( value.empty())
                              throw common::exception::system::invalid::Argument{ error};
                        };
                        throw_if_empty( settings.executable.definition, "no definition file provided");
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
                        auto properties = settings.resource.properties.empty() ?
                           configuration::resource::property::get() : configuration::resource::property::get( settings.resource.properties);

                        auto definition = configuration::build::executable::get( settings.executable.definition);

                        State result;

                        result.resources = build::transform::resources( 
                           definition.resources,
                           {}, // no raw keys
                           properties);

                        result.entrypoint = definition.entrypoint;

                        return result;
                     };

                  } // transform

                  namespace source
                  {
                     common::file::scoped::Path file( const Settings& settings)
                     {
                        if( settings.source.file.empty())
                           return { common::file::name::unique( "executable_", ".cpp")};
                        
                        return { settings.source.file};
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
                     generate( path, state);

                     auto source_keep = common::execute::scope( [&]()
                     { 
                        if( settings.source.keep)
                           path.release();
                     });
                           
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
                  
                  void main( int argc, char* argv[])
                  {
                     Settings settings;

                     {
                        using namespace casual::common::argument;

                        Parse{ "builds a casual executable",
                           Option{ std::tie( settings.directive.output), { "-o", "--output"}, "name of executable to be built"},
                           Option{ std::tie( settings.executable.definition), { "-d", "--definition"}, "path of the definition file"},
                           Option( std::tie( settings.directive.compiler), { "-c", "--compiler"}, "compiler to use"),
                           Option( build::Directive::split( settings.directive.directives), { "-b", "--build-directives", "-cl"}, "additional compile and link directives\n\ndeprecated: -cl")( argument::cardinality::any{}),
                           Option{ option::toggle( settings.source.keep), { "-k", "--keep"}, "keep the intermediate file"},
                           Option( option::toggle( settings.directive.use_defaults), { "--no-defaults"}, "do not add any default compiler/link directives\n\nuse --build-directives to add your own"),
                           Option{ std::tie( settings.source.file), { "--source"}, "explicit name of the intermediate file"},
                           Option{ option::toggle( settings.directive.verbose), { "-v", "--verbose"}, "verbose output"},
                        }( argc, argv);
                     }

                     validate( settings);

                     build( std::move( settings));
                  }
               } // <unnamed>
            } // local
         } // executable
      } // build
   } // tools
} // casual

int main( int argc, char **argv)
{
   return casual::common::exception::guard( std::cerr, [=]()
   {
      casual::tools::build::executable::local::main( argc, argv);
   });
}
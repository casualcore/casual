//!
//! Copyright (c) 2018, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!

#include "tools/build/task.h"
#include "tools/build/generate.h"
#include "tools/build/resource.h"
#include "tools/common.h"

#include "common/exception/handle.h"
#include "common/argument.h"
#include "common/environment.h"

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

                     std::string definition;

                     bool keep = false;

                     friend void validate( const Settings& settings)
                     {
                        auto throw_error = []( auto error){
                           throw common::exception::system::invalid::Argument{ error};
                        };

                        validate( settings.directive);

                        //if( settings.compiler.empty()) throw_error( "compiler not set");
                        if( settings.definition.empty()) throw_error( "no definition file provided");
                     }
                  };

                  common::file::scoped::Path generate( 
                     const configuration::build::Executable& configuration, 
                     const std::vector< build::generate::Content>& content)
                  {
                     common::file::scoped::Path path{ common::file::name::unique( "executable_", ".cpp")};

                     std::ofstream out{ path};
                     out << license::c << R"(
#include <xatmi/executable.h>

#ifdef __cplusplus
extern "C" {
#endif

)";

                     // declare entry point
                     out << "extern int " << configuration.entrypoint << "( int, char**);" << '\n';
                     

                     out << "\n\n\n";

                     // declarations
                     for( auto& cont : content)
                     {
                        out << cont.before_main;
                     }



                     out << R"(
int main( int argc, char** argv)
{
)";

                     // declarations
                     for( auto& cont : content)
                     {
                        out << cont.inside_main;
                     }

                     out << R"(

   struct casual_executable_arguments arguments = {
         )";
                     out << "&" << configuration.entrypoint << R"(,
         argc,
         argv,
         xa_mapping
   };


   /*
   * Start the executable
   */
   return casual_run_executable( &arguments);

}


#ifdef __cplusplus
}
#endif

)";



                     return path;
                  }

                  void build( Settings settings)
                  {
                     Trace trace{ "tools::build::executable::build"};

                     auto configuration = configuration::build::executable::get( settings.definition);

                     auto resources = build::transform::resources( 
                        std::move( configuration.resources), 
                        configuration::resource::property::get());
                     
                     settings.directive.add( resources);

                     auto path = generate( configuration, { build::generate::resources( resources)});

                     auto name = settings.keep ? path.release() : path.path();

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

                     build::task( name, settings.directive);
                  }
                  
                  void main( int argc, char* argv[])
                  {
                     Settings settings;

                     {
                        argument::Parse{ "builds a casual executable",
                           argument::Option{ std::tie( settings.directive.output), { "-o", "--output"}, "name of executable to be built"},
                           argument::Option{ std::tie( settings.definition), { "-d", "--definition"}, "path of the definition file"},
                           argument::Option( std::tie( settings.directive.compiler), { "-c", "--compiler"}, "compiler to use"),
                           argument::Option( settings.directive.cli_directives(), { "-cl", "--compile-link-directives"}, 
                              "additional compile & link directives")( argument::cardinality::any{}),

                           argument::Option{ argument::option::toggle( settings.keep), { "-k", "--keep"}, "keep the intermediate file"},
                           argument::Option{ argument::option::toggle( settings.directive.verbose), { "-v", "--verbose"}, "verbose output"},
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

int main( int argc, char* argv[])
{
   try
   {
      casual::tools::build::executable::local::main( argc, argv);
   }
   catch( ...)
   {
      return casual::common::exception::handle( std::cerr);
   }
   return 0;

} // main
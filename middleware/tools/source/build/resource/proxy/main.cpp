//! 
//! Copyright (c) 2015, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!

#include "tools/common.h"

#include "common/argument.h"
#include "common/environment.h"
#include "common/environment/expand.h"
#include "common/file.h"
#include "common/uuid.h"
#include "common/string.h"
#include "common/process.h"
#include "common/serialize/log.h"

#include "common/exception/guard.h"
#include "common/code/raise.h"
#include "common/code/casual.h"

#include "configuration/system.h"

#include <string>
#include <iostream>
#include <fstream>


namespace casual
{
   namespace tools
   {
      namespace build
      {
         namespace resource
         {
            namespace proxy
            {
               namespace local
               {
                  namespace
                  {

                     void generate( std::ostream& out, const configuration::model::system::Resource& resource)
                     {
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

int main( int argc, char** argv)
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

#ifdef __cplusplus
}
#endif

)";

                        // make sure we flush
                        out << std::flush;
                     }

                     struct Settings
                     {

                        std::string output;
                        std::string key;

                        struct
                        {
                           std::string link;
                           std::string compile = "-O3";

                           CASUAL_CONST_CORRECT_SERIALIZE
                           (
                              CASUAL_SERIALIZE( link);
                              CASUAL_SERIALIZE( compile);
                           )

                        } directives;

                        std::string compiler = "g++";
                        bool verbose = false;
                        bool keep_source = false;

                        struct
                        {
                           std::string system;
                        } files;



                        CASUAL_LOG_SERIALIZE(
                           CASUAL_SERIALIZE( output);
                           CASUAL_SERIALIZE( key);
                           CASUAL_SERIALIZE( directives);
                           CASUAL_SERIALIZE( compiler);
                           CASUAL_SERIALIZE( verbose);
                           CASUAL_SERIALIZE( keep_source);
                           CASUAL_SERIALIZE( files.system);

                        )
                     };



                     int build( const std::filesystem::path& file, const configuration::model::system::Resource& resource, const Settings& settings)
                     {
                        trace::Exit log( "build resource proxy", settings.verbose);

                        // Compile and link
                        std::vector< std::string> arguments{ file, "-o", settings.output};

                        common::algorithm::append( common::string::adjacent::split( settings.directives.compile), arguments);
                        common::algorithm::append( common::string::adjacent::split( settings.directives.link), arguments);


                        for( auto& include_path : resource.paths.include)
                        {
                           arguments.emplace_back( "-I" + common::environment::expand( include_path));
                        }
                        // Add casual-paths, that we know will be needed
                        arguments.emplace_back( common::environment::expand( "-I${CASUAL_HOME}/include"));

                        for( auto& lib_path : resource.paths.library)
                        {
                           arguments.emplace_back( "-L" + common::environment::expand( lib_path));
                        }
                        // Add casual-paths, that we know will be needed
                        arguments.emplace_back( common::environment::expand( "-L${CASUAL_HOME}/lib"));


                        for( auto& lib : resource.libraries)
                        {
                           arguments.emplace_back( "-l" + lib);
                        }
                        // Add casual-lib, that we know will be needed
                        arguments.emplace_back( "-lcasual-resource-proxy-server");


                        if( settings.verbose)
                        {
                           std::clog << settings.compiler << " " << common::string::join( arguments, " ") << '\n';
                        }

                        {
                           trace::Exit log( "execute " + settings.compiler, settings.verbose);
                           return common::process::execute( settings.compiler, arguments);

                        }

                     }


                     configuration::model::system::Resource configuration( const Settings& settings)
                     {
                        trace::Exit log( "read resource properties configuration", settings.verbose);

                        auto system = settings.files.system.empty() ?
                              configuration::system::get() : configuration::system::get( settings.files.system);

                        if( auto found = common::algorithm::find( system.resources, settings.key))
                           return *found;

                        common::code::raise::error( common::code::casual::invalid_argument, "resource-key: ", settings.key, " not found");
                     }

                     void main( int argc, char* argv[])
                     {
                        Settings settings;

                        {
                           using namespace common::argument;

                           Parse{ "builds a resource proxy",
                              Option( std::tie( settings.output), { "-o", "--output"}, "name of the resulting resource proxy"),
                              Option( std::tie( settings.key), { "-k", "--resource-key"}, "key of the resource"),
                              Option( std::tie( settings.compiler), { "-c", "--compiler"}, "compiler to use"),

                              Option( std::tie( settings.directives.compile), { "-c", "--compile-directives"}, "additional compile directives"),
                              Option( std::tie( settings.directives.link), { "-l", "--link-directives"}, "additional link directives"),

                              Option( std::tie( settings.files.system), option::keys( { "--system-configuration"}, {"-p", "--properties-file"}), "path to system configuration file"),
                              Option( std::tie( settings.verbose), { "-v", "--verbose"}, "verbose output"),
                              Option( std::tie( settings.keep_source), { "-s", "--keep-source"}, "keep the generated source file")
                           }( argc, argv);

                           if( settings.verbose)
                              common::log::line( std::cout, CASUAL_NAMED_VALUE( settings));
                        }

                        auto xa_switch = local::configuration( settings);

                        if( settings.verbose)
                           common::log::line( std::cout, '\n', CASUAL_NAMED_VALUE( xa_switch));

                        if( settings.output.empty())
                           settings.output = xa_switch.server;

                        // Generate file
                        common::file::scoped::Path path( common::file::name::unique( "rm_proxy_", ".cpp"));

                        {
                           std::ofstream file( path);

                           trace::Exit log( "generate file: " + path.string(), settings.verbose);

                           local::generate( file, xa_switch);
                        }

                        build( path, xa_switch, settings);

                        if( settings.keep_source)
                        {
                           path.release();
                        }
                     }
                  } // <unnamed>
               } // local
            } // proxy
         } // resource
      } // build
   } // tools
} // casual

int main( int argc, char **argv)
{
   return casual::common::exception::main::cli::guard( [=]()
   {
      casual::tools::build::resource::proxy::local::main( argc, argv);
   });
}


//!
//! casual
//!


#include "common/arguments.h"
#include "common/environment.h"
#include "common/file.h"
#include "common/uuid.h"
#include "common/error.h"
#include "common/string.h"
#include "common/process.h"
#include "common/exception.h"
#include "configuration/resource/property.h"

#include "sf/log.h"

#include <string>
#include <iostream>
#include <fstream>





namespace casual
{

   namespace local
   {
      namespace
      {

         void generate( std::ostream& out, const configuration::resource::Property& resource)
         {
            out << R"(   
/*
* Some licence....
*
*/

#include <transaction/resource/proxy_server.h>
#include <xa.h>

#ifdef __cplusplus
extern "C" {
#endif

)";

            //
            // Declare the xa_strut
            //
         
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


   /*
   // Start the server
   */
   return casual_start_reource_proxy( &serverArguments);

}


#ifdef __cplusplus
}
#endif

)";

            //
            // make sure we flush
            //
            out << std::flush;

         }

         struct Settings
         {

            std::string output;
            std::string key;

            struct
            {
               std::string link;
               std::string compile;

               CASUAL_CONST_CORRECT_SERIALIZE
               (
                  archive & CASUAL_MAKE_NVP( link);
                  archive & CASUAL_MAKE_NVP( compile);
               )

            } directives;

            std::string compiler = "g++";
            bool verbose = false;
            bool keep_source = false;

            std::string properties_file;


            CASUAL_CONST_CORRECT_SERIALIZE
            (
               archive & CASUAL_MAKE_NVP( output);
               archive & CASUAL_MAKE_NVP( key);
               archive & CASUAL_MAKE_NVP( directives);
               archive & CASUAL_MAKE_NVP( compiler);
               archive & CASUAL_MAKE_NVP( verbose);
               archive & CASUAL_MAKE_NVP( keep_source);
               archive & CASUAL_MAKE_NVP( properties_file);

            )
         };


         namespace trace
         {
            struct Exit
            {
               template< typename T>
               Exit( T&& information, bool print) : m_information( std::forward< T>( information)), m_print( print) {}

               ~Exit()
               {
                  if( m_print)
                  {
                     if( std::uncaught_exception())
                     {
                        std::cerr << m_information << " - failed" << std::endl;
                     }
                     else
                     {
                        std::cout << m_information << " - ok" << std::endl;
                     }
                  }

               }

               std::string m_information;
               bool m_print;
            };
         } // trace


         int build( const std::string& c_file, const configuration::resource::Property& resource, const Settings& settings)
         {
            trace::Exit log( "build resurce proxy", settings.verbose);
            //
            // Compile and link
            //

            std::vector< std::string> arguments{ c_file, "-o", settings.output};

            common::range::append( common::string::adjacent::split( settings.directives.compile), arguments);
            common::range::append( common::string::adjacent::split( settings.directives.link), arguments);


            for( auto& include_path : resource.paths.include)
            {
               arguments.emplace_back( "-I" + common::environment::string( include_path));
            }
            // Add casual-paths, that we know will be needed
            arguments.emplace_back( common::environment::string( "-I${CASUAL_HOME}/include"));

            for( auto& lib_path : resource.paths.library)
            {
               arguments.emplace_back( "-L" + common::environment::string( lib_path));
            }
            // Add casual-paths, that we know will be needed
            arguments.emplace_back( common::environment::string( "-L${CASUAL_HOME}/lib"));


            for( auto& lib : resource.libraries)
            {
               arguments.emplace_back( "-l" + lib);
            }
            // Add casual-lib, that we know will be needed
            arguments.emplace_back( "-lcasual-resource-proxy-server");


            if( settings.verbose)
            {
               std::clog << settings.compiler << " " << common::string::join( arguments, " ") << std::endl;
            }

            {
               trace::Exit log( "execute " + settings.compiler, settings.verbose);
               return common::process::execute( settings.compiler, arguments);

            }

         }


         configuration::resource::Property configuration( const Settings& settings)
         {
            trace::Exit log( "read resource properties configuration", settings.verbose);

            auto swithces = settings.properties_file.empty() ?
                  configuration::resource::property::get() : configuration::resource::property::get( settings.properties_file);

            auto found = common::range::find_if( swithces,
               [&]( const configuration::resource::Property& value){ return value.key == settings.key;});

            if( ! found)
            {
               throw common::exception::invalid::Argument( "resource-key: " + settings.key + " not found");
            }
            return *found;
         }

      } // <unnamed>
   } // local
} // casual

int main( int argc, char **argv)
{
   using namespace casual;

   try
   {
      local::Settings settings;

      {
         using namespace casual::common;

         Arguments handler{ {
            argument::directive( {"-o", "--output"}, "name of the resulting resource proxy", settings.output),
            argument::directive( {"-k", "--resource-key"}, "key of the resource", settings.key),
            argument::directive( {"-c", "--compiler"}, "compiler to use", settings.compiler),

            argument::directive( {"-c", "--compile-directives"}, "additional compile directives", settings.directives.compile),
            argument::directive( {"-l", "--link-directives"}, "additional link directives", settings.directives.link),

            argument::directive( {"-p", "--resource-properties"}, "path to resource properties file", settings.properties_file),
            argument::directive( {"-v", "--verbose"}, "verbose output", settings.verbose),
            argument::directive( {"-s", "--keep-source"}, "keep the generated source file", settings.keep_source)
         }};

         handler.parse( argc, argv);

         if( settings.verbose)
         {
            std::cout << std::endl << CASUAL_MAKE_NVP( settings);
         }

      }

      if( settings.verbose) std::cout << "";


      auto xa_switch = local::configuration( settings);

      if( settings.verbose)
      {
         std::cout << std::endl << CASUAL_MAKE_NVP( xa_switch) << std::endl;
      }

      if( settings.output.empty())
      {
         settings.output = xa_switch.server;
      }

      //
      // Generate file
      //
      common::file::scoped::Path path( common::file::name::unique( "rm_proxy_", ".c"));

      {
         local::trace::Exit log( "generate file:  " + path.path(), settings.verbose);

         std::ofstream file( path);
         local::generate( file, xa_switch);

         file.close();
      }

      if( settings.keep_source)
      {
         path.release();
      }

      return build( path, xa_switch, settings);


   }
   catch( const common::exception::invalid::base& exception)
   {
      std::cerr << "error: " << exception << std::endl;
      return 1;
   }
   catch( ...)
   {
      std::cerr << "error: details is found in casual.log\n";
      return common::error::handler();
   }

   return 0;
}


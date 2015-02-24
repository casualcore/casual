//!
//! build.cpp
//!
//! Created on: Aug 4, 2013
//!     Author: Lazan
//!


#include "common/arguments.h"
#include "common/environment.h"
#include "common/file.h"
#include "common/uuid.h"
#include "common/error.h"
#include "common/string.h"
#include "common/process.h"
#include "common/exception.h"


#include "config/xa_switch.h"

#include "sf/log.h"

#include <string>
#include <iostream>
#include <fstream>



using namespace casual;





void generate( std::ostream& out, const config::xa::Switch& xa_switch)
{
   out << R"(   
/*
* Some licence....
*
*/

#include "transaction/resource/proxy_server.h"
#include <xa.h>

#ifdef __cplusplus
extern "C" {
#endif

)";

   //
   // Declare the xa_strut
   //

   out << "extern struct xa_switch_t " << xa_switch.xa_struct_name << ";";


   out << R"(


int main( int argc, char** argv)
{

   struct casual_xa_switch_mapping xa_mapping[] = {
)";

   out << R"(      { ")" << xa_switch.key << R"(", &)" << xa_switch.xa_struct_name << "},";

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
   std::string resourceKey;

   std::string linkDirectives;

   std::string compiler = "g++";
   bool verbose = false;
   bool keepSource = false;


   template< typename A>
   void serialize( A& archive)
   {
      archive & CASUAL_MAKE_NVP( output);
      archive & CASUAL_MAKE_NVP( resourceKey);
      archive & CASUAL_MAKE_NVP( linkDirectives);
      archive & CASUAL_MAKE_NVP( compiler);
      archive & CASUAL_MAKE_NVP( verbose);
      archive & CASUAL_MAKE_NVP( keepSource);

   }
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


int build( const std::string& c_file, const config::xa::Switch& xa_switch, const Settings& settings)
{
   trace::Exit log( "build resurce proxy", settings.verbose);
   //
   // Compile and link
   //

   std::vector< std::string> arguments{ c_file, "-o", settings.output};

   for( auto& lib : xa_switch.libraries)
   {
      arguments.emplace_back( "-l" + lib);
   }

   auto linkDirective = common::string::adjacent::split( settings.linkDirectives);
   arguments.insert( std::end( arguments), std::begin( linkDirective), std::end( linkDirective));

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


config::xa::Switch configuration( const Settings& settings)
{
   trace::Exit log( "read xa-switch configuration", settings.verbose);

   auto swithces = config::xa::switches::get();

   auto findKey = std::find_if(
      std::begin( swithces),
      std::end( swithces),
      [&]( const config::xa::Switch& value){ return value.key == settings.resourceKey;});

   if( findKey == std::end( swithces))
   {
      throw common::exception::invalid::Argument( "resource-key: " + settings.resourceKey + " not found");
   }
   return *findKey;
}



int main( int argc, char **argv)
{
   try
   {
      Settings settings;

      {
         using namespace casual::common;

         Arguments handler;

         handler.add(
            argument::directive( {"-o", "--output"}, "name of the resulting proxy", settings.output),
            argument::directive( {"-r", "--resource-key"}, "key of the resource", settings.resourceKey),
            argument::directive( {"-c", "--compiler"}, "compiler to use", settings.compiler),
            argument::directive( {"-l", "--link-directives"}, "additional link directives", settings.linkDirectives),
            argument::directive( {"-v", "--verbose"}, "verbose output", settings.verbose),
            argument::directive( {"-s", "--keep-source"}, "keep the generated source file", settings.keepSource));

         if( ! handler.parse( argc, argv))
         {
            return 1;
         }


         if( settings.verbose)
         {
            std::cout << std::endl << CASUAL_MAKE_NVP( settings);
         }

      }

      if( settings.verbose) std::cout << "";


      auto xa_switch = configuration( settings);

      if( settings.output.empty())
      {
         settings.output = xa_switch.server;
      }

      //
      // Generate file
      //
      common::file::scoped::Path path( common::file::unique( "xa_switch_", ".c"));

      {
         trace::Exit log( "generate file:  " + path.path(), settings.verbose);

         std::ofstream file( path);
         generate( file, xa_switch);

         file.close();
      }

      if( settings.keepSource)
      {
         path.release();
      }

      return build( path, xa_switch, settings);


   }
   catch( const common::exception::invalid::Base& exception)
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


#include <vector>
#include <string>
//!
//! build_server.cpp
//!
//! Created on: Aug 7, 2013
//!     Author: Lazan
//!

#include "common/string.h"
#include "common/process.h"
#include "common/arguments.h"
#include "common/file.h"
#include "common/error.h"
#include "common/uuid.h"

#include "sf/namevaluepair.h"
#include "config/xa_switch.h"



#include <vector>
#include <string>
#include <iostream>
#include <fstream>
#include <map>






using namespace casual;


struct Settings
{
   Settings()
   {
      auto switches = config::xa::switches::get();

      for( auto& xa : switches)
      {
         xa_mapping.emplace( xa.key, std::move( xa));
      }
   }

   std::string output;
   std::string resourceKeys;

   std::vector< std::string> services;
   std::vector< std::string> resources;
   std::vector< std::string> compileLinkDirective;

   std::string compiler = "gcc";
   bool verbose = false;


   std::vector< config::xa::Switch> xa_switches;

   void setResources( const std::vector< std::string>& value)
   {
      auto splitted = split( value);

      for( auto& resource : splitted)
      {
         auto findIter = xa_mapping.find( resource);
         if( findIter != std::end( xa_mapping))
         {
            xa_switches.push_back( std::move( findIter->second));
            xa_mapping.erase( findIter);
         }
      }
   }

   void setServices( const std::vector< std::string>& value)
   {
      append( services, split( value, ','));
   }

   void setCompileLinkDirective( const std::vector< std::string>& value)
   {
      append( compileLinkDirective, split( value));
   }




   template< typename A>
   void serialize( A& archive)
   {
      archive & CASUAL_MAKE_NVP( output);
      archive & CASUAL_MAKE_NVP( services);
      archive & CASUAL_MAKE_NVP( resources);
      archive & CASUAL_MAKE_NVP( verbose);
      archive & CASUAL_MAKE_NVP( compiler);

   }
private:

   std::map< std::string, config::xa::Switch> xa_mapping;

   void append( std::vector< std::string>& target, const std::vector< std::string>& source)
   {
         target.insert( std::end( target), std::begin( source), std::end( source));
   }

   std::vector< std::string> split( const std::vector< std::string>& source, typename std::string::value_type delimiter = ' ')
   {
      std::vector< std::string> result;

      for( auto& resource : source)
      {
         auto splitted = common::string::split( resource, delimiter);
         result.insert( std::end( result), std::begin( splitted), std::end( splitted));
      }

      return result;
   }

};



void generate( std::ostream& out, Settings& settings)
{
   out << R"(   
/*
* Some licence....
*
*/

#include <xatmi.h>
#include <xatmi_server.h>

#ifdef __cplusplus
extern "C" {
#endif

)";


   //
   // declare services
   //
   for( auto& service : settings.services)
   {
      out << "extern void " << service << "( TPSVCINFO *serviceInfo);" << std::endl;
   }

   out << "\n\n\n";

   //
   // Declare the xa_struts
   //
   for( auto& xa : settings.xa_switches)
   {
      out << "extern struct xa_switch_t " << xa.xa_struct_name << ";" << std::endl;
   }



   out << R"(


int main( int argc, char** argv)
{

   struct casual_service_name_mapping service_mapping[] = {)";

      for( auto& service : settings.services)
      {
         out << R"(
      {&)" << service << R"(, ")" << service << R"("},)";
      }

         out << R"(
      { 0, 0} /* null ending */
   };


   struct casual_xa_switch_mapping xa_mapping[] = {)";

   for( auto& xa : settings.xa_switches)
   {
      out << R"(
      { ")" << xa.key << R"(", &)" << xa.xa_struct_name << "},";
   }

   out << R"(
      { 0, 0} /* null ending */
   };

   struct casual_server_argument serverArguments = {
         service_mapping,
         &tpsvrinit,
         &tpsvrdone,
         argc,
         argv,
         xa_mapping
   };


   /*
   // Start the server
   */
   return casual_start_server( &serverArguments);

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


int build( const std::string& c_file, const Settings& settings)
{
   trace::Exit log( "build resurce proxy", settings.verbose);
   //
   // Compile and link
   //

   std::vector< std::string> arguments{ c_file, "-o", settings.output};

   for( auto& xa : settings.xa_switches)
   {
      for( auto& lib : xa.libraries)
      {
         arguments.emplace_back( "-l" + lib);
      }
   }


   arguments.insert( std::end( arguments), std::begin( settings.compileLinkDirective), std::end( settings.compileLinkDirective));

   arguments.emplace_back( "-lcasual-xatmi");

   if( settings.verbose)
   {
      std::clog << settings.compiler << " " << common::string::join( arguments, " ") << std::endl;
   }

   {
      trace::Exit log( "execute " + settings.compiler, settings.verbose);
      return common::process::execute( settings.compiler, arguments);

   }

}


int main( int argc, char **argv)
{
   try
   {
      Settings settings;

      {
         trace::Exit log( "parse arguments", true);

         using namespace casual::common;

         Arguments handler;

         handler.add(
            argument::directive( {"-o", "--output"}, "name of server to be built", settings.output),
            argument::directive( {"-s", "--services"}, "comma separated list of services", settings, &Settings::setServices),
            argument::directive( {"-r", "--resource-keys"}, "key of the resource", settings, &Settings::setResources),
            argument::directive( {"-c", "--compiler"}, "compiler to use", settings.compiler),
            argument::directive( {"-f", "--link-directives"}, "additional compile and link directives", settings, &Settings::setCompileLinkDirective),
            argument::directive( {"-v", "--verbose"}, "verbose output", settings.verbose));

         if( ! handler.parse( argc, argv))
         {
            return 1;
         }

         //sf::archive::logger::Writer writer;
         //writer << CASUAL_MAKE_NVP( settings);


      }

      if( settings.verbose) std::cout << "";

      //
      // Generate file
      //
      common::file::ScopedPath path( "server_" + common::Uuid::make().string() + ".c");

      {
         trace::Exit log( "generate file:  " + path.path(), settings.verbose);

         std::ofstream file( path);
         generate( file, settings);

         file.close();
      }

      return build( path, settings);

   }
   catch( ...)
   {
      return common::error::handler();
   }

   return 0;
}

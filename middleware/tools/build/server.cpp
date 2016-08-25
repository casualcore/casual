//!
//! casual
//!

#include "common/string.h"
#include "common/process.h"
#include "common/arguments.h"
#include "common/file.h"
#include "common/error.h"
#include "common/uuid.h"
#include "common/environment.h"
#include "common/server/service.h"

#include "sf/namevaluepair.h"
#include "config/xa_switch.h"
#include "config/serverdefinition.h"



#include <vector>
#include <string>
#include <iostream>
#include <fstream>
#include <map>
#include <algorithm>






using namespace casual;


struct Settings
{

   struct Service
   {

      Service( const std::string& name) : name( name), function( name) {}
      Service() = default;
      std::string name;
      std::string function;
      std::uint64_t type = 0;
      common::service::transaction::Type transaction = common::service::transaction::Type::automatic;
   };

   std::string output;

   std::vector< Service> services;

   std::vector< std::string> compileLinkDirective;

   void set_server_definition_path( const std::string& file)
   {
      auto server = config::build::server::get( file);

      using service_type = config::build::server::Server::Service;

      common::range::transform( server.services, services, []( const service_type& service)
            {
               static std::map< std::string, int> type{
                  {"casual-sf", common::server::Service::Type::cCasualSF},
                  {"casual-admin", common::server::Service::Type::cCasualAdmin},
               };

               Service result;
               result.function = service.function;
               result.name = service.name;
               result.type = type[ service.type];
               result.transaction = common::service::transaction::mode( service.transaction);

               return result;
            });

   }

   std::string compiler = "gcc";
   bool verbose = false;
   bool keep = false;


   std::string xa_resource_file;
   std::vector< std::string> resources;



   const std::vector< config::xa::Switch>& get_xa_swiches() const
   {
      auto initialize = [&](){
         std::vector< config::xa::Switch> result;

         if( ! resources.empty())
         {
            auto switches = xa_resource_file.empty() ?
                  config::xa::switches::get() : config::xa::switches::get( xa_resource_file);

            for( auto& resource : resources)
            {
               auto found = common::range::find_if( switches, [&]( const config::xa::Switch& s){
                  return s.key == resource;
               });

               if( found)
               {
                  result.push_back( std::move( *found));
               }
               else
               {
                  throw common::exception::invalid::Argument{ "invalid resource key - " + resource };
               }
            }
         }
         return result;
      };

      static auto xa_switches = initialize();
      return xa_switches;
   }


   void set_resources( const std::vector< std::string>& value)
   {
      auto splitted = split( value);

      for( auto& resource : splitted)
      {
         auto insert_point = std::lower_bound( std::begin( resources), std::end( resources), resource);

         if( insert_point == std::end( resources) || *insert_point != resource)
         {
            resources.insert( insert_point, std::move( resource));
         }
      }
   }


   void set_services( const std::vector< std::string>& value)
   {
      auto splittet = split( value, ',');

      common::range::transform( splittet, services, []( const std::string& name){
         return Service{ name};
         });
   }


   void set_compile_link_directive( const std::vector< std::string>& value)
   {
      // std::clog << "setCompileLinkDirective" << std::endl;
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

   void append( std::vector< std::string>& target, const std::vector< std::string>& source)
   {
      std::copy( std::begin( source), std::end( source), std::back_inserter( target));
   }

   std::vector< std::string> split( const std::vector< std::string>& source, typename std::string::value_type delimiter = ' ')
   {
      std::vector< std::string> result;

      for( auto& resource : source)
      {
         auto splitted = common::string::adjacent::split( resource, delimiter);
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
      out << "extern void " << service.function << "( TPSVCINFO *serviceInfo);" << std::endl;
   }

   out << "\n\n\n";

   //
   // Declare the xa_struts
   //
   for( auto& xa : settings.get_xa_swiches())
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
      {&)" << service.function << R"(, ")" << service.name << R"(", )" << service.type << ", " << common::cast::underlying( service.transaction) << "},";
      }

         out << R"(
      { 0, 0, 0, 0} /* null ending */
   };


   struct casual_xa_switch_mapping xa_mapping[] = {)";

   for( auto& xa : settings.get_xa_swiches())
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
   trace::Exit log( "build server", settings.verbose);
   //
   // Compile and link
   //

   std::vector< std::string> arguments{ c_file, "-o", settings.output};

   for( auto& xa : settings.get_xa_swiches())
   {
      for( auto& lib : xa.libraries)
      {
         arguments.emplace_back( "-l" + lib);
      }
   }


   arguments.insert( std::end( arguments), std::begin( settings.compileLinkDirective), std::end( settings.compileLinkDirective));

   arguments.emplace_back( "-lcasual-xatmi");

   arguments.push_back( "-I${CASUAL_HOME}/include");
   arguments.push_back( "-L${CASUAL_HOME}/lib");

   //
   // Make sure we resolve environment stuff
   //
   for( auto& argument : arguments)
   {
      argument = common::environment::string( argument);
   }

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
         trace::Exit log( "parse arguments", false);

         using namespace casual::common;

         Arguments handler{{
            argument::directive( {"-o", "--output"}, "name of server to be built", settings.output),
            argument::directive( {"-s", "--service"}, "service names", settings, &Settings::set_services),
            argument::directive( {"-p", "--path"}, "service names", settings, &Settings::set_server_definition_path),
            argument::directive( {"-r", "--resource-keys"}, "key of the resource", settings, &Settings::set_resources),
            argument::directive( {"-c", "--compiler"}, "compiler to use", settings.compiler),
            argument::directive( {"-f", "--link-directives"}, "additional compile and link directives", settings, &Settings::set_compile_link_directive),
            argument::directive( {"-xa", "--xa-resource-file"}, "path to resource definition file", settings.xa_resource_file),
            argument::directive( {"-v", "--verbose"}, "verbose output", settings.verbose),
            argument::directive( {"-k", "--keep"}, "keep the intermediate file", settings.keep)
         }};

         handler.parse( argc, argv);
      }

      if( settings.verbose) std::cout << "";

      //
      // Generate file
      //

      common::file::scoped::Path path( common::file::name::unique( "server_", ".c"));
      //std::string path( "server_" + common::Uuid::make().string() + ".c");

      {
         trace::Exit log( "generate file:  " + path.path(), settings.verbose);

         std::ofstream file( path);
         generate( file, settings);

         file.close();
      }

      if( settings.keep)
      {
         path.release();
      }

      return build( path, settings);

   }
   catch( const std::exception& exception)
   {
      std::cerr << "error: " << exception.what() << std::endl;
      return common::error::handler();
   }
   catch( ...)
   {
      std::cerr << "error: unknown" << std::endl;
      return 10;
   }

   return 0;
}

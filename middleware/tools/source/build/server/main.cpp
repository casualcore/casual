//! 
//! Copyright (c) 2015, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!

#include "tools/common.h"
#include "tools/build/task.h"
#include "tools/build/generate.h"

#include "common/string.h"
#include "common/process.h"
#include "common/argument.h"
#include "common/file.h"
#include "common/uuid.h"
#include "common/environment.h"
#include "common/server/service.h"
#include "common/exception/system.h"
#include "common/exception/handle.h"

#include "configuration/build/server.h"
#include "configuration/build/resource.h"
#include "configuration/resource/property.h"

#include <vector>
#include <string>
#include <iostream>
#include <fstream>
#include <map>
#include <algorithm>


namespace casual
{
   namespace tools
   {
      namespace build
      {
         namespace local
         {
            namespace
            {
               struct Settings
               {

                  struct Service
                  {

                     Service( const std::string& name) : name( name), function( name) {}
                     Service() = default;
                     std::string name;
                     std::string function;
                     std::string category;
                     common::service::transaction::Type transaction = common::service::transaction::Type::automatic;
                  };

                  Settings()
                  {
                     directive.directives = { "-O3"};
                  }

                  build::Directive directive;

                  std::vector< Service> services;

                  void set_server_definition_path( const std::string& file)
                  {
                     const auto server = configuration::build::server::get( file);

                     common::algorithm::append( server.resources, resources); 

                     common::algorithm::transform( server.services, services, []( const auto& service)
                     {
                        Service result;
                        result.function = service.function.value_or( service.name);
                        result.name = service.name;

                        if( service.category) { result.category = service.category.value();}

                        result.transaction = common::service::transaction::mode( service.transaction.value_or( "auto"));

                        return result;
                     });

                  }
                  bool keep = false;


                  std::string properties_file;
                  std::vector< configuration::build::Resource> resources;


                  void set_resources( const std::vector< std::string>& value)
                  {
                     auto splitted = split( value);

                     for( auto& resource : splitted)
                     {
                        auto found = common::algorithm::find_if( resources, [&]( auto& value){ return value.key == resource;});

                        if( ! found)
                        {
                           resources.push_back( { resource, ""});
                        }
                     }
                  }


                  void set_services( const std::vector< std::string>& value)
                  {
                     auto splittet = split( value, ',');

                     common::algorithm::transform( splittet, services, []( const std::string& name){
                        return Service{ name};
                        });
                  }


                  friend void validate( const Settings& settings)
                  {
                     validate( settings.directive);
                  }

               private:

                  std::vector< std::string> split( const std::vector< std::string>& source, typename std::string::value_type delimiter = ' ')
                  {
                     std::vector< std::string> result;

                     for( auto& resource : source)
                     {
                        auto splitted = common::string::adjacent::split( resource, delimiter);
                        common::algorithm::append( splitted, result);
                     }
                     return result;
                  }
                  

               };


               common::file::scoped::Path generate( 
                  const Settings& settings, 
                  const std::vector< build::generate::Content>& content)
               {
                  common::file::scoped::Path path{ common::file::name::unique( "server_", ".cpp")};

                  std::ofstream out{ path};

                  out << license::c << R"(   
#include <xatmi.h>
#include <xatmi/server.h>

#ifdef __cplusplus
extern "C" {
#endif

)";

                  // declare services
                  for( auto& service : settings.services)
                  {
                     out << "extern void " << service.function << "( TPSVCINFO *context);" << '\n';
                  }

                  out << "\n\n\n";

                  // declarations resources
                  for( auto& cont : content)
                  {
                     out << cont.before_main;
                  }


                  out << R"(
int main( int argc, char** argv)
{

   struct casual_service_name_mapping service_mapping[] = {)";

                  for( auto& service : settings.services)
                  {
                     out << R"(
      {&)" << service.function << R"(, ")" << service.name << R"(", ")" << service.category << R"(", )" << common::cast::underlying( service.transaction) << "},";
                  }

                  out << R"(
      { 0, 0, 0, 0} /* null ending */
   };

)";

                  // declarations
                  for( auto& cont : content)
                  {
                     out << cont.inside_main;
                  };

                  out << R"(

   struct casual_server_arguments arguments = {
         service_mapping,
         &tpsvrinit,
         &tpsvrdone,
         argc,
         argv,
         xa_mapping
   };


   /*
   * Start the server
   */
   return casual_run_server( &arguments);

}


#ifdef __cplusplus
}
#endif

)";

                  // make sure we flush
                  out << std::flush;

                  return path;
               }

               void build( const std::string& c_file, Settings settings)
               {
                  trace::Exit exit( "build server", settings.directive.verbose);

                  common::log::line( log, "c_file: ", c_file);
                  
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


                  build::task( c_file, settings.directive);
               }

               void main( int argc, char **argv)
               {
                  Settings settings;

                  {
                     trace::Exit log( "parse arguments", false);

                     using namespace casual::common::argument;

                     Parse{ "builds a casual xatmi server",
                        Option( std::tie( settings.directive.output), { "-o", "--output"}, "name of server to be built"),
                        Option( [&]( const std::vector< std::string>& values){ settings.set_services( values);}, 
                           {"-s", "--service"}, "service names")( cardinality::any{}),
                        Option( [&]( const std::string& value){ settings.set_server_definition_path( value);}, { "-d", "--server-definition"}, "path to server definition file"),
                        Option( [&]( const std::vector< std::string>& values){ settings.set_resources( values);}, 
                           {"-r", "--resource-keys"}, "key of the resource")( cardinality::any{}),
                        Option( std::tie( settings.directive.compiler), {"-c", "--compiler"}, "compiler to use"),
                        Option( settings.directive.cli_directives(), 
                           {"-f", "--link-directives"}, "additional compile and link directives")( cardinality::any{}),
                        Option( std::tie( settings.properties_file), {"-p", "--properties-file"}, "path to resource properties file"),
                        Option( option::toggle( settings.directive.verbose), {"-v", "--verbose"}, "verbose output"),
                        Option( option::toggle( settings.keep), {"-k", "--keep"}, "keep the intermediate file")
                     }( argc, argv);

                     validate( settings);
                  }

                  if( settings.directive.verbose) std::cout << "";

                  // Generate file

                  auto properties = settings.properties_file.empty() ?
                     configuration::resource::property::get() : configuration::resource::property::get( settings.properties_file);

                  auto resources = build::transform::resources( 
                     std::move( settings.resources), 
                     std::move( properties));

                  settings.directive.add( resources);

                  auto path = generate( settings, { build::generate::resources( resources)});

                  auto name = settings.keep ? path.release() : path.path();

                  build( name, settings);
               }        

            } // <unnamed>
         } // local

      } // build
   } // tools
} // casual

int main( int argc, char **argv)
{
   try
   {
      casual::tools::build::local::main( argc, argv);
   }
   catch( ...)
   {
      return casual::common::exception::handle( std::cerr);
   }

   return 0;
}

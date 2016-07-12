//!
//! casual 
//!

#include "common/arguments.h"
#include "common/buffer/type.h"
#include "common/service/header.h"

#include "sf/buffer.h"
#include "sf/xatmi_call.h"
#include "sf/log.h"


#include <iostream>

namespace casual
{
   using namespace common;

   namespace tools
   {
      namespace service
      {


         namespace local
         {
            namespace
            {

               struct Settings
               {
                  std::string protocol;
                  std::vector< std::string> services;

               };

               namespace buffer
               {
                  common::buffer::Type type( const std::string& format)
                  {
                     static std::map< std::string, common::buffer::Type> types{
                        {"json", common::buffer::type::json()},
                        {"xml", common::buffer::type::xml()},
                        {"yaml", common::buffer::type::yaml()},
                        {"ini", common::buffer::type::ini()},
                     };

                     auto found = range::find( types, format);

                     if( found)
                     {
                        return found->second;
                     }
                     throw exception::invalid::Argument{ "format not supported", CASUAL_NIP( format)};
                  }

               } // buffer


               void call( std::ostream& out, const std::string& service, common::buffer::Type type)
               {
                  Trace trace{ "tools::service::example::local::call"};

                  sf::buffer::Buffer input{ type};
                  //range::copy( "{}", std::begin(input));

                  sf::buffer::Buffer output{ type};

                  sf::xatmi::service::call( service, input, output, 0);

                  out.write( output.data(), output.size());
               }



               void print( std::ostream& out, const Settings& settings)
               {
                  Trace trace{ "tools::service::example::local::print"};

                  //
                  // Set header so we invoke the servcie-example protocol
                  //
                  common::service::header::replace::add( { "casual-service-example", "true"});

                  for( auto& service : settings.services)
                  {
                     call( out, service, buffer::type( settings.protocol));
                  }
               }


            } // <unnamed>
         } // local


         int main( int argc, char **argv)
         {
            local::Settings settings;

            try
            {
               Arguments arguments{ "Describes a casual service",
                  {
                     argument::directive( {"-s", "--services"}, "services to exemplify ", settings.services),
                     argument::directive( {"-f", "--format"}, "format to use [json,yaml,xml,ini] default: json ", settings.protocol)
                  }};

               arguments.parse( argc, argv);
            }
            catch( ...)
            {
               return common::error::handler();
            }

            local::print( std::cout, settings);


            return 0;
         }

      } // service
   } // tools
} // casual


int main( int argc, char **argv)
{
   try
   {
      return casual::tools::service::main( argc, argv);
   }
   catch( ...)
   {
      casual::common::error::handler();
   }
}

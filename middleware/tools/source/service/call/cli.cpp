//!
//! Copyright (c) 2018, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!

#define CASUAL_NO_XATMI_UNDEFINE

#include "tools/service/call/cli.h"
#include "common/argument.h"
#include "common/optional.h"
#include "common/exception/handle.h"


#include "service/manager/admin/api.h"


#include <stdexcept>
#include <iostream>
#include <string>
#include <map>
#include <cstring>

#include "xatmi.h"

namespace casual 
{
   

   namespace local
   {
      namespace
      {
         namespace service
         {
            auto state()
            {
               auto result = casual::service::manager::admin::api::state();

               return common::algorithm::transform( result.services, []( auto& s){
                  return std::move( s.name);
               });
            }
         }



         std::string type_from_input( const std::string& input, const common::optional< std::string>& format)
         {
            if( format)
               return format.value();

            //
            // TODO: More fancy
            //

            if( ! input.empty())
            {
               const std::map<std::string::value_type,std::string> mappings
               {
                  { '[', CASUAL_BUFFER_INI_TYPE},
                  { '{', CASUAL_BUFFER_JSON_TYPE},
                  { '<', CASUAL_BUFFER_XML_TYPE},
                  { '%', CASUAL_BUFFER_YAML_TYPE},
               };

               const auto result = mappings.find( input.at( 0));

               if( result != mappings.end())
               {
                  std::clog << "assuming type " << result->second << std::endl;
                  return result->second;
               }
            }

            throw std::invalid_argument( "failed to deduce type from input");
         }

         std::vector< std::string> types()
         {
            return { 
               CASUAL_BUFFER_BINARY_TYPE, 
               CASUAL_BUFFER_INI_TYPE,
               CASUAL_BUFFER_JSON_TYPE,
               CASUAL_BUFFER_XML_TYPE,
               CASUAL_BUFFER_YAML_TYPE
            };
         }

         void call( const std::string& service, const common::optional< std::string>& format)
         {
            try
            {
               
               //
               // Read data from stdin
               //

               std::string payload;
               while( std::cin.peek() != std::istream::traits_type::eof())
               {
                  payload.push_back( std::cin.get());
               }

               //
               // Check is user provided a type
               //
               auto type = local::type_from_input( payload, format); 

               //
               // Allocate a buffer
               //
               auto buffer = tpalloc( type.c_str(), nullptr, payload.size());

               if( ! buffer)
               {
                  throw std::runtime_error( tperrnostring( tperrno));
               }

               //
               // Copy payload to buffer
               //
               std::memcpy( buffer, payload.data(), payload.size());

               long len = payload.size();

               //
               // Call the service
               //
               const auto result = tpcall( service.c_str(), buffer, len, &buffer, &len, 0);

               const auto error = result == -1 ? tperrno : 0;

               payload.assign( buffer, len);

               tpfree( buffer);

               //
               // Check some result
               //

               if( error)
               {
                  if( error != TPESVCFAIL)
                  {
                     throw std::runtime_error( tperrnostring( error));
                  }
               }

               //
               // Print the result (we don't add a new line)
               //
               std::cout << payload << std::flush;


               if( error)
               {
                  //
                  // Should only be with TPESVCFAIL
                  //
                  throw std::logic_error( tperrnostring( error));
               }
            }
            catch( ...)
            {
               casual::common::exception::handle( std::cerr);
            }
         }

         auto call_complete = []( auto values, bool help) -> std::vector< std::string>
         {
            if( help) 
            {
               return { common::string::compose( "<service> ", local::types())};
            }
            if( values.size() % 2 == 0)
            {
               try 
               {
                  return service::state();
               }
               catch( ...)
               {
                  return { "<value>"};
               }
            }

            return local::types();
         };
      } // <unnamed>
   } // local

   namespace tools 
   {
      namespace service
      {
         namespace call
         {
            struct cli::Implementation
            {
               common::argument::Option options()
               {

                  return common::argument::Option{ &local::call, local::call_complete, { "call"}, R"(generic service call

* service   name of the service to invoke
* [buffer]  optional buffer type how to interpret the payload )" + common::string::compose( local::types()) + R"(

reads from standard in and either treats the paylad as the provided format, or tries to deduce format based on payload
)"};
               }

            };

            cli::cli() = default; 
            cli::~cli() = default; 

            common::argument::Option cli::options() &
            {
               return m_implementation->options();
            }
         } // call
      } // manager  
   } // gateway
} // casual




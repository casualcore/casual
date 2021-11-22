//!
//! Copyright (c) 2019, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!

#include "xatmi.h"

#include "common/algorithm.h"
#include "common/domain.h"
#include "common/exception/guard.h"
#include "common/string.h"
#include "common/argument.h"


#include <locale>

namespace casual
{
   using namespace common;

   namespace example::resource::server
   {

      namespace local
      {
         namespace
         {
            struct
            {
               struct
               {
                  std::vector< std::string> services;
               } nested;
            } global;
            
         } // <unnamed>
      } // local


      extern "C"
      {
         void casual_example_resource_echo( TPSVCINFO* info)
         {
            tpreturn( TPSUCCESS, 0, info->data, info->len, 0);
         }

         void casual_example_resource_domain_name( TPSVCINFO* info)
         {
            auto buffer = ::tpalloc( X_OCTET, nullptr, domain::identity().name.size() + 1);

            algorithm::copy( domain::identity().name, buffer);
            buffer[ domain::identity().name.size()] = '\0';

            tpreturn( TPSUCCESS, 0, buffer, domain::identity().name.size() + 1, 0);
         }

         void casual_example_resource_nested_calls( TPSVCINFO* information)
         {
            auto descriptors = algorithm::transform( local::global.nested.services, [information]( auto& service)
            {
               return ::tpacall( service.data(), information->data, information->len, 0);
            });

            auto buffer = ::tpalloc( "X_OCTET", nullptr, 128);

            for( auto descriptor : descriptors)
            {
               auto size = ::tptypes( buffer, nullptr, nullptr);
               ::tpgetrply( &descriptor, &buffer, &size, 0);
            }

            ::tpreturn( TPSUCCESS, 0, information->data, information->len, 0);
         }

         int tpsvrinit( int argc, char* argv[])
         {
            return exception::main::log::guard( [&]()
            {
               argument::Parse{ "Only? for unittests",
                  argument::Option{ std::tie( local::global.nested.services), {"--nested-calls"}, "service that casual/example/forward should call"}
               }( argc, argv);

               auto advertise_service = []( auto function, std::string_view name)
               {
                  tpadvertise( name.data(), function);
               };

               advertise_service( &casual_example_resource_echo, string::compose( "casual/example/resource/domain/echo/", domain::identity().name));
               advertise_service( &casual_example_resource_nested_calls, string::compose( "casual/example/resource/nested/calls/", domain::identity().name));
            });
         }

      } // C

   } // example::resource::server
} // casual
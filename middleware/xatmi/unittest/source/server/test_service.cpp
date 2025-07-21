//! 
//! Copyright (c) 2015, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!

#define CASUAL_NO_XATMI_UNDEFINE

#include "common/unittest.h"

#include "casual/xatmi/internal/server/service.h"

#include "casual/xatmi.h"

namespace casual
{
   namespace xatmi
   {

      namespace local
      {
         namespace
         {
            void service1( TPSVCINFO *) {}
            void service2( TPSVCINFO *) {}

            void service_foo( TPSVCINFO* info) 
            {
               EXPECT_TRUE( info->name == std::string{ "service_foo"}) << "info->name: " << info->name;
               
               // tpreturn
               tpreturn( TPSUCCESS, 0, nullptr, 0, 0);
            };

            auto parameter( std::string name)
            {
               casual::server::service::invoke::Parameter parameter;
               parameter.service.name = std::move( name);
               parameter.payload.type = common::buffer::type::binary;
               
               return parameter;
            }

         } // <unnamed>
      } // local



      TEST( server_service, equality)
      {
         common::unittest::Trace trace;

         auto s1 = internal::server::service::create( ".1", &local::service1);
         auto s2 = internal::server::service::create( ".2", &local::service1);

         EXPECT_TRUE( s1 == s2);
      }

      TEST( server_service, in_equality)
      {
         common::unittest::Trace trace;

         auto s1 = internal::server::service::create( ".1", &local::service1);
         auto s2 = internal::server::service::create( ".2", &local::service2);

         EXPECT_TRUE( s1 != s2) << trace.compose( CASUAL_NAMED_VALUE( s1), " - ", CASUAL_NAMED_VALUE( s2));
      }

      TEST( server_xatmi_service, tpsvcinfo_name_correctly_copied)
      {
         common::unittest::Trace trace;

         auto service = internal::server::service::create( "service_foo", &local::service_foo);

         service( local::parameter( "service_foo"));

      }


   } // xatmi

} // casual

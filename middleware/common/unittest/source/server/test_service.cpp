//! 
//! Copyright (c) 2015, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!


#include "common/unittest.h"

#include "common/server/service.h"
#include "common/server/context.h"
#include "xatmi.h"

namespace casual
{
   namespace common
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
               server::context().jump_return( flag::xatmi::Return::success, 0, info->data, info->len);
            };

            auto parameter( std::string name)
            {
               service::invoke::Parameter parameter;
               parameter.service.name = std::move( name);
               parameter.payload.type = buffer::type::binary();
               
               return parameter;
            }

         } // <unnamed>
      } // local



      TEST( common_server_service, equality)
      {
         common::unittest::Trace trace;

         auto s1 = server::xatmi::service( ".1", &local::service1);
         auto s2 = server::xatmi::service( ".2", &local::service1);

         EXPECT_TRUE( s1 == s2);
      }

      TEST( common_server_service, in_equality)
      {
         common::unittest::Trace trace;

         auto s1 = server::xatmi::service( ".1", &local::service1);
         auto s2 = server::xatmi::service( ".2", &local::service2);

         EXPECT_TRUE( s1 != s2) << CASUAL_NAMED_VALUE( s1) << " - " << CASUAL_NAMED_VALUE( s2);
      }

      TEST( common_server_xatmi_service, tpsvcinfo_name_correctly_copied)
      {
         common::unittest::Trace trace;

         auto service = server::xatmi::service( "service_foo", &local::service_foo);

         service( local::parameter( "service_foo"));

      }


   } // common

} // casual

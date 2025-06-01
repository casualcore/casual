//! 
//! Copyright (c) 2015, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!


#include "common/unittest.h"

#include "server/argument.h"



namespace casual
{
   namespace server
   {
      namespace local
      {
         namespace
         {
            service::invoke::Result service1( service::invoke::Parameter&&) { return {};}
            service::invoke::Result service3( service::invoke::Parameter&&, int) { return {};}
            service::invoke::Result service4( service::invoke::Parameter&&, std::string& value) { value = "test"; return {};}
         } // <unnamed>
      } // local



      TEST( server_argument, construction)
      {
         common::unittest::Trace trace;

         EXPECT_NO_THROW({
            server::Arguments arguments;
         });
      }

      TEST( server_argument, service_emplace_back)
      {
         common::unittest::Trace trace;

         EXPECT_NO_THROW({
            server::Arguments arguments;

            arguments.services.emplace_back( ".1", &local::service1);

         });
      }


      TEST( server_argument, bind_service_emplace_back)
      {
         common::unittest::Trace trace;

         EXPECT_NO_THROW({
            server::Arguments arguments;

            arguments.services.emplace_back( ".1", std::bind( &local::service3, std::placeholders::_1, 10));
         });
      }

      TEST( server_argument, bind_service_type_trans_emplace_back)
      {
         common::unittest::Trace trace;

         EXPECT_NO_THROW({
            server::Arguments arguments;

            arguments.services.emplace_back( ".1",
               std::bind( &local::service3, std::placeholders::_1, 10),
               service::transaction::Type::none, service::visibility::Type::discoverable, service::category::admin);
         });
      }




      TEST( server_argument, bind_ref_service_type_trans_emplace_back)
      {
         common::unittest::Trace trace;

         std::string value;

         EXPECT_NO_THROW({
            server::Arguments arguments;

            arguments.services.emplace_back( ".1",
               std::bind( &local::service4, std::placeholders::_1, std::ref( value)),
               service::transaction::Type::none, service::visibility::Type::discoverable, service::category::admin);

            arguments.services.back()( service::invoke::Parameter{ .payload = common::buffer::Payload{ ".binary/", 128}});
         });

         EXPECT_TRUE( value == "test");

      }

      namespace local
      {
         namespace
         {
            server::Arguments make_arguments( std::string& value)
            {
               server::Arguments arguments;

               arguments.services = {
                     { ".1",
                           std::bind( &local::service4, std::placeholders::_1, std::ref( value)),
                           service::transaction::Type::none, service::visibility::Type::discoverable, service::category::admin}
               };

               arguments.services.back()( service::invoke::Parameter{});

               return arguments;
            }
         } // <unnamed>
      } // local


      TEST( server_argument, bind_ref_service_type_trans_emplace_back__return_by_value)
      {
         common::unittest::Trace trace;

         std::string value;

         auto arguments = local::make_arguments( value);

         EXPECT_TRUE( value == "test");
      }


   } // server
} // casual

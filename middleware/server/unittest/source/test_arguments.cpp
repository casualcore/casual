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



      namespace local
      {
         namespace
         {
            server::Arguments make_arguments( std::string& value)
            {
               server::Arguments arguments;

               arguments.services = {
                     server::Service{ 
                        .name = ".1",
                        .function = std::bind( &local::service4, std::placeholders::_1, std::ref( value)),
                        .transaction = service::transaction::Type::none,
                        .visibility = service::visibility::Type::discoverable
                     }
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

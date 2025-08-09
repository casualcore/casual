//!
//! Copyright (c) 2025, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!

#include "common/unittest.h"

#include "service/protocol/call.h"
#include "service/unittest/utility.h"

#include "domain/unittest/manager.h"

namespace casual
{
   namespace test
   {

      namespace local
      {
         namespace
         {
            namespace configuration
            {
               constexpr auto base = R"(
domain:
   name: service-domain
   groups:
      -  name: base
      -  name: user
         dependencies: [ base]

   servers:
      -  path: ${CMAKE_BINARY_DIR}/middleware/service/bin/casual-service-manager
         memberships: [ base]

)";

            } // configuration

            template< typename... C>
            auto domain( C&&... configurations)
            {
               return domain::unittest::manager( configuration::base, std::forward< C>( configurations)...);
            }
         } // <unnamed>
      } // local

      TEST( test_service_protocol_call, binary_call)
      {
         common::unittest::Trace trace;

         auto a = local::domain( R"(
domain:
   name: A
   servers:
      -  path: ${CMAKE_BINARY_DIR}/middleware/example/server/bin/casual-example-server
         memberships: [ user]

)");

         const long arg_long = 42;
         const std::string arg_string = "hello world";

         auto result =  service::protocol::binary::Call{}( "casual/example/echo", arg_long, arg_string);

         EXPECT_TRUE( result.extract< long>() == arg_long);
         EXPECT_TRUE( result.extract< std::string>() == arg_string);
      }


      TEST( test_service_protocol_call, binary_send_no_arguments)
      {
         common::unittest::Trace trace;

         auto a = local::domain();

         service::unittest::advertise( { "a"});

         auto receive = service::protocol::binary::Send{}( "a");

         // reply to the send
         {
            auto request = common::communication::ipc::receive< common::message::service::call::callee::Request>();
            service::unittest::send::ack( request);
            auto reply = common::message::reverse::type( request);
            reply.buffer = request.buffer;
            common::communication::device::blocking::send( request.process.ipc, reply);
         }

         EXPECT_NO_THROW(
            std::ignore = receive();
         );
      }

      TEST( test_service_protocol_call, binary_send_with_arguments)
      {
         common::unittest::Trace trace;

         auto a = local::domain();

         service::unittest::advertise( { "a"});

         const long arg_long = 42;
         const std::string arg_string = "hello world";
         auto receive = service::protocol::binary::Send{}( "a", arg_long, arg_string);

         // reply to the send
         {
            auto request = common::communication::ipc::receive< common::message::service::call::callee::Request>();
            service::unittest::send::ack( request);
            auto reply = common::message::reverse::type( request);
            reply.buffer = request.buffer;
            common::communication::device::blocking::send( request.process.ipc, reply);
         }

         auto result = receive();

         EXPECT_TRUE( result.extract< long>() == arg_long);
         EXPECT_TRUE( result.extract< std::string>() == arg_string);
      }

      TEST( test_service_protocol_call, binary_send__with_complement)
      {
         common::unittest::Trace trace;

         auto a = local::domain();

         service::unittest::advertise( { "a"});

         const auto header = casual::header::Fields{ { { "test-header", "casual"}}};

         const auto complement = service::send::Complement{ .header = header};

         const long arg_long = 42;
         const std::string arg_string = "hello world";
         auto receive = service::protocol::binary::Send{}( "a", complement, arg_long, arg_string);

         // reply to the send
         {
            auto request = common::communication::ipc::receive< common::message::service::call::callee::Request>();

            // Check that the header was propagated.
            EXPECT_TRUE( request.header.contains( "test-header"));
            EXPECT_TRUE( request.header.at( "test-header").value() == "casual");

            service::unittest::send::ack( request);
            auto reply = common::message::reverse::type( request);
            reply.buffer = request.buffer;
            reply.header = request.header; // reply the header
            common::communication::device::blocking::send( request.process.ipc, reply);
         }

      
         auto result = receive();

         EXPECT_TRUE( result.header == header);

         EXPECT_TRUE( result.extract< long>() == arg_long);
         EXPECT_TRUE( result.extract< std::string>() == arg_string);
      }
      
   } // test
   
} // casual

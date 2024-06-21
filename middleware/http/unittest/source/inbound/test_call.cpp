//!
//! Copyright (c) 2021, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!

#include "common/unittest.h"

#include "domain/unittest/manager.h"

#include "http/inbound/call.h"

#include "service/unittest/utility.h"

namespace casual
{
   using namespace common;

   namespace http::inbound
   {
      namespace local
      {
         namespace
         {
            namespace configuration
            {
               constexpr auto base = R"(
domain: 
   name: base

   groups: 
      - name: base
      - name: user
        dependencies: [ base]
   
   servers:
      - path: "${CASUAL_MAKE_SOURCE_ROOT}/middleware/service/bin/casual-service-manager"
        memberships: [ base]
      - path: "${CASUAL_MAKE_SOURCE_ROOT}/middleware/transaction/bin/casual-transaction-manager"
        memberships: [ base]
      - path: "${CASUAL_MAKE_SOURCE_ROOT}/middleware/example/server/bin/casual-example-server"
        memberships: [ user]
)";
               

            } // configuration

            auto domain()
            {
               return casual::domain::unittest::manager( configuration::base);
            }


            auto wait( call::Context context) -> decltype( context.receive())
            {
               auto count = 1000;

               while( count-- > 0)
               {
                  if( auto reply = context.receive())
                     return reply;
                  process::sleep( std::chrono::milliseconds{ 1});
               }
               return {};
            }

            namespace header
            {
               auto value( const std::vector< call::header::Field>& header, std::string_view key)
               {
                  if( auto found = algorithm::find( header, key))
                     return found->value();

                  common::code::raise::error( common::code::casual::invalid_argument, "unittest - failed to find key: ", key);
               }
            } // header


         } // <unnamed>
      } // local

      TEST( http_inbound_call, call_echo)
      {
         unittest::Trace trace;

         auto domain = local::domain();

         constexpr std::string_view json = R"(
{
   "a" : 42  
}
)";
         auto request = [&json]()
         {
            call::Request result;
            result.service = "casual/example/echo";
            result.payload.header = { call::header::Field{ "content-type:application/json"}};
            algorithm::copy( view::binary::make( json), result.payload.body);
            return result; 
         };

         auto result = local::wait( call::Context{ call::Directive::service, request()});

         ASSERT_TRUE( result);
         auto& reply = result.value();
         EXPECT_TRUE( reply.code == http::code::ok);
         EXPECT_TRUE( algorithm::equal( reply.payload.body, view::binary::make( json)));
         EXPECT_TRUE( local::header::value( reply.payload.header, "content-type") == "application/json");
         EXPECT_TRUE( local::header::value( reply.payload.header, "content-length") == std::to_string( json.size()));

         EXPECT_TRUE( local::header::value( reply.payload.header, "casual-result-code") == "OK");
         EXPECT_TRUE( local::header::value( reply.payload.header, "casual-result-user-code") == "0");

      }

      TEST( http_inbound_call, call_non_exixting_service)
      {
         unittest::Trace trace;

         auto domain = local::domain();
         const auto service = "non-existent-service";

         constexpr std::string_view json = R"(
{}
)";
         auto request = [&json, &service]()
         {
            call::Request result;
            result.service = service;
            result.payload.header = { call::header::Field{ "content-type:application/json"}};
            algorithm::copy( view::binary::make( json), result.payload.body);
            return result; 
         };

         auto result = local::wait( call::Context{ call::Directive::service, request()});

         ASSERT_TRUE( result);
         auto& reply = result.value();
         EXPECT_TRUE( reply.code == http::code::not_found) << CASUAL_NAMED_VALUE( reply.code);

         auto size = common::string::compose( "failed to lookup service: ", service, ": TPENOENT").size();
         EXPECT_TRUE( local::header::value( reply.payload.header, "content-length") == std::to_string( size));
      }

      TEST( http_inbound_call, call_rollback)
      {
         unittest::Trace trace;

         auto domain = local::domain();

         constexpr std::string_view json = R"(
{
   "a": "b"
}
)";
         auto request = [&json]()
         {
            call::Request result;
            result.service = "casual/example/rollback";
            result.payload.header = { call::header::Field{ "content-type:application/json"}};
            algorithm::copy( view::binary::make( json), result.payload.body);
            return result; 
         };

         auto result = local::wait( call::Context{ call::Directive::service, request()});

         ASSERT_TRUE( result);
         auto& reply = result.value();
         // TODO: is 500 really the best code?
         EXPECT_TRUE( reply.code == http::code::internal_server_error) << reply.code;
         EXPECT_TRUE( local::header::value( reply.payload.header, "casual-result-code") == "TPESVCFAIL");

         EXPECT_TRUE( local::header::value( reply.payload.header, "content-type") == "application/json");
         EXPECT_TRUE( local::header::value( reply.payload.header, "content-length") == std::to_string( json.size()));

      }

      TEST( http_inbound_call, call_service__expect_same_correlation_for_lookup_and_call)
      {
         auto domain = local::domain();

         constexpr std::string_view json = R"(
{
   "a" : 42
}
)";

         constexpr auto service = "steve";
         casual::service::unittest::advertise( { service});

         auto request = [ &]()
         {
            call::Request result;
            result.service = service;
            result.payload.header = { call::header::Field{ "content-type:application/json"}};
            algorithm::copy( view::binary::make( json), result.payload.body);
            return result; 
         };

         // Triggers a lookup and reservation of the service
         auto context = call::Context{ call::Directive::service, request()};

         // Get the correlation for the reservation
         auto state = casual::service::unittest::fetch::until( []( auto& state){ return state.reservations.size() == 2;});    // one reservation is .casual/service/state itself
         auto found = algorithm::find_if( state.reservations, [ &]( auto& reservation){ return reservation.service == service;});
         ASSERT_TRUE( found);
         auto correlation = found->correlation;

         // Advance the call state-machine from lookup to call
         // TODO: can we guarantee that the lookup reply has been received here?
         context.receive();

         // Expect the call to have the same correlation as the reservation
         {
            message::service::call::callee::Request request;
            communication::device::blocking::receive( communication::ipc::inbound::device(), request);

            EXPECT_TRUE( request.correlation == correlation);

            communication::device::blocking::send( request.process.ipc, message::reverse::type( request));
         }

         // Expect the call to complete successfully
         {
            auto result = local::wait( std::move( context));
            ASSERT_TRUE( result);

            auto& reply = result.value();
            EXPECT_TRUE( reply.code == http::code::ok);
            EXPECT_TRUE( local::header::value( reply.payload.header, "casual-result-code") == "OK");
            EXPECT_TRUE( local::header::value( reply.payload.header, "casual-result-user-code") == "0");
         }
      }

   } // http::inbound
   
} // casual

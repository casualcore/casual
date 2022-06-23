//!
//! Copyright (c) 2021, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!

#include "common/unittest.h"

#include "domain/unittest/manager.h"

#include "service/unittest/utility.h"

#include "gateway/unittest/utility.h"

#include "common/communication/instance.h"



namespace casual
{
   using namespace common;

   namespace test::domain::service
   {
      namespace local
      {
         namespace
         {


            namespace configuration
            {
               constexpr auto base =  R"(
domain: 
   groups: 
      -  name: base
      -  name: user
         dependencies: [ base]
   
   servers:
      -  path: "${CASUAL_MAKE_SOURCE_ROOT}/middleware/service/bin/casual-service-manager"
         memberships: [ base]
      -  path: "${CASUAL_MAKE_SOURCE_ROOT}/middleware/transaction/bin/casual-transaction-manager"
         memberships: [ base]
      -  path: "${CASUAL_MAKE_SOURCE_ROOT}/middleware/gateway/bin/casual-gateway-manager"
         memberships: [ base]
   
)";
            } // configuration

            namespace is
            {
               auto service( std::string_view name)
               {
                  return [name]( auto& service)
                  {
                     return service.name == name;
                  };
               }
                        
            } // is   

         } // <unnamed>
      } // local

      TEST( test_service, two_server_alias__service_restriction)
      {
         common::unittest::Trace trace;

         auto domain = casual::domain::unittest::manager( local::configuration::base, R"(
domain: 
   name: A
   servers:         
      -  alias: A
         path: "${CASUAL_MAKE_SOURCE_ROOT}/middleware/example/server/bin/casual-example-server"
         memberships: [ user]
         restrictions:
            -  casual/example/echo

      -  alias: B
         path: "${CASUAL_MAKE_SOURCE_ROOT}/middleware/example/server/bin/casual-example-server"
         memberships: [ user]
         restrictions:
            -  casual/example/sink

)");

         auto state = casual::service::unittest::state();

         // check that we excluded services that doesn't match the restrictions
         EXPECT_TRUE( ! algorithm::find_if( state.services, local::is::service( "casual/example/uppercase")));


         {
            auto service = algorithm::find_if( state.services, local::is::service( "casual/example/echo"));
            ASSERT_TRUE( service);
            EXPECT_TRUE( service->instances.sequential.size() == 1) << CASUAL_NAMED_VALUE( service->instances.sequential);
            EXPECT_TRUE( service->instances.concurrent.empty());
         }

         {
            auto service = algorithm::find_if( state.services, local::is::service( "casual/example/sink"));
            ASSERT_TRUE( service);
            EXPECT_TRUE( service->instances.sequential.size() == 1) << CASUAL_NAMED_VALUE( service->instances.sequential);
            EXPECT_TRUE( service->instances.concurrent.empty());
         }
      }

      TEST( test_service, service_restriction_regex)
      {
         common::unittest::Trace trace;

         auto domain = casual::domain::unittest::manager( local::configuration::base, R"(
domain: 
   name: A
   servers:         
      -  alias: A
         path: "${CASUAL_MAKE_SOURCE_ROOT}/middleware/example/server/bin/casual-example-server"
         memberships: [ user]
         restrictions:
            -  ".*/echo$"
            -  ".*/sink$"

)");

         auto state = casual::service::unittest::state();

         // check that we excluded services that doesn't match the restrictions
         EXPECT_TRUE( ! algorithm::find_if( state.services, local::is::service( "casual/example/uppercase")));


         {
            auto service = algorithm::find_if( state.services, local::is::service( "casual/example/echo"));
            ASSERT_TRUE( service);
            EXPECT_TRUE( service->instances.sequential.size() == 1) << CASUAL_NAMED_VALUE( service->instances.sequential);
            EXPECT_TRUE( service->instances.concurrent.empty());
         }

         {
            auto service = algorithm::find_if( state.services, local::is::service( "casual/example/sink"));
            ASSERT_TRUE( service);
            EXPECT_TRUE( service->instances.sequential.size() == 1) << CASUAL_NAMED_VALUE( service->instances.sequential);
            EXPECT_TRUE( service->instances.concurrent.empty());
         }
      }

      namespace local
      {
         namespace
         {
            auto lookup = []( auto service, auto context)
            {
               auto& manager = communication::instance::outbound::service::manager::device();
               casual::message::service::lookup::Request request( process::handle());
               request.context = context;
               request.requested = service;

               auto reply = communication::ipc::call( manager, request);

               // discard the lookup
               {
                  common::message::service::lookup::discard::Request message{ process::handle()};
                  message.correlation = reply.correlation;
                  message.reply = false;
                  communication::device::blocking::send( manager, message);
               }

               return reply;
            };
            
         } // <unnamed>
      } // local

      TEST( test_service, lookup_restriction_depending_on_where_the_lookup_comes_from)
      {
         common::unittest::Trace trace;


         auto b = casual::domain::unittest::manager( local::configuration::base, R"(
domain: 
   name: B
   servers:         
      -  path: "${CASUAL_MAKE_SOURCE_ROOT}/middleware/example/server/bin/casual-example-server"
         memberships: [ user]
   services:
      -  name: casual/example/echo
         routes: [ b]
   gateway:
      inbound:
         groups:
            -  connections:
                  -  address: 127.0.0.1:7010
)");

         auto a = casual::domain::unittest::manager( local::configuration::base, R"(
domain: 
   name: A
   servers:         
      -  path: "${CASUAL_MAKE_SOURCE_ROOT}/middleware/example/server/bin/casual-example-server"
         memberships: [ user]
   services:
      -  name: casual/example/echo
         routes: [ a]
   gateway:
      outbound:
         groups:
            -  connections:
                  -  address: 127.0.0.1:7010
)");
         auto state = gateway::unittest::fetch::until( gateway::unittest::fetch::predicate::outbound::connected( 1));

         // internal
         {
            casual::message::service::lookup::request::Context context;
            context.requester = decltype( context.requester)::internal;
            auto reply = local::lookup( "b", context);
            EXPECT_TRUE( reply.state == decltype( reply.state)::idle);
         }

         // external
         {
            casual::message::service::lookup::request::Context context;
            context.requester = decltype( context.requester)::external;
            // remote service - absent
            auto reply = local::lookup( "b", context);
            EXPECT_TRUE( reply.state == decltype( reply.state)::absent);
            // local service
            reply = local::lookup( "a", context);
            EXPECT_TRUE( reply.state == decltype( reply.state)::idle);
         }

         // external with discover forward
         {
            casual::message::service::lookup::request::Context context;
            context.requester = decltype( context.requester)::external_discovery;

            {
               // local service
               auto reply = local::lookup( "a", context);
               EXPECT_TRUE( reply.state == decltype( reply.state)::idle);
            }
            {
               // remote service - idle (found)
               auto reply = local::lookup( "b", context);
               EXPECT_TRUE( reply.state == decltype( reply.state)::idle);
            }
         }

      }

   } // test::domain::service

} // casual
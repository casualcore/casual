//!
//! Copyright (c) 2022, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!
#include "common/unittest.h"

#include "queue/common/ipc/message.h"

#include "domain/unittest/manager.h"

#include "gateway/unittest/utility.h"

#include "common/communication/instance.h"

namespace casual
{
   using namespace common;

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
   groups: 
      -  name: base
      -  name: user
         dependencies: [ base]
      -  name: end
         dependencies: [ user]
   
   servers:
      -  path: "${CASUAL_MAKE_SOURCE_ROOT}/middleware/service/bin/casual-service-manager"
         memberships: [ base]
      -  path: "${CASUAL_MAKE_SOURCE_ROOT}/middleware/transaction/bin/casual-transaction-manager"
         memberships: [ base]
      -  path: "${CASUAL_MAKE_SOURCE_ROOT}/middleware/queue/bin/casual-queue-manager"
         memberships: [ base]
      -  path: "${CASUAL_MAKE_SOURCE_ROOT}/middleware/gateway/bin/casual-gateway-manager"
         memberships: [ end]
   
)";
   
            } // configuration

            template< typename... Cs>
            auto domain( Cs&&... configurations)
            {
               return casual::domain::unittest::manager( local::configuration::base, std::forward< Cs>( configurations)...);
            }

            template< typename C>
            auto lookup( std::string name, C context)
            {
               casual::queue::ipc::message::lookup::Request request{ process::handle()};
               request.context = std::move( context);
               request.name = std::move( name);
               return communication::ipc::call( communication::instance::outbound::queue::manager::device(), request);
            }

         } // <unnamed>
      } // local

      TEST( test_queue, lookup_restriction_depending_on_where_the_lookup_comes_from)
      {
         common::unittest::Trace trace;


         auto b = local::domain( R"(
domain: 
   name: B
   queue:
      groups:
         -  alias: B
            queuebase: ':memory:'
            queues:
               -  name: b1
               -  name: b2
   gateway:
      inbound:
         groups:
            -  connections:
                  -  address: 127.0.0.1:7010
)");

         auto a = local::domain( R"(
domain: 
   name: A
   queue:
      groups:
         -  alias: A
            queuebase: ':memory:'
            queues:
               -  name: a1
               -  name: a2
   gateway:
      outbound:
         groups:
            -  connections:
                  -  address: 127.0.0.1:7010
)");
         auto state = gateway::unittest::fetch::until( gateway::unittest::fetch::predicate::outbound::connected( 1));

         // internal
         {
            casual::queue::ipc::message::lookup::request::Context context;
            context.requester = decltype( context.requester)::internal;
            auto reply = local::lookup( "b1", context);
            EXPECT_TRUE( reply);
         }

         // external
         {
            casual::queue::ipc::message::lookup::request::Context context;
            context.requester = decltype( context.requester)::external;
            // remote service - absent
            EXPECT_TRUE( ! local::lookup( "b1", context));
            // local service
            EXPECT_TRUE( local::lookup( "a1", context)); 
         }

         // external-discovery
         {
            casual::queue::ipc::message::lookup::request::Context context;
            context.requester = decltype( context.requester)::external_discovery;
            // remote service - already discovered 
            EXPECT_TRUE( local::lookup( "b1", context));
            // remote service - not discovered
            EXPECT_TRUE( ! local::lookup( "b2", context));
            // local service
            EXPECT_TRUE( local::lookup( "a1", context));
         }

      }
      
   } // test
} // casual
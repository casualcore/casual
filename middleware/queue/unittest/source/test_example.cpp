//!
//! Copyright (c) 2024, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!

#include "common/unittest.h"

#include "casual/xatmi.h"
#include "domain/unittest/manager.h"
#include "queue/api/queue.h"
#include "common/buffer/type.h"

namespace casual
{
   using namespace common;

   namespace queue::example
   {
      namespace local
      {
         namespace
         {
            namespace configuration
            {
               constexpr auto base = R"(
system:
   resources:
      -  key: rm-mockup
         server: "${CASUAL_MAKE_SOURCE_ROOT}/middleware/transaction/bin/rm-proxy-casual-mockup"
         xa_struct_name: casual_mockup_xa_switch_static
         libraries:
            -  casual-mockup-rm

domain:
   groups: 
      -  name: base
      -  name: queue
         dependencies: [ base]
      -  name: user
         dependencies: [ queue]
      -  name: gateway
         dependencies: [ user]
   
   queue:
      groups:
         - alias: example
           queuebase: ":memory:"
           queues:
            - name: example.q1
            - name: example.q2
            - name: example.q3

   servers:
      - path: "${CASUAL_MAKE_SOURCE_ROOT}/middleware/service/bin/casual-service-manager"
        memberships: [ base]
      - path: "${CASUAL_MAKE_SOURCE_ROOT}/middleware/transaction/bin/casual-transaction-manager"
        memberships: [ base]
      - path: "${CASUAL_MAKE_SOURCE_ROOT}/middleware/gateway/bin/casual-gateway-manager"
        memberships: [ gateway]
      - path: "${CASUAL_MAKE_SOURCE_ROOT}/middleware/queue/bin/casual-queue-manager"
        memberships: [ queue]
)";
            } // configuration

            template< typename... C>
            auto domain( C&&... configurations)
            {
               return casual::domain::unittest::manager( configuration::base, std::forward< C>( configurations)...);
            }

            template< typename B>
            int call_enqueue( B&& buffer, const std::string& queuename)
            {
               auto len = tptypes( buffer, nullptr, nullptr);
               return tpcall( common::string::compose("casual/example/enqueue/", queuename).data(), buffer, len, &buffer, &len, 0);
            }

         } // <unnamed> 

      } // local

      TEST( casual_queue_example_server, enqueue_non_existing_queue__expect_failure)
      {
         common::unittest::Trace trace;
         
         constexpr auto example_server = R"(
domain:
   servers:
      - path: "${CASUAL_MAKE_SOURCE_ROOT}/middleware/queue/bin/casual-queue-example-server"
        memberships: [ user]
        arguments: [ --queues, non-existing]
)";
         auto domain = local::domain( example_server);

         auto contents = common::unittest::random::binary( 128);
         auto buffer = tpalloc( X_OCTET, nullptr, contents.size());
         common::algorithm::copy( contents, common::binary::span::make( buffer, contents.size()));

         EXPECT_EQ( local::call_enqueue( buffer, "non-existing"), -1);
         tpfree( buffer);
      }

      TEST( casual_queue_example_server, enqueue_1_message__expect_1_message_on_queue)
      {
         common::unittest::Trace trace;
         
         constexpr auto example_server = R"(
domain:
   servers:
      - path: "${CASUAL_MAKE_SOURCE_ROOT}/middleware/queue/bin/casual-queue-example-server"
        memberships: [ user]
        arguments: [ --queues, example.q1]
)";
         auto domain = local::domain( example_server);

         auto contents = common::unittest::random::binary( 128);

         // enqueue 1 message
         {
            auto buffer = tpalloc( X_OCTET, nullptr, contents.size());
            common::algorithm::copy( contents, common::binary::span::make( buffer, contents.size()));

            EXPECT_EQ( local::call_enqueue( buffer, "example.q1"), 0);
            tpfree( buffer);
         }

         // dequeue 1 message
         {
            auto messages = queue::dequeue( "example.q1");
            EXPECT_EQ( messages.size(), 1UL);

            EXPECT_EQ( messages.front().payload.type, "X_OCTET/");
            EXPECT_TRUE( common::algorithm::equal(messages.front().payload.data, contents));
         }
      }

      TEST( casual_queue_example_server, enqueue_1_message_then_abort__expect_empty_queue)
      {
         common::unittest::Trace trace;
         
         constexpr auto example_server = R"(
domain:
   servers:
      - path: "${CASUAL_MAKE_SOURCE_ROOT}/middleware/queue/bin/casual-queue-example-server"
        memberships: [ user]
        arguments: [ --queues, example.q1]
)";
         auto domain = local::domain( example_server);

         auto contents = common::unittest::random::binary( 128);

         ::tx_begin();

         // enqueue 1 message
         {
            auto buffer = tpalloc( X_OCTET, nullptr, contents.size());
            common::algorithm::copy( contents, common::binary::span::make( buffer, contents.size()));

            EXPECT_EQ( local::call_enqueue( buffer, "example.q1"), 0);
            tpfree( buffer);
         }

         EXPECT_EQ( queue::peek::information("example.q1").size(), 1UL);

         ::tx_rollback();

         EXPECT_EQ( queue::dequeue( "example.q1").size(), 0UL);
      }

      TEST( casual_queue_example_server, dequeue_non_existing_queue__expect_failure)
      {
         common::unittest::Trace trace;
         
         constexpr auto example_server = R"(
domain:
   servers:
      - path: "${CASUAL_MAKE_SOURCE_ROOT}/middleware/queue/bin/casual-queue-example-server"
        memberships: [ user]
        arguments: [ --queues, non-existing]
)";
         auto domain = local::domain( example_server);

         auto buffer = tpalloc( X_OCTET, nullptr, 128);
         long olen = 999;

         EXPECT_EQ( tpcall( "casual/example/dequeue/non-existing", nullptr, 0, &buffer, &olen, 0), -1);
      }

      TEST( casual_queue_example_server, dequeue_empty_queue__expect_empty_response)
      {
         common::unittest::Trace trace;
         
         constexpr auto example_server = R"(
domain:
   servers:
      - path: "${CASUAL_MAKE_SOURCE_ROOT}/middleware/queue/bin/casual-queue-example-server"
        memberships: [ user]
        arguments: [ --queues, example.q1]
)";
         auto domain = local::domain( example_server);

         auto buffer = tpalloc( X_OCTET, nullptr, 128);
         long olen = 999;

         EXPECT_EQ( tpcall( "casual/example/dequeue/example.q1", nullptr, 0, &buffer, &olen, 0), 0);
         EXPECT_EQ( olen, 0);
      }

      TEST( casual_queue_example_server, dequeue_1_message)
      {
         common::unittest::Trace trace;
         
         constexpr auto example_server = R"(
domain:
   servers:
      - path: "${CASUAL_MAKE_SOURCE_ROOT}/middleware/queue/bin/casual-queue-example-server"
        memberships: [ user]
        arguments: [ --queues, example.q1]
)";
         auto domain = local::domain( example_server);

         auto contents = common::unittest::random::binary( 128);

         // enqueue 1 message
         {
            queue::Message message;
            message.payload.type = "X_OCTET/";
            common::algorithm::copy( contents, message.payload.data);
            queue::enqueue( "example.q1", message);
         }

         // dequeue 1 message
         {
            auto buffer = tpalloc( X_OCTET, nullptr, 256);
            long olen = 0;

            EXPECT_EQ( tpcall( "casual/example/dequeue/example.q1", nullptr, 0, &buffer, &olen, 0), 0);
            EXPECT_EQ( olen, 128);

            char buf_type[9];
            char buf_subtype[17];

            tptypes( buffer, buf_type, buf_subtype);
            EXPECT_STREQ( buf_type, X_OCTET);
            EXPECT_TRUE( common::algorithm::equal( common::binary::span::make( buffer, olen), contents));
         }

         EXPECT_EQ( queue::dequeue( "example.q1").size(), 0UL);

      }

      TEST( casual_queue_example_server, enqueue_multiple_queues)
      {
         common::unittest::Trace trace;
         
         constexpr auto example_server = R"(
domain:
   servers:
      - path: "${CASUAL_MAKE_SOURCE_ROOT}/middleware/queue/bin/casual-queue-example-server"
        memberships: [ user]
        arguments: [ --queues, example.q1, example.q2, example.q3]
)";
         auto domain = local::domain( example_server);

         auto contents = common::unittest::random::binary( 128);

         // enqueue 1 message
         {
            auto buffer = tpalloc( X_OCTET, nullptr, contents.size());
            common::algorithm::copy( contents, common::binary::span::make( buffer, contents.size()));

            EXPECT_EQ( local::call_enqueue( buffer, "example.q1"), 0);
            tpfree( buffer);
         }
         // dequeue 1 message
         {
            auto messages = queue::dequeue( "example.q1");
            EXPECT_EQ( messages.size(), 1UL);

            EXPECT_EQ( messages.front().payload.type, "X_OCTET/");
            EXPECT_TRUE( common::algorithm::equal(messages.front().payload.data, contents));
         }

         // enqueue 1 message
         {
            auto buffer = tpalloc( X_OCTET, nullptr, contents.size());
            common::algorithm::copy( contents, common::binary::span::make( buffer, contents.size()));

            EXPECT_EQ( local::call_enqueue( buffer, "example.q2"), 0);
            tpfree( buffer);
         }
         // dequeue 1 message
         {
            auto messages = queue::dequeue( "example.q2");
            EXPECT_EQ( messages.size(), 1UL);

            EXPECT_EQ( messages.front().payload.type, "X_OCTET/");
            EXPECT_TRUE( common::algorithm::equal(messages.front().payload.data, contents));
         }
      }

   } // queue::example

} // casual
//!
//! Copyright (c) 2022, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!
#include "common/unittest.h"

#include "queue/api/queue.h"
#include "queue/common/ipc/message.h"
#include "queue/unittest/utility.h"

#include "domain/unittest/manager.h"

#include "gateway/unittest/utility.h"

#include "common/communication/instance.h"
#include "common/transaction/context.h"
#include "common/execute.h"
#include "common/sink.h"

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
      -  name: queue
         dependencies: [ base]
      -  name: user
         dependencies: [ queue]
      -  name: end
         dependencies: [ user]
   
   servers:
      -  path: "${CASUAL_MAKE_SOURCE_ROOT}/middleware/service/bin/casual-service-manager"
         memberships: [ base]
      -  path: "${CASUAL_MAKE_SOURCE_ROOT}/middleware/transaction/bin/casual-transaction-manager"
         memberships: [ base]
      -  path: "${CASUAL_MAKE_SOURCE_ROOT}/middleware/queue/bin/casual-queue-manager"
         memberships: [ queue]
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


            auto message()
            {
               queue::Message result{ queue::Payload{ "payload", unittest::random::binary( 1024) }};
               return result;
            }

            namespace dequeue
            {
               auto until( const std::string& queue)
               {
                  return unittest::fetch::until( [ queue]()
                  { 
                     return queue::dequeue( queue);
                  })
                  ( []( auto&& message)
                  {
                     return ! message.empty();
                  });
               }
            } // dequeue

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

      TEST( test_queue, forward_a1_to_b1___shutdown_B__enqueue_a1__expect_forward_fail_to_a1_error___boot_B__enqueue_a1_expect_forward_success_to_b1)
      {
         common::unittest::Trace trace;

         // sink child signals 
         common::signal::callback::registration< common::code::signal::child>( [](){});


         auto boot_domain_b = []()
         {
            return local::domain( R"(
domain: 
   name: B
   queue:
      groups:
         -  alias: B
            queuebase: ':memory:'
            queues:
               -  name: b1
   gateway:
      inbound:
         groups:
            -  connections:
                  -  address: 127.0.0.1:7010
)");

         };

         auto b = boot_domain_b();

         auto a = local::domain( R"(
domain: 
   name: A
   queue:
      groups:
         -  alias: A
            queuebase: ':memory:'
            queues:
               -  name: a1
      forward:
         groups:
            -  alias: FA
               queues:
               -  source: a1
                  instances: 2
                  target: 
                     queue: b1
   gateway:
      outbound:
         groups:
            -  connections:
                  -  address: 127.0.0.1:7010
)");

         gateway::unittest::fetch::until( gateway::unittest::fetch::predicate::outbound::connected());

         const auto origin = local::message();
         
         // make sure all parts knows about b1
         queue::enqueue( "b1", origin);
         EXPECT_TRUE( ! queue::dequeue( "b1").empty());
         
         // shutdown b
         common::sink( std::move( b));

         // wait until we've lost the connection
         gateway::unittest::fetch::until( gateway::unittest::fetch::predicate::outbound::connected( 0));

         {            
            // should fail, of course...
            EXPECT_ANY_THROW( queue::enqueue( "b1", origin));
         }

         // Add a few for the forward, they will block
         algorithm::for_n< 5>( [ &origin]()
         {
            queue::enqueue( "a1", origin);
         });

         // boot b again...
         b = boot_domain_b();
         a.activate();
         gateway::unittest::fetch::until( gateway::unittest::fetch::predicate::outbound::connected( 1));


         // the forward should have "self heal" and work. We add a few more
         algorithm::for_n< 5>( [ &origin]()
         {
            queue::enqueue( "a1", origin);
         });

         // we should get the 5 from before the boot, and the 5 from after.
         algorithm::for_n< 10>( [ &origin]()
         {
            auto message = local::dequeue::until( "b1");

            ASSERT_TRUE( ! message.empty());
            EXPECT_TRUE( message.front().payload.data == origin.payload.data);
         });
      }


      TEST( test_queue, enable_enqueue_dequeue)
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
               -  name: a
                  enable:
                     enqueue: false
               -  name: b
                  enable:
                     dequeue: false
               -  name: c
                  enable:
                     enqueue: false
                     dequeue: false
               -  name: d

                  
   gateway:
      inbound:
         groups:
            -  connections:
                  -  address: 127.0.0.1:7010
)");

         auto a = local::domain( R"(
domain: 
   name: A
   gateway:
      outbound:
         groups:
            -  connections:
                  -  address: 127.0.0.1:7010
)");
         gateway::unittest::fetch::until( gateway::unittest::fetch::predicate::outbound::connected());

         auto enqueue = []( std::string name) -> std::error_code
         {
            try
            {
               queue::Message message;
               queue::enqueue( name, message);
               return common::code::queue::ok;
            }
            catch( const std::system_error& error)
            {
               return error.code();
            }
         };

         auto dequeue = []( std::string name) -> std::error_code
         {
            try
            {
               auto message = queue::dequeue( name);
               if( ! message.empty())
                  return common::code::queue::ok;
               else 
                  return common::code::queue::no_message;
            }
            catch( const std::system_error& error)
            {
               return error.code();
            }
         };


         EXPECT_TRUE( enqueue( "a") == common::code::queue::no_queue);
         EXPECT_TRUE( enqueue( "b") == common::code::queue::ok);
         EXPECT_TRUE( enqueue( "c") == common::code::queue::no_queue);
         EXPECT_TRUE( enqueue( "d") == common::code::queue::ok);

         EXPECT_TRUE( dequeue( "a") == common::code::queue::no_message);
         EXPECT_TRUE( dequeue( "b") == common::code::queue::no_queue);
         EXPECT_TRUE( dequeue( "c") == common::code::queue::no_queue);
         // we've enqueued to d before
         EXPECT_TRUE( dequeue( "d") == common::code::queue::ok);

      }
      
   } // test
} // casual
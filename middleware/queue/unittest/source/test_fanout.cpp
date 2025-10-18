//!
//! Copyright (c) 2025, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!

#include "common/unittest.h"

#include "queue/unittest/utility.h"
#include "queue/api/queue.h"

#include "domain/unittest/manager.h"
#include "domain/unittest/configuration.h"

#include "configuration/unittest/utility.h"
#include "configuration/model/transform.h"

#include "common/buffer/type.h"

namespace casual
{
   namespace queue
   {

      namespace local
      {
         namespace
         {
            namespace configuration
            {
               static constexpr auto servers = R"(
domain:
   name: A
   groups: 
      - name: base
      - name: queue
        dependencies: [ base]

   servers:
      - path: ${CASUAL_MAKE_SOURCE_ROOT}/middleware/service/bin/casual-service-manager
        memberships: [ base]
      - path: ${CASUAL_MAKE_SOURCE_ROOT}/middleware/transaction/bin/casual-transaction-manager
        memberships: [ base]
      - path: bin/casual-queue-manager
        memberships: [ queue]
)";

            } // configuration



            template< typename... C>
            auto domain( C&&... configurations)
            {
               return casual::domain::unittest::manager( configuration::servers, std::forward< C>( configurations)...);
            }

            auto message()
            {
               queue::Message message;

               message.payload.type = common::buffer::type::binary;
               message.payload.data = common::unittest::random::binary( 256);

               // use us precision for the test to be less fragile. queue attributes uses us.
               message.attributes.available = std::chrono::time_point_cast< std::chrono::microseconds>( common::chronology::time_point::clock::now());
               message.attributes.properties = "property-value";
               message.attributes.reply = "reply-queue";

               return message;
            }
            
         } // <unnamed>
      } // local


      TEST( queue_fanout, startup)
      {
         common::unittest::Trace trace;

         constexpr auto configuration = R"(
domain: 
   name: A
   queue:
      groups:
         - alias: group-a
           queuebase: ":memory:"
           queues:
            -  name: a1
         - alias: group-b
           queuebase: ":memory:"
           queues:
            -  name: b1
            -  name: b2
            -  name: b3

      fanout:
         groups:
            -  alias: fanout-group-a
               queues:
                  -  alias: fo1
                     source: a1
                     targets:
                        -  queue: b1
                        -  queue: b2
                        -  queue: b3
      )";

        
         auto domain = local::domain( configuration);

         auto state = unittest::fetch::until( unittest::fetch::predicate::fanout_groups( 1));

         {
            auto& group = state.fanout.groups.at( 0);
            auto& queue = state.fanout.queues.at( 0);

            EXPECT_TRUE( group.alias == "fanout-group-a") << CASUAL_NAMED_VALUE( state.fanout);
            EXPECT_TRUE( queue.alias == "fo1") << CASUAL_NAMED_VALUE( state.fanout);
            EXPECT_TRUE( queue.group == group.process.pid) << CASUAL_NAMED_VALUE( state.fanout);
            EXPECT_TRUE( queue.source == "a1") << CASUAL_NAMED_VALUE( state.fanout);
            EXPECT_TRUE( queue.targets.size() == 3) << CASUAL_NAMED_VALUE( state.fanout);
            EXPECT_TRUE( queue.targets.at( 0).queue == "b1") << CASUAL_NAMED_VALUE( state.fanout);
            EXPECT_TRUE( queue.targets.at( 1).queue == "b2") << CASUAL_NAMED_VALUE( state.fanout);
            EXPECT_TRUE( queue.targets.at( 2).queue == "b3") << CASUAL_NAMED_VALUE( state.fanout);
         }
      }

      TEST( queue_fanout, enqueue_to_source__expect__fanout_to_targets)
      {
         common::unittest::Trace trace;

         constexpr auto configuration = R"(
domain: 
   name: A
   queue:
      groups:
         - alias: group-a
           queuebase: ":memory:"
           queues:
            -  name: a1
         - alias: group-b
           queuebase: ":memory:"
           queues:
            -  name: b1
            -  name: b2
            -  name: b3

      fanout:
         groups:
            -  alias: fanout-group-a
               queues:
                  -  alias: fo1
                     source: a1
                     targets:
                        -  queue: b1
                        -  queue: b2
                        -  queue: b3
      )";

        
         auto domain = local::domain( configuration);

         auto state = unittest::fetch::until( unittest::fetch::predicate::fanout_groups( 1));

         const auto origin = local::message();

         // enqueue to source
         queue::enqueue( "a1", origin);

         // fetch from targets
         for( auto target_queue : { "b1", "b2", "b3"})
         {
            auto message = queue::blocking::dequeue( target_queue);
            EXPECT_TRUE( origin.payload.data == message.payload.data);
            EXPECT_TRUE( origin.payload.type == message.payload.type);
            EXPECT_TRUE( origin.attributes.properties == message.attributes.properties);
            EXPECT_TRUE( origin.attributes.reply == message.attributes.reply);
            EXPECT_TRUE( origin.attributes.available == message.attributes.available);
         }
      }

      TEST( queue_fanout, disabled_fanout_queue)
      {
         common::unittest::Trace trace;

         auto domain = local::domain( R"(
domain: 
   name: A
   groups: 
      -  name: a
         dependencies: [ queue]
      -  name: b
         enabled: false
         dependencies: [ queue]
   queue:
      fanout:
         groups:
            -  alias: fanout-group-a
               queues:
                  -  alias: fo1
                     source: a1
                     memberships: [ a]
                     targets:
                        -  queue: b1
                        -  queue: b2
                        -  queue: b3
                  -  alias: fo2
                     source: a2
                     memberships: [ b]
                     instances: 2
                     targets:
                        -  queue: b4
                        -  queue: b5
         )");         

         {
            auto state = unittest::fetch::until( unittest::fetch::predicate::fanout_groups( 1));

            auto fo1 = common::algorithm::find( state.fanout.queues, "fo1");
            ASSERT_TRUE( fo1) << CASUAL_NAMED_VALUE( state.fanout);
            EXPECT_TRUE( fo1->enabled) << CASUAL_NAMED_VALUE( *fo1);
            EXPECT_TRUE( fo1->instances.configured == 1) << CASUAL_NAMED_VALUE( *fo1);
            EXPECT_TRUE( fo1->instances.running == 1) << CASUAL_NAMED_VALUE( *fo1);

            auto fo2 = common::algorithm::find( state.fanout.queues, "fo2");
            ASSERT_TRUE( fo2) << CASUAL_NAMED_VALUE( state.fanout);
            EXPECT_TRUE( ! fo2->enabled) << CASUAL_NAMED_VALUE( *fo2);
            EXPECT_TRUE( fo2->instances.configured == 2) << CASUAL_NAMED_VALUE( *fo2);
            EXPECT_TRUE( fo2->instances.running == 0) << CASUAL_NAMED_VALUE( *fo2);
         }

         // enable group b
         {
            constexpr auto wanted = R"(
domain: 
   name: A
   groups: 
      -  name: b
         enabled: true
         dependencies: [ queue]
)";

            casual::domain::unittest::configuration::put( 
               configuration::model::transform( casual::configuration::unittest::load( wanted)));

            static auto is_enabled = []( auto& queue){ return queue.enabled;};
            
            // all fanout queues should be enabled now
            unittest::fetch::until( []( auto& state){ return std::ranges::all_of( state.fanout.queues, is_enabled);});
         }
      }

      TEST( queue_fanout, target_delay__enqueue_to_source__expect__fanout_to_targets_with_delay)
      {
         common::unittest::Trace trace;

         constexpr auto configuration = R"(
domain: 
   name: A
   queue:
      groups:
         - alias: group-a
           queuebase: ":memory:"
           queues:
            -  name: a1
         - alias: group-b
           queuebase: ":memory:"
           queues:
            -  name: b1
            -  name: b2
            -  name: b3

      fanout:
         groups:
            -  alias: fanout-group-a
               queues:
                  -  alias: fo1
                     source: a1
                     targets:
                        -  queue: b1
                           delay: 1ms
                        -  queue: b2
                           delay: 2ms
                        -  queue: b3
                           delay: 3ms
      )";

        
         auto domain = local::domain( configuration);

         auto state = unittest::fetch::until( unittest::fetch::predicate::fanout_groups( 1));

         const auto payload = common::unittest::random::binary( 1024);

         const auto origin = local::message();

         auto start = common::chronology::time_point::clock::now();

         // enqueue to source
         queue::enqueue( "a1", origin);

         auto validate_message = [&]( const queue::Message& message)
         {
            EXPECT_TRUE( origin.payload.data == message.payload.data);
            EXPECT_TRUE( origin.payload.type == message.payload.type);
            EXPECT_TRUE( origin.attributes.properties == message.attributes.properties);
            EXPECT_TRUE( origin.attributes.reply == message.attributes.reply);
         };

         {
            auto message = queue::blocking::dequeue( "b1");
            validate_message( message);
            EXPECT_TRUE( message.attributes.available >= start + std::chrono::milliseconds( 1));
         }
         {
            auto message = queue::blocking::dequeue( "b2");
            validate_message( message);
            EXPECT_TRUE( message.attributes.available >= start + std::chrono::milliseconds( 2));
         }
         {
            auto message = queue::blocking::dequeue( "b3");
            validate_message( message);
            EXPECT_TRUE( message.attributes.available >= start + std::chrono::milliseconds( 3));
         }

      }

      TEST( queue_fanout, target_queue_full__error__expect_rollback)
      {
         common::unittest::Trace trace;

         constexpr auto configuration = R"(
domain: 
   name: A
   queue:
      groups:
         - alias: group-a
           queuebase: ":memory:"
           queues:
            -  name: a1
         - alias: group-b
           queuebase: ":memory:"
           capacity: 
            size: 500B
           queues:
            -  name: b1
            -  name: b2
            -  name: b3

      fanout:
         groups:
            -  alias: fanout-group-a
               queues:
                  -  alias: fo1
                     source: a1
                     targets:
                        -  queue: b1
                        -  queue: b2
                        -  queue: b3
      )";

        
         auto domain = local::domain( configuration);

         auto state = unittest::fetch::until( unittest::fetch::predicate::fanout_groups( 1));

         const auto payload = common::unittest::random::binary( 1024);

         const auto origin = local::message();

         // enqueue to source
         queue::enqueue( "a1", origin);

         // expect rollback to a1.error
         {
            auto message = queue::blocking::dequeue( "a1.error");
            EXPECT_TRUE( origin.payload.data == message.payload.data);
            EXPECT_TRUE( origin.payload.type == message.payload.type);
            EXPECT_TRUE( origin.attributes.properties == message.attributes.properties);
            EXPECT_TRUE( origin.attributes.reply == message.attributes.reply);
         }


      }
   } // queue
   
} // casual

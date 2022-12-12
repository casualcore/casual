//!
//! Copyright (c) 2020, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!

#include "common/unittest.h"
//#include "common/unittest/file.h"

#include "queue/unittest/utility.h"
#include "queue/common/queue.h"
#include "queue/api/queue.h"
#include "queue/code.h"

#include "common/process.h"
#include "common/message/domain.h"
#include "common/message/service.h"
#include "common/signal/timer.h"

#include "common/transaction/context.h"
#include "common/transaction/resource.h"

#include "common/serialize/macro.h"
#include "common/communication/instance.h"


#include "domain/unittest/manager.h"
#include "domain/unittest/configuration.h"

#include "service/unittest/utility.h"

#include "transaction/unittest/utility.h"

#include "configuration/model/transform.h"
#include "configuration/model/load.h"


namespace casual
{
   using namespace common;
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
               
           

               static constexpr auto queue = R"(
domain: 
   name: forward-domain

   queue:
      note: some note...
      default:
         queue: 
            retry:
               count: 3

      groups:
         - alias: a
           queuebase: ":memory:"
           note: alias-a
           queues:
            -  name: a1
               note: queue a1
            -  name: a2
            -  name: a3
            -  name: a4
         - alias: b
           queuebase: ":memory:"
           queues:
            - name: b1
            - name: b2
            - name: b3
            - name: b4
         - alias: c
           queuebase: ":memory:"
           queues:
            - name: c1
            - name: c2
            - name: c3
            - name: c4


      forward:
         groups:
            -  alias: forward-group-1
               note: some note..
               services:
                  -  alias: foo
                     source: a1
                     target: 
                        service: queue/unittest/service
                     instances: 4
                     reply:
                        queue: b1
                     note: some note...
                  -  alias: bar
                     source: a2
                     target: 
                        service: queue/unittest/service
                     instances: 4
                     reply:
                        queue: b2
                  -  source: a3
                     target: 
                        service: queue/unittest/service
                     instances: 1
                     reply:
                        queue: b3

                  -  source: x1
                     target: 
                        service: queue/unittest/service
                     instances: 4
                     reply:
                        queue: b1

               queues:
                  -  source: a4
                     target:
                        queue: b4
                        delay: 10ms
                     instances: 2 
                  
                  # a forward 'chain' from c1 -> c4 
                  -  source: c1
                     target:
                        queue: c2
                     instances: 1

                  -  source: c2
                     target:
                        queue: c3
                     instances: 1

                  -  source: c3
                     target:
                        queue: c4
                     instances: 1
)";

               template< typename... C>
               auto load( C&&... contents)
               {
                  auto files = common::unittest::file::temporary::contents( ".yaml", std::forward< C>( contents)...);

                  auto get_path = []( auto& file){ return static_cast< std::filesystem::path>( file);};

                  return casual::configuration::model::load( common::algorithm::transform( files, get_path));
               }

             } // configuration

            template< typename... C>
            auto domain( C&&... configurations)
            {
               return casual::domain::unittest::manager( configuration::servers, std::forward< C>( configurations)...);
            }

            //! default domain
            auto domain()
            {
               return domain( configuration::queue);
            }

         } // <unnamed>
      } // local

      TEST( casual_queue_forward, domain_startup)
      {
         common::unittest::Trace trace;

         EXPECT_NO_THROW(
            auto domain = local::domain();
         );
      }

      TEST( casual_queue_forward, configuration_get)
      {
         common::unittest::Trace trace;

         auto domain = local::domain();

         auto origin = local::configuration::load( local::configuration::servers, local::configuration::queue);

         auto model = casual::configuration::model::transform( casual::domain::unittest::configuration::get());

         EXPECT_TRUE( origin.queue == model.queue) << CASUAL_NAMED_VALUE( origin.queue) << '\n' << CASUAL_NAMED_VALUE( model.queue);

      }


      TEST( casual_queue_forward, configuration_post)
      {
         common::unittest::Trace trace;

         constexpr auto extra = R"(
domain:
   queue:
      groups:
         -  alias: extra
            queues:
               -  name: x1
               -  name: x2
               -  name: x3
)";

         auto domain = local::domain();

         auto wanted = local::configuration::load( local::configuration::servers, local::configuration::queue, extra);

         // make sure the wanted differs (otherwise we're not testing anyting...)
         ASSERT_TRUE( wanted.queue != casual::configuration::model::transform( casual::domain::unittest::configuration::get()).queue);

         // post the wanted model (in transformed user representation)
         auto updated = casual::configuration::model::transform( 
            casual::domain::unittest::configuration::post( casual::configuration::model::transform( wanted)));

         EXPECT_TRUE( wanted.queue == updated.queue) << CASUAL_NAMED_VALUE( wanted.queue) << '\n' << CASUAL_NAMED_VALUE( updated.queue);

      }

      TEST( casual_queue_forward, state)
      {
         common::unittest::Trace trace;
         
         auto domain = local::domain();

         auto state = unittest::state();

         EXPECT_TRUE( state.forward.services.size() == 4) << CASUAL_NAMED_VALUE( state.forward.services.size());
         EXPECT_TRUE( state.forward.queues.size() == 4) << CASUAL_NAMED_VALUE( state.forward.queues.size());

      }

      TEST( casual_queue_forward, scale_all_aliases_to_5_instances)
      {
         common::unittest::Trace trace;
         
         auto domain = local::domain();

         unittest::scale::all::forward::aliases( 5);

         auto state = unittest::state();

         auto has_instances = []( auto& forward){ return forward.instances.configured == 5;};

         EXPECT_TRUE( common::algorithm::all_of( state.forward.services, has_instances));
      }

      TEST( casual_queue_forward, scale_all_aliases_to_0_instances)
      {
         common::unittest::Trace trace;
         
         auto domain = local::domain();

         unittest::scale::all::forward::aliases( 0);

         auto state = unittest::state();

         auto has_instances = []( auto& forward){ return forward.instances.configured == 0;};

         EXPECT_TRUE( common::algorithm::all_of( state.forward.services, has_instances));
      }

      TEST( casual_queue_forward, scale_alias_up_and_down__expect_running_and_configured_to_match)
      {
         common::unittest::Trace trace;

         constexpr auto configuration = R"(
domain: 
   name: unittest

   queue:
      groups:
         -  alias: a
            queuebase: ":memory:"
            queues:
               - name: a1
               - name: a2
               - name: a3

      forward:
         groups:
            -  alias: forward-group-1
               services:
                  -  alias: foo
                     source: a1
                     target: 
                        service: queue/unittest/service
                     instances: 1
                     reply:
                        queue: a2
                  -  alias: bar
                     source: a2
                     target: 
                        service: queue/unittest/service
                     instances: 1
                     reply:
                        queue: a3
)";

         auto domain = local::domain( configuration);

         auto scale_forward = []( platform::size::type instances, std::string_view alias)
         {
            manager::admin::model::scale::Alias request;
            request.name = alias;
            request.instances = instances;

            unittest::scale::aliases( { request});

            // Fetch until we have as many running instances as configured
            unittest::fetch::until( [ instances, alias]( auto& state)
            {
               auto found = algorithm::find_if( state.forward.services, [ &alias]( const auto& forward)
               {
                  return forward.alias == alias;
               });

               return found && found->instances.configured == instances && found->instances.running == instances;
            });
         };

         algorithm::for_n< 10>( [ &scale_forward]
         {
            scale_forward( 5, "foo");
            scale_forward( 1, "foo");
         });
      }

      TEST( casual_queue_forward, advertise_service__enqueue__expect_forward)
      {
         common::unittest::Trace trace;

         constexpr auto configuration = R"(
domain: 
   name: service-forward

   queue:
      groups:
         -  alias: a
            queuebase: ":memory:"
            queues:
               - name: a1
               - name: a2

      forward:
         groups:
            -  alias: forward-group-1
               services:
                  -  alias: foo
                     source: a1
                     target: 
                        service: queue/unittest/service
                     instances: 1
                     reply:
                        queue: a2
)";

         auto domain = local::domain( configuration);

         casual::service::unittest::advertise( { "queue/unittest/service"});

         const auto payload = common::unittest::random::binary( 1024);

         // enqueue
         {
            queue::Message message;
            message.payload.type = common::buffer::type::binary;
            message.payload.data = payload;

            queue::enqueue( "a1", message);
         }

         signal::timer::Scoped alarm{ std::chrono::seconds{ 5}};

         // we expect to get a forward call
         {
            common::message::service::call::callee::Request request;
            
            common::communication::device::blocking::receive( 
               common::communication::ipc::inbound::device(),
               request);

            EXPECT_TRUE( request.buffer.data == payload);
            EXPECT_TRUE( request.buffer.type == common::buffer::type::binary);

            auto reply = common::message::reverse::type( request);
            reply.buffer = std::move( request.buffer);
            common::communication::device::blocking::send( request.process.ipc, reply);
         }

         // we expect the reply to be enqueued to a2
         {
            auto message = queue::blocking::dequeue( "a2");
            EXPECT_TRUE( message.payload.data == payload);
            EXPECT_TRUE( message.payload.type == common::buffer::type::binary);
         }
      }

      TEST( casual_queue_forward, advertise_service__enqueue_2_message__expect_forward_on_first__and_service_lookup_discard_on_shutdown)
      {
         common::unittest::Trace trace;

         auto domain = local::domain();

         constexpr auto service = "queue/unittest/service";

         casual::service::unittest::advertise( { service});

         const auto payload = common::unittest::random::binary( 1024);

         // enqueue
         {
            queue::Message message;
            message.payload.type = common::buffer::type::binary;
            message.payload.data = payload;

            queue::enqueue( "a3", message);
            queue::enqueue( "a3", message);
         }

         // we expect to get a forward call
         {
            common::message::service::call::callee::Request request;
            
            common::communication::device::blocking::receive( 
               common::communication::ipc::inbound::device(),
               request);

            EXPECT_TRUE( request.buffer.data == payload);
            EXPECT_TRUE( request.buffer.type == common::buffer::type::binary);

            auto reply = common::message::reverse::type( request);
            reply.buffer = std::move( request.buffer);
            common::communication::device::blocking::send( request.process.ipc, reply);

            casual::service::unittest::unadvertise( { service});

            casual::service::unittest::send::ack( request);
         }

         // we expect the reply to be enqueued to b3
         {
            auto message = queue::blocking::dequeue( "b3");
            EXPECT_TRUE( message.payload.data == payload);
            EXPECT_TRUE( message.payload.type == common::buffer::type::binary);
         }
      }

      TEST( casual_queue_forward, enqueue_a4__expect_queue_forward_to_b4)
      {
         common::unittest::Trace trace;

         auto domain = local::domain();


         const auto payload = common::unittest::random::binary( 1024);

         auto start = platform::time::clock::type::now();

         // enqueue
         {
            queue::Message message;
            message.payload.type = common::buffer::type::binary;
            message.payload.data = payload;

            queue::enqueue( "a4", message);
         }


         // we expect the message to be forward to b4, and it takes at least 10ms (we got a 10ms delay)
         {
            auto message = queue::blocking::dequeue( "b4");

            EXPECT_TRUE( platform::time::clock::type::now() - start > std::chrono::milliseconds{ 10});
            EXPECT_TRUE( message.payload.data == payload);
            EXPECT_TRUE( message.payload.type == common::buffer::type::binary);
         }
      }

      TEST( casual_queue_forward, enqueue_c1__expect_queue_forward_to_c2_c3_c4)
      {
         common::unittest::Trace trace;

         auto domain = local::domain();

         const auto payload = common::unittest::random::binary( 1024);

         // enqueue
         {
            queue::Message message;
            message.payload.type = common::buffer::type::binary;
            message.payload.data = payload;

            queue::enqueue( "c1", message);
         }


         // we expect the message to be forward to b4, and it takes at least 10ms (we got a 10ms delay)
         {
            auto message = queue::blocking::dequeue( "c4");
            EXPECT_TRUE( message.payload.data == payload);
            EXPECT_TRUE( message.payload.type == common::buffer::type::binary);
         }
      }

      TEST( casual_queue_forward, service_queue_forward__ring__enqueue_10_message__scale_to_0__expect__gracefull_shutdown)
      {
         common::unittest::Trace trace;

         static constexpr auto configuration = R"(
domain: 
   name: forward-domain

   queue:
      groups:
         - name: a
           queuebase: ":memory:"
           queues:
            - name: a1
            - name: a2
            - name: a3
         - name: b
           queuebase: ":memory:"
           queues:
            - name: b1
            - name: b2
            - name: b3

      forward:
         default:
            service:
               instances: 0
            queue:
               instances: 0
         groups:
            -  alias: forward-group-1
               services:
                  -  source: a1
                     target: 
                        service: queue/unittest/service
                     reply:
                        queue: b1
               queues:
                  # circle...
                  -  source: b1
                     target:
                        queue: b2
                  -  source: b2
                     target:
                        queue: b3
                  -  source: b3
                     target:
                        queue: b1
)";

         auto domain = local::domain( configuration);

         casual::service::unittest::advertise( { "queue/unittest/service"});

         const auto payload = common::unittest::random::binary( 1024);

         // enqueue
         {
            queue::Message message;
            message.payload.type = common::buffer::type::binary;
            message.payload.data = payload;


            common::algorithm::for_n< 10>( [&message]()
            {
               queue::enqueue( "b1", message);
            });

            common::algorithm::for_n< 10>( [&message]()
            {
               queue::enqueue( "a1", message);
            });
         }

         unittest::scale::all::forward::aliases( 4);

         // we only replies to one of the service calls, and not send 'ACK' to service-manger
         // hence, the service-forwards will be waiting for a service-lookup-reply from service-manager
         {
            common::message::service::call::callee::Request request;
            
            common::communication::device::blocking::receive( 
               common::communication::ipc::inbound::device(),
               request);

            EXPECT_TRUE( request.buffer.data == payload);
            EXPECT_TRUE( request.buffer.type == common::buffer::type::binary);

            auto reply = common::message::reverse::type( request);
            reply.buffer = std::move( request.buffer);
            common::communication::device::blocking::send( request.process.ipc, reply);
         }

         unittest::scale::all::forward::aliases( 0);


         auto state = unittest::fetch::until( []( auto& state)
         {
            auto has_zero = []( auto forward)
            {
               return forward.instances.configured == 0 && forward.instances.running == 0;
            };

            return common::algorithm::all_of( state.forward.services, has_zero) &&
               common::algorithm::all_of( state.forward.queues, has_zero);
         });


         // check queue-forwards, there should not be any rollbacks on queue-forwards
         {
            auto no_rollback = []( auto& forward)
            {
               return forward.metric.rollback.count == 0;
            };
            EXPECT_TRUE( common::algorithm::all_of( state.forward.queues, no_rollback)) << CASUAL_NAMED_VALUE( state);
         }

         // check service-forward
         {
            // only one has been committed (since we only allowed one call to this unittest-binary)
            EXPECT_TRUE( state.forward.services.at( 0).metric.commit.count == 1);
         }

         {
            auto state = casual::transaction::unittest::fetch::until( casual::transaction::unittest::fetch::predicate::transactions( 0));

            EXPECT_TRUE( state.transactions.empty()) << CASUAL_NAMED_VALUE( state);
            // groups a and b
            EXPECT_TRUE( state.externals.size() == 2) << CASUAL_NAMED_VALUE( state);
         }
   
      }

   } // queue
} // casual
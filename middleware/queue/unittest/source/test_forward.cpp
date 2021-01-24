//!
//! Copyright (c) 2020, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!

#include "common/unittest.h"

#include "queue/common/queue.h"
#include "queue/api/queue.h"
#include "queue/manager/admin/model.h"
#include "queue/manager/admin/services.h"
#include "queue/code.h"

#include "common/process.h"
#include "common/message/gateway.h"
#include "common/message/domain.h"
#include "common/message/service.h"

#include "common/transaction/context.h"
#include "common/transaction/resource.h"

#include "common/communication/instance.h"

#include "serviceframework/service/protocol/call.h"
#include "common/serialize/macro.h"

#include "domain/manager/unittest/process.h"
#include "service/unittest/advertise.h"



#include <fstream>

namespace casual
{
   using namespace common;
   namespace queue
   {
      namespace local
      {
         namespace
         {
            struct Domain 
            {
               Domain( std::string configuration) 
                  : domain{ { std::move( configuration)}} {}

               Domain() : Domain{ Domain::configuration} {}

               casual::domain::manager::unittest::Process domain;

               static constexpr auto configuration = R"(
domain: 
   name: forward-domain

   groups: 
      - name: base
      - name: queue
        dependencies: [ base]

   servers:
      - path: "${CASUAL_HOME}/bin/casual-service-manager"
        memberships: [ base]
      - path: "${CASUAL_HOME}/bin/casual-transaction-manager"
        memberships: [ base]
      - path: "./bin/casual-queue-manager"
        memberships: [ queue]

   queue:
      default:
         queue: 
            retry:
               count: 3

      groups:
         - alias: a
           queuebase: ":memory:"
           queues:
            - name: a1
            - name: a2
            - name: a3
            - name: a4
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
               services:
                  -  alias: foo
                     source: a1
                     target: 
                        service: queue/unittest/service
                     instances: 4
                     reply:
                        queue: b1
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
            };


            namespace call
            {
               manager::admin::model::State state()
               {
                  serviceframework::service::protocol::binary::Call call;
                  auto reply = call( manager::admin::service::name::state);

                  manager::admin::model::State result;
                  reply >> CASUAL_NAMED_VALUE( result);

                  return result;
               }

               void scale_aliases( const std::vector< manager::admin::model::scale::Alias>& aliases)
               {
                  serviceframework::service::protocol::binary::Call call;
                  call << CASUAL_NAMED_VALUE( aliases);
                  call( manager::admin::service::name::forward::scale::aliases);
               }

            } // call

            void scale_all_forward_aliases( platform::size::type instances)
            {
               auto state = local::call::state();

               auto scale_forward = [instances]( auto& forward)
               {
                  manager::admin::model::scale::Alias result;
                  result.name = forward.alias;
                  result.instances = instances;
                  return result;
               };

               auto aliases = common::algorithm::transform( state.forward.services, scale_forward);
               common::algorithm::transform( state.forward.queues, aliases, scale_forward);

               call::scale_aliases( aliases);
            }

         } // <unnamed>
      } // local

      TEST( casual_queue_forward, domain_startup)
      {
         common::unittest::Trace trace;

         EXPECT_NO_THROW(
            local::Domain domain;
         );
      }

      TEST( casual_queue_forward, state)
      {
         common::unittest::Trace trace;
         
         local::Domain domain;

         auto state = local::call::state();

         EXPECT_TRUE( state.forward.services.size() == 4) << CASUAL_NAMED_VALUE( state.forward.services.size());
         EXPECT_TRUE( state.forward.queues.size() == 4) << CASUAL_NAMED_VALUE( state.forward.queues.size());

      }

      TEST( casual_queue_forward, scale_all_aliases_to_5_instances)
      {
         common::unittest::Trace trace;
         
         local::Domain domain;

         local::scale_all_forward_aliases( 5);

         auto state = local::call::state();

         auto has_instances = []( auto& forward){ return forward.instances.configured == 5;};

         EXPECT_TRUE( common::algorithm::all_of( state.forward.services, has_instances));
      }

      TEST( casual_queue_forward, scale_all_aliases_to_0_instances)
      {
         common::unittest::Trace trace;
         
         local::Domain domain;

         local::scale_all_forward_aliases( 0);

         auto state = local::call::state();

         auto has_instances = []( auto& forward){ return forward.instances.configured == 0;};

         EXPECT_TRUE( common::algorithm::all_of( state.forward.services, has_instances));
      }

      TEST( casual_queue_forward, advertise_service__enqueue__expect_forward)
      {
         common::unittest::Trace trace;

         constexpr auto configuration = R"(
domain: 
   name: service-forward

   groups: 
      -  name: base
      -  name: queue
         dependencies: [ base]

   servers:
      -  path: "${CASUAL_HOME}/bin/casual-service-manager"
         memberships: [ base]
      -  path: "${CASUAL_HOME}/bin/casual-transaction-manager"
         memberships: [ base]
      -  path: "bin/casual-queue-manager"
         memberships: [ queue]

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

         local::Domain domain{ configuration};

         casual::service::unittest::advertise( { "queue/unittest/service"});

         const auto payload = common::unittest::random::binary( 1024);

         // enqueue
         {
            queue::Message message;
            message.payload.type = common::buffer::type::binary();
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

            EXPECT_TRUE( request.buffer.memory == payload);
            EXPECT_TRUE( request.buffer.type == common::buffer::type::binary());

            auto reply = common::message::reverse::type( request);
            reply.buffer = std::move( request.buffer);
            common::communication::device::blocking::send( request.process.ipc, reply);
         }

         // we expect the reply to be enqueued to a2
         {
            auto message = queue::blocking::dequeue( "a2");
            EXPECT_TRUE( message.payload.data == payload);
            EXPECT_TRUE( message.payload.type == common::buffer::type::binary());
         }
      }

      TEST( casual_queue_forward, advertise_service__enqueue_2_message__expect_forward_on_first__and_service_lookup_discard_on_shutdown)
      {
         common::unittest::Trace trace;

         local::Domain domain;

         casual::service::unittest::advertise( { "queue/unittest/service"});

         const auto payload = common::unittest::random::binary( 1024);

         // enqueue
         {
            queue::Message message;
            message.payload.type = common::buffer::type::binary();
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

            EXPECT_TRUE( request.buffer.memory == payload);
            EXPECT_TRUE( request.buffer.type == common::buffer::type::binary());

            auto reply = common::message::reverse::type( request);
            reply.buffer = std::move( request.buffer);
            common::communication::device::blocking::send( request.process.ipc, reply);
         }

         // we expect the reply to be enqueued to b3
         {
            auto message = queue::blocking::dequeue( "b3");
            EXPECT_TRUE( message.payload.data == payload);
            EXPECT_TRUE( message.payload.type == common::buffer::type::binary());
         }
      }

      TEST( casual_queue_forward, enqueue_a4__expect_queue_forward_to_b4)
      {
         common::unittest::Trace trace;

         local::Domain domain;


         const auto payload = common::unittest::random::binary( 1024);

         auto start = platform::time::clock::type::now();

         // enqueue
         {
            queue::Message message;
            message.payload.type = common::buffer::type::binary();
            message.payload.data = payload;

            queue::enqueue( "a4", message);
         }


         // we expect the message to be forward to b4, and it takes at least 10ms (we got a 10ms delay)
         {
            auto message = queue::blocking::dequeue( "b4");

            EXPECT_TRUE( platform::time::clock::type::now() - start > std::chrono::milliseconds{ 10});
            EXPECT_TRUE( message.payload.data == payload);
            EXPECT_TRUE( message.payload.type == common::buffer::type::binary());
         }
      }

      TEST( casual_queue_forward, enqueue_c1__expect_queue_forward_to_c2_c3_c4)
      {
         common::unittest::Trace trace;

         local::Domain domain;

         const auto payload = common::unittest::random::binary( 1024);

         // enqueue
         {
            queue::Message message;
            message.payload.type = common::buffer::type::binary();
            message.payload.data = payload;

            queue::enqueue( "c1", message);
         }


         // we expect the message to be forward to b4, and it takes at least 10ms (we got a 10ms delay)
         {
            auto message = queue::blocking::dequeue( "c4");
            EXPECT_TRUE( message.payload.data == payload);
            EXPECT_TRUE( message.payload.type == common::buffer::type::binary());
         }
      }

      TEST( casual_queue_forward, service_queue_forward__ring__enqueue_10_message__scale_to_0__expect__gracefull_shutdown)
      {
         common::unittest::Trace trace;

         static constexpr auto configuration = R"(
domain: 
   name: forward-domain

   groups: 
      -  name: base
      -  name: queue
         dependencies: [ base]

   servers:
      -  path: "${CASUAL_HOME}/bin/casual-service-manager"
         memberships: [ base]
      -  path: "${CASUAL_HOME}/bin/casual-transaction-manager"
         memberships: [ base]
      -  path: "./bin/casual-queue-manager"
         memberships: [ queue]

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

         local::Domain domain{ configuration};

         casual::service::unittest::advertise( { "queue/unittest/service"});

         const auto payload = common::unittest::random::binary( 1024);

         // enqueue
         {
            queue::Message message;
            message.payload.type = common::buffer::type::binary();
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

         local::scale_all_forward_aliases( 4);

         // we only replies to one of the service calls, and not send 'ACK' to service-manger
         // hence, the service-forwards will be wating for a service-lookup-reply from service-manager
         {
            common::message::service::call::callee::Request request;
            
            common::communication::device::blocking::receive( 
               common::communication::ipc::inbound::device(),
               request);

            EXPECT_TRUE( request.buffer.memory == payload);
            EXPECT_TRUE( request.buffer.type == common::buffer::type::binary());

            auto reply = common::message::reverse::type( request);
            reply.buffer = std::move( request.buffer);
            common::communication::device::blocking::send( request.process.ipc, reply);
         }

         local::scale_all_forward_aliases( 0);

         auto zero_instances = []( auto& state)
         {
            auto has_zero = []( auto forward)
            {
               return forward.instances.configured == 0 && forward.instances.running == 0;
            };

            return common::algorithm::all_of( state.forward.services, has_zero) &&
               common::algorithm::all_of( state.forward.queues, has_zero);
         };

         auto state = local::call::state();

         while( ! zero_instances( state))
         {
            common::process::sleep( std::chrono::milliseconds{ 5});
            state = local::call::state();
         }

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
      }

   } // queue
} // casual
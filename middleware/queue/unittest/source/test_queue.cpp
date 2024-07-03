//! 
//! Copyright (c) 2015, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!


#include "common/unittest.h"
#include "common/unittest/file.h"
#include "common/unittest/environment.h"

#include "queue/unittest/utility.h"
#include "queue/common/queue.h"
#include "queue/api/queue.h"

#include "common/array.h"
#include "common/code/queue.h"
#include "common/process.h"
#include "common/file.h"
#include "common/message/domain.h"
#include "common/environment.h"

#include "common/transaction/context.h"
#include "common/transaction/resource.h"
#include "common/transaction/global.h"

#include "common/code/signal.h"
#include "common/signal.h"
#include "common/communication/instance.h"
#include "common/serialize/macro.h"

#include "domain/discovery/api.h"
#include "domain/unittest/manager.h"

#include "queue/manager/admin/services.h"
#include "serviceframework/service/protocol.h"
#include "serviceframework/service/protocol/call.h"

#include <fstream>
#include <future>

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
               constexpr auto servers = R"(
domain: 
   name: queue-domain

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

               constexpr auto queue = R"(
domain:
   queue:
      note: some note...
      default:
         queue: 
            retry:
               count: 3

      groups:
         - alias: A
           queuebase: ":memory:"
           queues:
            -  name: a1
               note: some note...
            -  name: a2
            -  name: a3
            -  name: delayed_100ms
               retry: { count: 10, delay: 100ms}
         - alias: B
           queuebase: ":memory:"
           queues:
            - name: b1
            - name: b2
            - name: b3

)";


            } // configuration

            template< typename... C>
            auto domain( C&&... configurations)
            {
               return domain::unittest::manager( configuration::servers, std::forward< C>( configurations)...);
            }

            //! default domain
            auto domain()
            {
               return domain( configuration::queue);
            }

         } // <unnamed>

      } // local




      TEST( casual_queue, manager_startup)
      {
         common::unittest::Trace trace;

         auto domain = local::domain();

         auto state = unittest::state();

         EXPECT_TRUE( state.groups.size() == 2);
         EXPECT_TRUE( state.queues.size() == ( 3 + 4) * 2)  << "state.queues.size(): " << state.queues.size();
      }


      TEST( casual_queue, manager_correct_retry_state)
      {
         common::unittest::Trace trace;

         auto domain = local::domain();

         auto state = unittest::state();

         auto find_and_compare_queue = [&state]( const std::string& name, auto&& predicate)
         {
            auto found = common::algorithm::find( state.queues, name);
            if( ! found)
               return false;

            return predicate( *found);
         };

         auto retry_predicate = []( auto count, auto delay)
         {
            return [count, delay]( auto& queue)
            {
               return queue.retry.count == count && queue.retry.delay == delay;
            };
         };

         auto default_retry = retry_predicate( 3, std::chrono::seconds{ 0});

         EXPECT_TRUE( find_and_compare_queue( "a1", default_retry)) << CASUAL_NAMED_VALUE( state.queues);
         EXPECT_TRUE( find_and_compare_queue( "a2", default_retry));
         EXPECT_TRUE( find_and_compare_queue( "a3", default_retry));
         EXPECT_TRUE( find_and_compare_queue( "delayed_100ms", retry_predicate( 10, std::chrono::milliseconds{ 100})));

         EXPECT_TRUE( find_and_compare_queue( "b1", default_retry)) << CASUAL_NAMED_VALUE( state.queues);
         EXPECT_TRUE( find_and_compare_queue( "b2", default_retry));
         EXPECT_TRUE( find_and_compare_queue( "b3", default_retry));
      }


      TEST( casual_queue, lookup_request_a1__expect_existence)
      {
         common::unittest::Trace trace;

         auto domain = local::domain();

         {
            // Send request
            ipc::message::lookup::Request request;
            request.process = common::process::handle();
            request.name = "a1";

            common::communication::device::blocking::send( 
               common::communication::instance::outbound::queue::manager::device(), 
               request);
         }

         {
            ipc::message::lookup::Reply reply;
            common::communication::device::blocking::receive( common::communication::ipc::inbound::device(), reply);

            EXPECT_TRUE( reply.queue) << CASUAL_NAMED_VALUE( reply);
         }
      }

      TEST( casual_queue, lookup_request_non_existence__expect_absent)
      {
         common::unittest::Trace trace;

         auto domain = local::domain();

         {
            // Send request
            ipc::message::lookup::Request request;
            request.process = common::process::handle();
            request.name = "non-existence";

            common::communication::device::blocking::send( 
               common::communication::instance::outbound::queue::manager::device(), 
               request);
         }

         {
            ipc::message::lookup::Reply reply;
            common::communication::device::blocking::receive( common::communication::ipc::inbound::device(), reply);

            EXPECT_FALSE( reply.queue) << CASUAL_NAMED_VALUE( reply);
         }
      }

      TEST( casual_queue, multiple_lookup_request__expect_one_discovery)
      {
         common::unittest::Trace trace;

         auto domain = local::domain();

         domain::discovery::provider::registration( domain::discovery::provider::Ability::discover);

         auto send_lookup_request = []()
         {
            ipc::message::lookup::Request request{ common::process::handle()};
            request.name = "a";

            common::communication::device::blocking::send( 
               common::communication::instance::outbound::queue::manager::device(), 
               request);
         };

         send_lookup_request();

         {
            ipc::message::Advertise remote;

            remote.process = common::process::handle();
            remote.queues.add.push_back( { "a", {}, {}});

            common::communication::device::blocking::send( 
               common::communication::instance::outbound::queue::manager::device(), remote);
         }

         {
            // Receive discovery request and send reply
            auto request = common::communication::ipc::receive< domain::message::discovery::Request>();

            EXPECT_TRUE( common::algorithm::equal( request.content.queues, common::array::make( "a")));

            auto reply = common::message::reverse::type( request);
            reply.content.queues = { domain::message::discovery::reply::content::Queue{ "a"}};
            common::communication::device::blocking::send( request.process.ipc, reply);
         }
         
         {
            // Receive lookup reply
            auto reply = common::communication::ipc::receive< ipc::message::lookup::Reply>();
            EXPECT_TRUE( reply.queue) << CASUAL_NAMED_VALUE( reply);
         }

         send_lookup_request();
         
         {
            // Expect to not receive discovery request
            domain::message::discovery::Request request;
            common::algorithm::for_n( 5, [&request]()
            {
               EXPECT_FALSE( common::communication::device::non::blocking::receive( common::communication::ipc::inbound::device(), request));
               common::process::sleep( std::chrono::milliseconds{ 1});
            });
         }
      }

      TEST( casual_queue, enqueue_1_message___expect_1_message_in_queue)
      {
         common::unittest::Trace trace;

         auto domain = local::domain();

         const auto payload = common::unittest::random::binary( 64);
         queue::Message message;
         message.payload.type = common::buffer::type::binary;
         message.payload.data = payload;

         queue::enqueue( "a1", message);
         auto messages = unittest::messages( "a1"); // unittest::messages( "a1");

         EXPECT_TRUE( messages.size() == 1);
      }

      TEST( casual_queue, enqueue_1_message_with_no_payload___expect_1_message_in_queue)
      {
         common::unittest::Trace trace;

         auto domain = local::domain();

         queue::Message message;

         queue::enqueue( "a1", message);
         auto messages = unittest::messages( "a1");

         EXPECT_TRUE( messages.size() == 1);
      }


      TEST( casual_queue, enqueue_5_message___expect_5_message_in_queue)
      {
         common::unittest::Trace trace;

         auto domain = local::domain();

         auto count = 5;
         while( count-- > 0)
         {
            const auto payload = common::unittest::random::binary( 64);
            queue::Message message;
            message.payload.data.assign( std::begin( payload), std::end( payload));

            queue::enqueue( "a1", message);
         }

         auto messages = unittest::messages( "a1");

         EXPECT_TRUE( messages.size() == 5);
      }
      namespace local
      {
         namespace
         {
            bool compare( const platform::time::point::type& lhs, const platform::time::point::type& rhs)
            {
               return std::chrono::time_point_cast< std::chrono::microseconds>( lhs)
                     == std::chrono::time_point_cast< std::chrono::microseconds>( rhs);
            }
         } // <unnamed>
      } // local

      TEST( casual_queue, enqueue_1_message__peek__information__expect_1_peeked)
      {
         common::unittest::Trace trace;

         auto domain = local::domain();

         auto now = platform::time::clock::type::now();

         const auto payload = common::unittest::random::binary( 64);
         queue::Message message;
         {
            message.attributes.available = now;
            message.attributes.properties = "poop";
            message.attributes.reply = "a2";
            message.payload.type = common::buffer::type::binary;
            message.payload.data.assign( std::begin( payload), std::end( payload));
         }

         auto id = queue::enqueue( "a1", message);

         auto information = queue::peek::information( "a1");

         ASSERT_TRUE( information.size() == 1);
         auto& info = information.at( 0);
         EXPECT_TRUE( info.id == id);
         EXPECT_TRUE( local::compare( info.attributes.available, now));
         EXPECT_TRUE( info.attributes.properties == "poop") << "info: " << CASUAL_NAMED_VALUE( info);
         EXPECT_TRUE( info.attributes.reply == "a2");
         EXPECT_TRUE( info.payload.type == common::buffer::type::binary);
         EXPECT_TRUE( info.payload.size == common::range::size( payload));

         // message should still be there
         {
            auto messages = queue::dequeue( "a1");
            EXPECT_TRUE( ! messages.empty());
            auto& message = messages.at( 0);
            EXPECT_TRUE( message.id == id);
            EXPECT_TRUE( common::algorithm::equal( message.payload.data, payload));
         }
      }

      TEST( casual_queue, enqueue_1_message__peek_message__expect_1_peeked)
      {
         common::unittest::Trace trace;

         auto domain = local::domain();

         auto now = platform::time::clock::type::now();

         const auto payload = common::unittest::random::binary( 64);

         auto enqueue = [&](){
            queue::Message message;
            {
               message.attributes.available = now;
               message.attributes.properties = "poop";
               message.attributes.reply = "a2";
               message.payload.type = common::buffer::type::binary;
               message.payload.data.assign( std::begin( payload), std::end( payload));
            }

            return queue::enqueue( "a1", message);
         };

         auto id = enqueue();

         auto messages = queue::peek::messages( "a1", { id});

         ASSERT_TRUE( messages.size() == 1);
         auto& message = messages.at( 0);
         EXPECT_TRUE( message.id == id) << "message: " << CASUAL_NAMED_VALUE( message);
         EXPECT_TRUE( local::compare( message.attributes.available, now));
         EXPECT_TRUE( message.attributes.properties == "poop");
         EXPECT_TRUE( message.attributes.reply == "a2");
         EXPECT_TRUE( message.payload.type == common::buffer::type::binary);
         EXPECT_TRUE( common::algorithm::equal( message.payload.data, payload));

         // message should still be there
         {
            auto messages = queue::dequeue( "a1");
            EXPECT_TRUE( ! messages.empty());
            auto& message = messages.at( 0);
            EXPECT_TRUE( message.id == id);
            EXPECT_TRUE( common::algorithm::equal( message.payload.data, payload));
         }
      }

      TEST( casual_queue, enqueue_1_message_with_no_payload__peek_message__expect_1_peeked)
      {
         common::unittest::Trace trace;

         auto domain = local::domain();

         auto now = platform::time::clock::type::now();

         auto enqueue = [&](){
            queue::Message message;
            {
               message.attributes.available = now;
               message.attributes.properties = "poop";
               message.attributes.reply = "a2";
            }

            return queue::enqueue( "a1", message);
         };

         auto id = enqueue();

         auto messages = queue::peek::messages( "a1", { id});

         ASSERT_TRUE( messages.size() == 1);
         auto& message = messages.at( 0);
         EXPECT_TRUE( message.id == id) << "message: " << CASUAL_NAMED_VALUE( message);
         EXPECT_TRUE( local::compare( message.attributes.available, now));
         EXPECT_TRUE( message.attributes.properties == "poop");
         EXPECT_TRUE( message.attributes.reply == "a2");

         const decltype( message.payload.data) empty{};
         EXPECT_TRUE( message.payload.data == empty);

         // message should still be there
         {
            auto messages = queue::dequeue( "a1");
            EXPECT_TRUE( ! messages.empty());
            auto& message = messages.at( 0);
            EXPECT_TRUE( message.id == id);
            EXPECT_TRUE( message.payload.data == empty);
         }
      }



      TEST( casual_queue, peek_remote_message___expect_throw)
      {
         common::unittest::Trace trace;

         auto domain = local::domain();

         // make sure casual-manager-queue knows about a "remote queue"
         {
            ipc::message::Advertise remote;

            remote.process.pid = common::strong::process::id{ 666};
            remote.process.ipc = common::strong::ipc::id{ common::uuid::make()};

            remote.queues.add.push_back( { "remote-queue", {}, {}});

            common::communication::device::blocking::send( 
               common::communication::instance::outbound::queue::manager::device(), remote);
         }

         EXPECT_CODE({
            (void)queue::peek::information( "remote-queue");
         }, common::code::queue::argument);
      }

      TEST( casual_queue, enqueue_1_message__dequeue_1_message)
      {
         common::unittest::Trace trace;

         auto domain = local::domain();

         const auto payload = common::unittest::random::binary( 64);

         auto enqueue = [&](){
            queue::Message message;
            {
               message.attributes.properties = "poop";
               message.attributes.reply = "a2";
               message.payload.type = common::buffer::type::binary;
               message.payload.data.assign( std::begin( payload), std::end( payload));
            }

            return queue::enqueue( "a1", message);
         };

         enqueue();

         auto message = queue::dequeue( "a1");

         ASSERT_TRUE( message.size() == 1);
         EXPECT_TRUE( common::algorithm::equal( message.at( 0).payload.data, payload));
      }

      TEST( casual_queue, enqueue_1_message_delay_100ms__blocking_dequeue__expect_1_message_after_100ms)
      {
         common::unittest::Trace trace;

         auto domain = local::domain();

         const auto payload = common::unittest::random::binary( 64);

         // message will be available at this absolute time
         auto available = platform::time::clock::type::now() + std::chrono::milliseconds{ 100};

         {
            queue::Message message;
            {
               message.attributes.properties = "poop";
               message.attributes.reply = "a2";
               message.attributes.available = available;
               message.payload.type = common::buffer::type::binary;
               message.payload.data.assign( std::begin( payload), std::end( payload));
            }

            queue::enqueue( "a1", message);
         }

         auto message = queue::blocking::dequeue( "a1");
         
         // we expect that at least 100ms has passed
         EXPECT_TRUE( platform::time::clock::type::now() > available);
         EXPECT_TRUE( common::algorithm::equal( message.payload.data, payload));
      }

      TEST( casual_queue, enqueue_available_5min___expect_dequeue_with_id_to_ignore_available)
      {
         common::unittest::Trace trace;

         auto domain = local::domain( R"(
domain:
   name: A
   queue:
      groups:
         -  alias: A
            queuebase: ":memory:"
            queues:
               -  name: a1
)");

         const auto payload = common::unittest::random::binary( 128);

         const auto id = [ &]()
         {
            queue::Message message;
            {
               message.attributes.properties = "poop";
               message.attributes.available = platform::time::clock::type::now() + std::chrono::minutes{ 5};
               message.payload.type = common::buffer::type::binary;
               message.payload.data = payload;
            }

            return queue::enqueue( "a1", message);
         }();


         queue::Selector selector;
         selector.id = id;
         auto message = queue::dequeue( "a1", selector);

         ASSERT_TRUE( ! message.empty()) << CASUAL_NAMED_VALUE( id);
         
         // we expect the property available to be in the future.
         EXPECT_TRUE( message.at( 0).attributes.available > platform::time::clock::type::now());
         EXPECT_TRUE( common::algorithm::equal( message.at( 0).payload.data, payload));
      }

      TEST( casual_queue, enqueue_1_message___dequeue_rollback___expect_available_after_retry_delay_100ms)
      {
         common::unittest::Trace trace;

         auto domain = local::domain();

         const auto payload = common::unittest::random::binary( 64);

         constexpr auto name = "delayed_100ms";

         // enqueue
         {
            queue::Message message;
            {
               message.attributes.properties = "poop";
               message.payload.type = common::buffer::type::binary;
               message.payload.data.assign( std::begin( payload), std::end( payload));
            }
            queue::enqueue( name, message);
         }

         auto start = platform::time::clock::type::now();

         // dequeue, and rollback
         {
            EXPECT_EQ( common::transaction::context().begin(), common::code::tx::ok);
            ASSERT_TRUE( ! queue::dequeue( name).empty());
            EXPECT_EQ( common::transaction::context().rollback(), common::code::tx::ok);
         }  

         // not available yet
         ASSERT_TRUE( queue::dequeue( name).empty());

         auto message = queue::blocking::dequeue( name);

         // we expect at least 100ms has passed, 
         EXPECT_TRUE( platform::time::clock::type::now() - start > std::chrono::milliseconds{ 100});
         EXPECT_TRUE( common::algorithm::equal( message.payload.data, payload));
      }

      TEST( casual_queue, enqueue_1_message___dequeue_rollback___expect_message_on_error_queue_without_delay)
      {
         common::unittest::Trace trace;

         auto domain = local::domain( R"(
domain:
   name: A
   queue:
      groups:
         -  alias: A
            queuebase: ":memory:"
            queues:
               -  name: a1
                  retry:
                     count: 0
                     delay: 5s
)");

         const auto payload = common::unittest::random::binary( 64);

         constexpr auto name = "a1";
         constexpr auto error_name = "a1.error";

         // enqueue
         {
            queue::Message message;
            {
               message.attributes.properties = "poop";
               message.payload.type = common::buffer::type::binary;
               message.payload.data.assign( std::begin( payload), std::end( payload));
            }
            queue::enqueue( name, message);
         }

         // dequeue, and rollback
         {
            EXPECT_EQ( common::transaction::context().begin(), common::code::tx::ok);
            ASSERT_TRUE( ! queue::dequeue( name).empty());
            EXPECT_EQ( common::transaction::context().rollback(), common::code::tx::ok);
         }  

         // removed from original queue
         ASSERT_TRUE( queue::dequeue( name).empty());
         // immediately present on error queue
         auto messages = queue::dequeue( error_name);
         ASSERT_TRUE( messages.size() == 1);
         EXPECT_TRUE( common::algorithm::equal( messages.front().payload.data, payload));
      }

      namespace local
      {
         namespace
         {
            namespace blocking
            {
               template< typename IPC>
               void dequeue( IPC&& ipc, std::string name)
               {
                  queue::Lookup lookup{ std::move( name), queue::Lookup::Action::dequeue};

                  ipc::message::group::dequeue::Request message;
                  message.name = lookup.name();
                  message.block = true;
                  message.process.pid = common::process::id();
                  message.process.ipc = ipc.connector().handle().ipc();

                  auto destination = lookup();
                  message.queue = destination.queue;

                  common::communication::device::blocking::send( destination.process.ipc, message);
               }
            } // blocking
         } // <unnamed>
      } // local

      TEST( casual_queue, blocking_dequeue_4_message__enqueue_4_message)
      {
         common::unittest::Trace trace;

         auto domain = local::domain();

         common::communication::ipc::inbound::Device ipc1;
         common::communication::ipc::inbound::Device ipc2;
         common::communication::ipc::inbound::Device ipc3;
         common::communication::ipc::inbound::Device ipc4;

         local::blocking::dequeue( ipc1, "a1");
         local::blocking::dequeue( ipc2, "a1");
         local::blocking::dequeue( ipc3, "a1");
         local::blocking::dequeue( ipc4, "a1");

         const auto payload = common::unittest::random::binary( 64);

         auto enqueue = [&](){
            queue::Message message;
            {
               message.attributes.properties = "poop";
               message.attributes.reply = "a2";
               message.payload.type = common::buffer::type::binary;
               message.payload.data.assign( std::begin( payload), std::end( payload));
            }

            return queue::enqueue( "a1", message);
         };

         enqueue();
         enqueue();
         enqueue();
         enqueue();

         auto dequeue = []( auto& ipc){
            ipc::message::group::dequeue::Reply message;
            common::communication::device::blocking::receive( ipc, message);
            return message;
         };

         {
            auto message = dequeue( ipc1);
            ASSERT_TRUE( message.message);
            EXPECT_TRUE( common::algorithm::equal( message.message->payload.data, payload));
         }

         {
            auto message = dequeue( ipc2);
            ASSERT_TRUE( message.message);
            EXPECT_TRUE( common::algorithm::equal( message.message->payload.data, payload));
         }

         {
            auto message = dequeue( ipc3);
            ASSERT_TRUE( message.message);
            EXPECT_TRUE( common::algorithm::equal( message.message->payload.data, payload));
         }

         {
            auto message = dequeue( ipc4);
            ASSERT_TRUE( message.message);
            EXPECT_TRUE( common::algorithm::equal( message.message->payload.data, payload));
         }
      }

      TEST( BACKWARD_casual_queue, queue_forward__enqueue_A3__expect_forward_to_B3)
      {
         common::unittest::Trace trace;

         constexpr auto configuration = R"(
domain: 
   name: deprecated-forward

   groups: 
      - name: forward
        dependencies: [ queue]
   
   servers:
      - path: bin/casual-queue-forward-queue
        arguments: [ --forward, a3, b3]
        memberships: [ forward]
)";   

         auto domain = local::domain( local::configuration::queue, configuration);

         const auto payload = common::unittest::random::binary( 64);

         // enqueue
         {
            queue::Message message;
            message.payload.data.assign( std::begin( payload), std::end( payload));

            queue::enqueue( "a3", message);
         }

         auto message = queue::blocking::dequeue( "b3");

         EXPECT_TRUE( common::algorithm::equal( message.payload.data, payload));
      }


      TEST( casual_queue, enqueue_1___remove_message____expect_0_message_in_queue)
      {
         common::unittest::Trace trace;

         auto domain = local::domain();

         const auto payload = common::unittest::random::binary( 64);
         queue::Message message;
         message.payload.type = common::buffer::type::binary;
         message.payload.data.assign( std::begin( payload), std::end( payload));

         queue::enqueue( "a1", message);
         auto messages = unittest::messages( "a1");

         ASSERT_TRUE( messages.size() == 1);
         auto removed = queue::messages::remove( "a1", { messages.front().id});
         EXPECT_TRUE( messages.front().id == removed.at( 0));

         EXPECT_TRUE( unittest::messages( "a1").empty());
      }

      namespace local
      {
         namespace
         {
            std::vector< common::transaction::global::ID> recover( const std::vector< common::transaction::global::ID>& gtrids,
               ipc::message::group::message::recovery::Directive directive)
            {
               using Call = serviceframework::service::protocol::binary::Call;
               return Call{}( manager::admin::service::name::recover,
                  std::move( gtrids),
                  std::move( directive)).extract< std::vector< common::transaction::global::ID>>();
            }
         } // unnamed
      } // local

      TEST( casual_queue, enqueue_1___dequeue_message___recover_commit____expect_0_message_in_queue)
      {
         common::unittest::Trace trace;

         using Directive = ipc::message::group::message::recovery::Directive;

         auto domain = local::domain();

         const auto payload = common::unittest::random::binary( 64);
         queue::Message message;
         message.payload.type = common::buffer::type::binary;
         message.payload.data.assign( std::begin( payload), std::end( payload));

         queue::enqueue( "a1", message);
         {
            auto messages = unittest::messages( "a1");
            ASSERT_TRUE( messages.size() == 1);
         }

         {
            // simulate a uncommitted dequeue
            EXPECT_EQ( common::transaction::context().begin(), common::code::tx::ok);
            EXPECT_TRUE( ! queue::dequeue( "a1").empty());
            {
               auto messages = unittest::messages( "a1");
               ASSERT_TRUE( messages.size() == 1);
               auto trid = common::transcode::hex::encode( messages.front().trid);
               ASSERT_TRUE( trid.size() > 0);

               common::transaction::global::ID gtrid{ trid};

               auto committed = local::recover({ gtrid}, Directive::commit);
               ASSERT_TRUE( committed.front() == gtrid);
            }
         }

         {
            EXPECT_TRUE( unittest::messages( "a1").empty());
            EXPECT_TRUE( unittest::messages( "a1.error").empty());
         }

         // rollback simulated uncommited dequeue, otherwise casual-queue-group has ongoing transactions
         EXPECT_EQ( common::transaction::context().rollback(), common::code::tx::ok);
      }

      TEST( casual_queue, enqueue_1___dequeue_message___recover_rollback____expect_1_message_in_queue)
      {
         common::unittest::Trace trace;

         using Directive = ipc::message::group::message::recovery::Directive;

         auto domain = local::domain();

         const auto payload = common::unittest::random::binary( 64);
         queue::Message message;
         message.payload.type = common::buffer::type::binary;
         message.payload.data.assign( std::begin( payload), std::end( payload));

         queue::enqueue( "a1", message);
         {
            auto messages = unittest::messages( "a1");
            ASSERT_TRUE( messages.size() == 1);
         }

         {
            // simulate a uncommitted dequeue
            EXPECT_EQ( common::transaction::context().begin(), common::code::tx::ok);
            EXPECT_TRUE( ! queue::dequeue( "a1").empty());
            {
               auto messages = unittest::messages( "a1");
               ASSERT_TRUE( messages.size() == 1);
               EXPECT_TRUE( messages.front().redelivered == 0);

               auto trid = common::transcode::hex::encode( messages.front().trid);
               ASSERT_TRUE( trid.size() > 0);

               common::transaction::global::ID gtrid{ trid};

               auto recovery_rollbacked = local::recover({ gtrid}, Directive::rollback);

               ASSERT_TRUE( ! recovery_rollbacked.empty());

               EXPECT_TRUE( recovery_rollbacked.front() == gtrid);
            }
            {
               // verify that message is redelivered once
               auto messages = unittest::messages( "a1");

               ASSERT_TRUE( messages.size() == 1);
               EXPECT_TRUE( messages.front().redelivered == 1);
            }

            // rollback simulated uncommited dequeue, otherwise casual-queue-group has ongoing transactions
            EXPECT_EQ( common::transaction::context().rollback(), common::code::tx::ok);
         }

     }


      TEST( casual_queue, enqueue_5___clear_queue___expect_0_message_in_queue)
      {
         common::unittest::Trace trace;

         auto domain = local::domain();

         const auto payload = common::unittest::random::binary( 64);
         {
            queue::Message message;
            message.payload.type = common::buffer::type::binary;
            message.payload.data.assign( std::begin( payload), std::end( payload));

            common::algorithm::for_n< 5>( [&]()
            {
               queue::enqueue( "a1", message);
            });
         }

         auto messages = unittest::messages( "a1");
         EXPECT_TRUE( messages.size() == 5);

         auto cleared = queue::clear::queue( { "a1"});
         ASSERT_TRUE( cleared.size() == 1);
         EXPECT_TRUE( cleared.at( 0).queue == "a1");
         EXPECT_TRUE( cleared.at( 0).count == 5);

         EXPECT_TRUE( unittest::messages( "a1").empty());
      }

      TEST( casual_queue, configure_with_enviornment_variables)
      {
         common::unittest::Trace trace;

         constexpr auto configuration = R"(
domain: 
   name: queue-environment

   environment:
      variables:
         -  key: Q_GROUP_PREFIX
            value: "group."
         -  key: Q_BASE
            value: ":memory:"
         -  key: Q_PREFIX
            value: "casual."
         -  key: Q_POSTFIX
            value: ".queue"

   queue:
      groups:
         -  alias: "${Q_GROUP_PREFIX}A"
            queuebase: "${Q_BASE}"
            queues:
               - name: ${Q_PREFIX}a1${Q_POSTFIX}
               - name: ${Q_PREFIX}a2${Q_POSTFIX}
               - name: ${Q_PREFIX}a3${Q_POSTFIX}

)";


         auto domain = local::domain( configuration);

         auto state = unittest::state();

         ASSERT_TRUE( state.groups.size() == 1) << CASUAL_NAMED_VALUE( state);
         auto& group = state.groups.at( 0);
         EXPECT_TRUE( group.alias == "group.A") << CASUAL_NAMED_VALUE( group);
         EXPECT_TRUE( group.queuebase == ":memory:");
         
         auto order_queue = []( auto& l, auto& r){ return l.name < r.name;};
         common::algorithm::sort( state.queues, order_queue);
         ASSERT_TRUE( state.queues.size() == 6) << CASUAL_NAMED_VALUE( state.queues);

         EXPECT_TRUE( state.queues.at( 0).name == "casual.a1.queue");
         EXPECT_TRUE( state.queues.at( 1).name == "casual.a1.queue.error");
         EXPECT_TRUE( state.queues.at( 2).name == "casual.a2.queue");
         EXPECT_TRUE( state.queues.at( 3).name == "casual.a2.queue.error");
         EXPECT_TRUE( state.queues.at( 4).name == "casual.a3.queue");
         EXPECT_TRUE( state.queues.at( 5).name == "casual.a3.queue.error");

      }

      TEST( casual_queue, shutdown_during_non_blocking_queue_lookup__expect_shutdown_not_consumed)
      {
         common::unittest::Trace trace;

         auto domain = local::domain();

         auto& inbound = common::communication::ipc::inbound::device();

         // prepare the fake shutdown 
         inbound.push( common::message::shutdown::Request{ common::process::handle()});

         queue::Message message;
         {
            message.payload.type = common::buffer::type::binary;
            message.payload.data = common::unittest::random::binary( 43);
         }

         auto id = queue::enqueue( "a1", message);
         EXPECT_TRUE( id) << "id: " << id;

         {
            common::message::shutdown::Request message;
            EXPECT_TRUE( common::communication::device::blocking::receive( inbound, message));
         }

      }

      TEST( casual_queue, shutdown_during_blocking_dequeue__expect_shutdown_not_consumed)
      {
         common::unittest::Trace trace;

         auto domain = local::domain();

         auto& inbound = common::communication::ipc::inbound::device();

         // prepare the fake shutdown 
         inbound.push( common::message::shutdown::Request{ common::process::handle()});

         EXPECT_CODE( 
         {  
            (void)queue::blocking::dequeue( "a1");
         }, common::code::queue::no_message);

         // expect shutdown to 'stay'
         {
            common::message::shutdown::Request message;
            EXPECT_TRUE( common::communication::device::blocking::receive( inbound, message));
         }
      }

      TEST( casual_queue, enqueue_message__shutdown_during_blocking_dequeue___expect__message_and_shutdown_not_consumed)
      {
         common::unittest::Trace trace;

         auto domain = local::domain();

         {
            queue::Message message;
            {
               message.payload.type = common::buffer::type::binary;
               message.payload.data = common::unittest::random::binary( 43);
            }
            queue::enqueue( "a1", message);
         }

         auto& inbound = common::communication::ipc::inbound::device();

         // prepare the fake shutdown 
         inbound.push( common::message::shutdown::Request{ common::process::handle()});

         EXPECT_NO_THROW(
         {  
            (void)queue::blocking::dequeue( "a1");
         });

         // expect shutdown to 'stay'
         {
            common::message::shutdown::Request message;
            EXPECT_TRUE( common::communication::device::blocking::receive( inbound, message));
         }
      }

      namespace local
      {
         namespace
         {
            namespace zombie
            {
               constexpr auto configuration_with_queue_B = R"(
domain: 
   queue:
      groups:
         -  alias: "A"
            queuebase: ".casual/zombies.db"

            queues:
               - name: queue_A
               - name: queue_B
)";

               constexpr auto configuration_without_queue_B = R"(
domain: 
   queue:
      groups:
         -  alias: "A"
            queuebase: ".casual/zombies.db"
            queues:
               - name: queue_A
)";

               constexpr auto database{".casual/zombies.db"};
            } // zombie
         } // <unnamed>
      } // local

      TEST( casual_queue, detect_zombie_queue__remove_empty_queues__no_zombies)
      {
         common::unittest::Trace trace;
         common::file::scoped::Path database{ local::zombie::database};

         // sink child signals 
         common::signal::callback::registration< common::code::signal::child>( [](){});

         {
            auto domain = local::domain( local::zombie::configuration_with_queue_B);
            auto state = unittest::state();

            EXPECT_TRUE( state.queues.size() == 2 * 2) << "state.queues.size(): " << state.queues.size();
            EXPECT_TRUE( state.zombies.size() == 0) << "state.zombies.size(): " << state.zombies.size();
         }

         {
            auto domain = local::domain( local::zombie::configuration_without_queue_B);
            auto state = unittest::state();

            EXPECT_TRUE( state.queues.size() == 1 * 2) << "state.queues.size(): " << state.queues.size();
            EXPECT_TRUE( state.zombies.size() == 0) << "state.zombies.size(): " << state.zombies.size();
         }
      }

      TEST( casual_queue, detect_zombie_queue__add_message__cannot_remove_queue__1_zombie_queue)
      {
         common::unittest::Trace trace;
         common::file::scoped::Path database{ local::zombie::database};

         // sink child signals 
         common::signal::callback::registration< common::code::signal::child>( [](){});

         {
            auto domain = local::domain( local::zombie::configuration_with_queue_B);
            auto state = unittest::state();

            EXPECT_TRUE( state.queues.size() == 2 * 2) << "state.queues.size(): " << state.queues.size();
            EXPECT_TRUE( state.zombies.size() == 0) << "state.zombies.size(): " << state.zombies.size();

            queue::Message message;

            queue::enqueue( "queue_B", message);
            auto messages = unittest::messages( "queue_B");

            EXPECT_TRUE( messages.size() == 1);
         }

         {
            auto domain = local::domain( local::zombie::configuration_without_queue_B);
            auto state = unittest::state();

            EXPECT_TRUE( state.queues.size() == 1 * 2) << "state.queues.size(): " << state.queues.size();
            EXPECT_TRUE( state.zombies.size() == 1 * 2) << "state.zombies.size(): " << state.zombies.size();
 
            {
               // Send request
               ipc::message::lookup::Request request;
               request.process = common::process::handle();
               // queue_B should not be advertised now because it is a zombie
               request.name = "queue_B";

               common::communication::device::blocking::send( 
                  common::communication::instance::outbound::queue::manager::device(), 
                  request);

               ipc::message::lookup::Reply reply;
               common::communication::device::blocking::receive( common::communication::ipc::inbound::device(), reply);

               EXPECT_FALSE( reply.queue) << CASUAL_NAMED_VALUE( reply);
            }
         }
      }

      TEST( casual_queue, enqueue_10__browse__expect_all_10)
      {
         common::unittest::Trace trace;

         constexpr auto number_of_messages = 10;

         auto domain = local::domain();

         common::algorithm::for_n< number_of_messages>( []( auto index)
         {
            queue::Message message;
            message.payload.type = common::buffer::type::binary;
            message.payload.data = common::unittest::random::binary( ( index * 10) + 10);

            ASSERT_TRUE( queue::enqueue( "b1", message));
         });

         {
            platform::size::type count = 0;
            queue::browse::peek( "b1", [ &count]( auto&& message)
            {
               EXPECT_TRUE( common::range::size( message.payload.data) == ( 10 * count++) + 10);
               return true;
            });

            EXPECT_TRUE( count == number_of_messages) << CASUAL_NAMED_VALUE( count);
         }
      }
      
      TEST( casual_queue, enqueue_10_available_in_the_future__browse__expect_none)
      {
         common::unittest::Trace trace;

         constexpr auto number_of_messages = 10;

         auto domain = local::domain();

         common::algorithm::for_n< number_of_messages>( []( auto index)
         {
            queue::Message message;
            message.payload.type = common::buffer::type::binary;
            message.payload.data = common::unittest::random::binary( ( index * 10) + 10);
            message.attributes.available = platform::time::clock::type::now() + std::chrono::hours{ 5};

            ASSERT_TRUE( queue::enqueue( "b1", message));
         });

         {
            platform::size::type count = 0;
            queue::browse::peek( "b1", [ &count]( auto&& message)
            {
               EXPECT_TRUE( common::range::size( message.payload.data) == ( 10 * count++) + 10);
               return true;
            });

            EXPECT_TRUE( count == 0) << CASUAL_NAMED_VALUE( count);
         }
      }

      TEST( casual_queue, enqueue_20_10_available_in_the_future__browse__expect_10)
      {
         common::unittest::Trace trace;

         constexpr auto number_of_messages = 10;

         auto domain = local::domain();

         common::algorithm::for_n< number_of_messages>( []( auto index)
         {
            queue::Message message;
            message.payload.type = common::buffer::type::binary;
            message.payload.data = common::unittest::random::binary( ( index * 10) + 10);

            ASSERT_TRUE( queue::enqueue( "b1", message));

            // this message will not be available for browse::peek
            message.attributes.available = platform::time::clock::type::now() + std::chrono::hours{ 5};
            ASSERT_TRUE( queue::enqueue( "b1", message));
         });

         {
            platform::size::type count = 0;
            queue::browse::peek( "b1", [ &count]( auto&& message)
            {
               EXPECT_TRUE( common::range::size( message.payload.data) == ( 10 * count++) + 10);
               return true;
            });

            EXPECT_TRUE( count == number_of_messages) << CASUAL_NAMED_VALUE( count);
         }
      }

      TEST( casual_queue, enqueue_queuebase_out_of_memory__expect_error)
      {
         common::unittest::Trace trace;

         auto directory = common::unittest::directory::temporary::Scoped{};

         // some space is needed for queue-group initialization
         auto content = common::unittest::file::content( directory.path() / "a.pre.statements", R"(
PRAGMA max_page_count = 25;
PRAGMA page_size = 512;
)");

         auto scope = common::unittest::environment::scoped::variable( "CASUAL_UNITTEST_QUEUEBASE", ( directory.path() / "a.qb").string());

         constexpr auto config = R"(
domain: 
   queue:
      groups:
         -  alias: "A"
            queuebase: ${CASUAL_UNITTEST_QUEUEBASE}
            queues:
               - name: a1
)";

         auto domain = local::domain( config);

         queue::Message message;
         message.payload.type = common::buffer::type::binary;
         message.payload.data = common::unittest::random::binary( 512);

         EXPECT_THROW( common::algorithm::for_n( 2000, [ &message]()
         {
            queue::enqueue( "a1", message);
         }), std::system_error);
      }

      TEST( casual_queue, dequeue_fails__expect_error_reply)
      {
         common::unittest::Trace trace;

         auto domain = local::domain();

         auto find_queuegroup = []
         {
            ipc::message::lookup::Request request{ common::process::handle()};
            request.name = "a1";

            common::communication::device::blocking::send( 
               common::communication::instance::outbound::queue::manager::device(), 
               request);

            auto reply = common::communication::ipc::receive< ipc::message::lookup::Reply>();
            return reply.process.ipc;
         };

         // We cause an error by requesting a nonexistent queue
         ipc::message::group::dequeue::Request request{ common::process::handle()};
         request.name = "i_dont_exist";

         common::communication::device::blocking::send( find_queuegroup(), request);

         auto reply = common::communication::ipc::receive< ipc::message::group::dequeue::Reply>();
         EXPECT_TRUE( ! reply.message);
         EXPECT_TRUE( reply.code == decltype( reply.code)::no_queue) << CASUAL_NAMED_VALUE( reply);
      }


      TEST( casual_queue, enqueue_dequeue_enable)
      {
         common::unittest::Trace trace;

         auto a = local::domain( R"(
domain:
   name: A
   queue:
      groups:
         -  queues:
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

)");

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

      namespace local
      {
         namespace
         {
            namespace capacity
            {
               auto domain()
               {
                  constexpr auto config = R"(
domain: 
   queue:
      groups:
         -  alias: "A"
            queuebase: ${CASUAL_UNITTEST_QUEUEBASE}
            capacity: "100B"
            queues:
               - name: a1
)";
                  return local::domain( config);
               }

               auto message( platform::size::type size)
               {
                  queue::Message message;
                  message.payload.type = common::buffer::type::binary;
                  message.payload.data = common::unittest::random::binary( size);
                  return message;
               }

               std::optional< platform::size::type> group_size( std::string_view group)
               {
                  auto state = unittest::state();
                  if( ! state.groups.empty())
                     return state.groups.at( 0).size;
                  return std::nullopt;
               }
            } // capacity
         } // unnammed
      } // local

      TEST( casual_queue, enqueue_dequeue_messages__expect_correct_group_size)
      {
         common::unittest::Trace trace;

         auto domain = local::capacity::domain();

         queue::enqueue( "a1", local::capacity::message( 30));
         queue::enqueue( "a1", local::capacity::message( 30));

         {
            auto current_size = local::capacity::group_size( "A");
            ASSERT_TRUE( current_size);
            EXPECT_TRUE( current_size.value() == 60);
         }
         
         {
            EXPECT_FALSE( queue::dequeue( "a1").empty());
            auto current_size = local::capacity::group_size( "A");
            ASSERT_TRUE( current_size);
            EXPECT_TRUE( current_size.value() == 30);
         }
      }

      TEST( casual_queue, enqueue_message__insufficient_capacity__expect_error)
      {
         common::unittest::Trace trace;

         auto domain = local::capacity::domain();

         EXPECT_THROW({
            queue::enqueue( "a1", local::capacity::message( 101));
         }, std::system_error);
      }

      TEST( casual_queue, dequeue_message__full_group__expect_success)
      {
         common::unittest::Trace trace;

         auto domain = local::capacity::domain();

         queue::enqueue( "a1", local::capacity::message( 100));
         
         EXPECT_FALSE( queue::dequeue( "a1").empty());
      }

      TEST( casual_queue, restart_domain__expect_correct_group_size)
      {
         common::unittest::Trace trace;

         auto directory = common::unittest::directory::temporary::Scoped{};
         auto scope = common::unittest::environment::scoped::variable( "CASUAL_UNITTEST_QUEUEBASE", ( directory.path() / "a.qb").string());

         {
            auto domain = local::capacity::domain();

            queue::enqueue( "a1", local::capacity::message( 24));
            queue::enqueue( "a1", local::capacity::message( 24));
         }

         {
            auto domain = local::capacity::domain();

            auto current_size = local::capacity::group_size( "A");
            ASSERT_TRUE( current_size);
            EXPECT_TRUE( current_size.value() == 48);
         }
      }

      TEST( casual_queue, enqueue_message_transaction__commit__expect_correct_group_size)
      {
         common::unittest::Trace trace;

         auto domain = local::capacity::domain();

         EXPECT_EQ( common::transaction::context().begin(), common::code::tx::ok);
         queue::enqueue( "a1", local::capacity::message( 30));

         {
            auto current_size = local::capacity::group_size( "A");
            ASSERT_TRUE( current_size);
            EXPECT_TRUE( current_size.value() == 30);
         }

         EXPECT_EQ( common::transaction::context().commit(), common::code::tx::ok);

         {
            auto current_size = local::capacity::group_size( "A");
            ASSERT_TRUE( current_size);
            EXPECT_TRUE( current_size.value() == 30);
         }
      }

      TEST( casual_queue, dequeue_message_transaction__commit__expect_correct_group_size)
      {
         common::unittest::Trace trace;

         auto domain = local::capacity::domain();

         queue::enqueue( "a1", local::capacity::message( 30));

         EXPECT_EQ( common::transaction::context().begin(), common::code::tx::ok);

         // dequeue not committed - message should not be subtracted from size
         {
            EXPECT_FALSE( queue::dequeue( "a1").empty());
            auto current_size = local::capacity::group_size( "A");
            ASSERT_TRUE( current_size);
            EXPECT_TRUE( current_size.value() == 30);
         }

         EXPECT_EQ( common::transaction::context().commit(), common::code::tx::ok);

         // dequeue rollbacked - expect message subtracted from size
         {
            auto current_size = local::capacity::group_size( "A");
            ASSERT_TRUE( current_size);
            EXPECT_TRUE( current_size.value() == 0);
         }
      }

      TEST( casual_queue, enqueue_message_transaction__rollback__expect_correct_group_size)
      {
         common::unittest::Trace trace;

         auto domain = local::capacity::domain();

         EXPECT_EQ( common::transaction::context().begin(), common::code::tx::ok);
         queue::enqueue( "a1", local::capacity::message( 30));

         {
            auto current_size = local::capacity::group_size( "A");
            ASSERT_TRUE( current_size);
            EXPECT_TRUE( current_size.value() == 30);
         }

         EXPECT_EQ( common::transaction::context().rollback(), common::code::tx::ok);

         {
            auto current_size = local::capacity::group_size( "A");
            ASSERT_TRUE( current_size);
            EXPECT_TRUE( current_size.value() == 0);
         }
      }

      TEST( casual_queue, dequeue_message_transaction__rollback__expect_correct_group_size)
      {
         common::unittest::Trace trace;

         auto domain = local::capacity::domain();

         queue::enqueue( "a1", local::capacity::message( 30));

         EXPECT_EQ( common::transaction::context().begin(), common::code::tx::ok);

         // dequeue not committed - message should not be subtracted from size
         {
            EXPECT_FALSE( queue::dequeue( "a1").empty());
            auto current_size = local::capacity::group_size( "A");
            ASSERT_TRUE( current_size);
            EXPECT_TRUE( current_size.value() == 30);
         }

         EXPECT_EQ( common::transaction::context().rollback(), common::code::tx::ok);

         // dequeue rollbacked - expect message still included in size
         {
            auto current_size = local::capacity::group_size( "A");
            ASSERT_TRUE( current_size);
            EXPECT_TRUE( current_size.value() == 30);
         }
      }
   } // queue
} // casual

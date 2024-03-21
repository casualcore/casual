//! 
//! Copyright (c) 2015, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!

#include "common/unittest.h"
#include "common/unittest/file.h"

#include "gateway/unittest/utility.h"

#include "gateway/manager/admin/model.h"
#include "gateway/manager/admin/server.h"
#include "gateway/message.h"

#include "common/environment.h"
#include "common/service/lookup.h"
#include "common/communication/instance.h"
#include "common/event/listen.h"
#include "common/message/event.h"

#include "common/message/domain.h"
#include "common/algorithm/is.h"
#include "common/result.h"
#include "common/sink.h"

#include "serviceframework/service/protocol/call.h"

#include "domain/unittest/manager.h"
#include "domain/unittest/configuration.h"
#include "domain/unittest/discover.h"
#include "domain/unittest/utility.h"
#include "domain/discovery/api.h"

#include "service/unittest/utility.h"

#include "queue/api/queue.h"

#include "configuration/model/load.h"
#include "configuration/model/transform.h"

namespace casual
{
   using namespace common;
   namespace gateway
   {

      namespace local
      {
         namespace
         {
            namespace configuration
            {
               constexpr auto servers = R"(
domain: 

   groups: 
      - name: base
      - name: user
        dependencies: [ base]
      - name: gateway
        dependencies: [ user]
   
   servers:
      - path: ${CASUAL_MAKE_SOURCE_ROOT}/middleware/service/bin/casual-service-manager
        memberships: [ base]
      - path: ${CASUAL_MAKE_SOURCE_ROOT}/middleware/transaction/bin/casual-transaction-manager
        memberships: [ base]
      - path: bin/casual-gateway-manager
        memberships: [ gateway]
)";

               constexpr auto inbound = R"(
domain: 
   name: inbound
   gateway:
      inbound:
         groups:
            -  connections: 
                  -  address: 127.0.0.1:6669
)";
               constexpr auto outbound =  R"(
domain: 
   name: outbound
   gateway:
      outbound:
         groups:
            -  connections: 
                  -  address: 127.0.0.1:6669
)";

               constexpr auto gateway = R"(
domain: 
   name: gateway-domain

   gateway:
      inbound:
         groups:
            -  alias: inbound-1
               note: xxx
               connections: 
                  -  address: 127.0.0.1:6669
                     note: yyy
      outbound:
         groups: 
            -  alias: outbound-1
               connections:
                  -  address: 127.0.0.1:6669
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
               return domain( configuration::gateway);
            }

         } // <unnamed>
      } // local



      TEST( gateway_manager, boot_shutdown)
      {
         common::unittest::Trace trace;

         EXPECT_NO_THROW( {
            auto domain = local::domain(); 
         });
      }

      TEST( gateway_manager, configuration_get)
      {
         common::unittest::Trace trace;

         auto domain = local::domain();

         // wait for the 'bounds' to be connected...
         unittest::fetch::until( unittest::fetch::predicate::outbound::connected());

         auto origin = local::configuration::load( local::configuration::servers, local::configuration::gateway);

         auto model = casual::configuration::model::transform( casual::domain::unittest::configuration::get());

         EXPECT_TRUE( origin.gateway == model.gateway) << CASUAL_NAMED_VALUE( origin.gateway) << "\n " << CASUAL_NAMED_VALUE( model.gateway);

      }

      TEST( gateway_manager, configuration_post)
      {
         common::unittest::Trace trace;

         auto domain = local::domain( R"(
domain: 
   name: post
   gateway:
      inbound:
         groups:
            -  alias: inbound-1
               note: xxx
               connections: 
                  -  address: 127.0.0.1:6669
                     note: yyy
      outbound:
         groups: 
            -  alias: outbound-1
               connections:
                  -  address: 127.0.0.1:6669
)");

         // wait for the 'bounds' to be connected...
         unittest::fetch::until( unittest::fetch::predicate::outbound::connected());

         auto wanted = local::configuration::load( local::configuration::servers, R"(
domain: 
   name: post
   gateway:
      inbound:
         groups:
            -  alias: A
               note: yyy
               connections: 
                  -  address: 127.0.0.1:7001
                     note: yyy
      outbound:
         groups: 
            -  alias: B
               connections:
                  -  address: 127.0.0.1:7001
)");

         // make sure the wanted differs (otherwise we're not testing anything...)
         ASSERT_TRUE( wanted.gateway != casual::configuration::model::transform( casual::domain::unittest::configuration::get()).gateway);

         // post the wanted model (with transformed user representation)
         casual::domain::unittest::configuration::post( casual::configuration::model::transform( wanted));
         
         // wait for the 'bounds' to be connected...
         unittest::fetch::until( unittest::fetch::predicate::outbound::connected());

         auto updated = casual::configuration::model::transform( casual::domain::unittest::configuration::get());

         EXPECT_TRUE( wanted.gateway == updated.gateway) << CASUAL_NAMED_VALUE( wanted.gateway) << '\n' << CASUAL_NAMED_VALUE( updated.gateway);

      }

      TEST( gateway_manager, listen_on_127_0_0_1__6666__outbound__127_0_0_1__6666__expect_connection)
      {
         common::unittest::Trace trace;

         auto domain = local::domain(); 

         EXPECT_TRUE( common::communication::instance::fetch::handle( common::communication::instance::identity::gateway::manager.id));

         auto state = unittest::fetch::until( unittest::fetch::predicate::outbound::connected());

         EXPECT_TRUE( state.connections.size() == 2);
      }

      TEST( gateway_manager, outbound_groups_3___expect_order)
      {
         common::unittest::Trace trace;

         constexpr auto configuration = R"(
domain:
   name: A
   gateway:
      inbound:
         groups:
            -  connections:
                  -  address: 127.0.0.1:7001
      outbound:
         groups:
            -  connections:
                  -  address: 127.0.0.1:7001
            -  connections:
                  -  address: 127.0.0.1:7001
            -  connections:
                  -  address: 127.0.0.1:7001

)";

         auto domain = local::domain( configuration);

         EXPECT_TRUE( common::communication::instance::fetch::handle( common::communication::instance::identity::gateway::manager.id));

         auto state = unittest::fetch::until( unittest::fetch::predicate::outbound::connected());
         
         auto& groups = state.outbound.groups;

         EXPECT_TRUE( groups.size() == 3);

         auto order_less = []( auto& l, auto& r){ return l.order < r.order;};
         auto order_equal = []( auto& l, auto& r){ return l.order == r.order;};

         auto unique = algorithm::unique( algorithm::sort( groups, order_less), order_equal);

         EXPECT_TRUE( unique.size() == 3);

      }

      namespace local
      {
         namespace
         {
            namespace service
            {
               void echo() 
               {
                  common::message::service::call::callee::Request request;
                  communication::device::blocking::receive( communication::ipc::inbound::device(), request);
                  
                  auto reply = common::message::reverse::type( request);
                  reply.buffer = std::move( request.buffer);
                  reply.transaction.trid = std::move( request.trid);

                  // emulate some sort of work..
                  process::sleep( std::chrono::milliseconds{ 1});

                  communication::device::blocking::send( request.process.ipc, reply);

                  casual::service::unittest::send::ack( request);
               };
            } // service
         } // <unnamed>
      } // local
      
      TEST( gateway_manager, remote1_call__expect_service_remote1)
      {
         common::unittest::Trace trace;

         auto b = local::domain( R"(
domain: 
   name: B
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

         unittest::fetch::until( unittest::fetch::predicate::outbound::connected());


         b.activate();

         // we exposes service a
         casual::service::unittest::advertise( { "b"});

         a.activate();

         const auto data = common::unittest::random::binary( 128);

         // Expect us to reach service "b" via outbound -> inbound -> <service remote1>
         // we act as the server
         {
            auto correlation = common::unittest::service::send( "b", data);

            b.activate();

            // echo the call
            local::service::echo();

            a.activate();

            auto reply = common::communication::ipc::receive< common::message::service::call::Reply>( correlation);

            EXPECT_TRUE( reply.buffer.data == data) << CASUAL_NAMED_VALUE( reply);
         }
      }

      TEST( gateway_manager, advertise_b_to_B__call_b_via_A__simulate_A_get_the_same_call_message_again__loop_between_gateways__expect_error_reply)
      {
         common::unittest::Trace trace;

         auto b = local::domain( R"(
domain:
   name: B
   gateway:
      inbound:
         groups:
            -  alias: inbound
               connections:
                  -  address: 127.0.0.1:7010
)");

         // we expose service b, in domain B
         casual::service::unittest::advertise( { "b"});

         auto a = local::domain( R"(
domain:
   name: A
   gateway:
      outbound:
         groups:
            -  alias: outbound
               connections:
                  -  address: 127.0.0.1:7010
)");

         const auto a_state = unittest::fetch::until( unittest::fetch::predicate::outbound::connected());

         auto outbound_connection_ipc = a_state.connections.at( 0).ipc;

         ASSERT_TRUE( outbound_connection_ipc);

         const auto data = unittest::random::binary( 256);

         const auto correlation = unittest::service::send( "b", data);

         auto loop_device = communication::ipc::inbound::Device{};

         trace.line( "receive the call and send call to outbound to simulate a _gateway loop_");
         auto request = communication::ipc::receive< common::message::service::call::callee::Request>( correlation);
         EXPECT_TRUE( request.buffer.data == data);

         trace.line( "emulate loop - later we should receive an error reply");
         {
            auto loop_request = request;
            loop_request.process = common::process::Handle{ common::process::id(), loop_device.connector().handle().ipc()};
            communication::device::blocking::send( outbound_connection_ipc, loop_request);
         }
         
         trace.line( "make sure outbound did not _terminate_");
         {
            auto current = unittest::fetch::until( unittest::fetch::predicate::outbound::connected( 1)).outbound.groups.at( 0).process;
            ASSERT_TRUE( a_state.outbound.groups.at( 0).process == current.pid);
         }
         
         trace.line( "receive the error reply");
         {
            auto reply = unittest::service::receive< common::message::service::call::Reply>( loop_device, correlation);
            EXPECT_TRUE( reply.code.result == decltype( reply.code.result)::system) << CASUAL_NAMED_VALUE( reply.code);
         }

         trace.line( "reply to the request");
         {
            auto reply = common::message::reverse::type( request);
            reply.buffer = std::move( request.buffer);
            communication::device::blocking::send( request.process.ipc, reply);
         }

         trace.line( "receive the real reply");
         {
            auto reply = communication::ipc::receive< common::message::service::call::Reply>( correlation);
            EXPECT_TRUE( reply.code.result == decltype( reply.code.result)::ok);
            EXPECT_TRUE( reply.buffer.data == data);

         }
      }

      TEST( gateway_manager, remote1_call_in_transaction___expect_same_transaction_in_reply)
      {
         common::unittest::Trace trace;

         auto b = local::domain( local::configuration::inbound);
         auto a = local::domain( local::configuration::outbound);

         auto a_state = unittest::fetch::until( unittest::fetch::predicate::outbound::connected());

         b.activate();

         // we exposes service "a"
         casual::service::unittest::advertise( { "b"});

         a.activate();

      
         const auto trid = common::transaction::id::create( common::process::handle());

         // Expect us to reach service "b" via outbound -> inbound -> <service b>
         {
            const auto data = common::unittest::random::binary( 128);

            const auto correlation = unittest::service::send( "b", data, trid);
            
            b.activate();

            // echo the call
            local::service::echo();

            a.activate();

            auto reply = common::communication::ipc::receive< common::message::service::call::Reply>( correlation);

            EXPECT_TRUE( reply.buffer.data == data);
            EXPECT_TRUE( reply.transaction.trid == trid) << "reply.transaction.trid: " << reply.transaction.trid << "\ntrid: " << trid;
         }

         // send commit
         {
            common::message::transaction::commit::Request request{ common::process::handle()};
            request.trid = trid;
            communication::device::blocking::send( communication::instance::outbound::transaction::manager::device(), request);
         }

         // get commit reply
         {
            auto reply = common::communication::ipc::receive< common::message::transaction::commit::Reply>();
            EXPECT_TRUE( reply.trid == trid) << CASUAL_NAMED_VALUE( reply.trid) << "\n" << CASUAL_NAMED_VALUE( trid);
         }
      }


      TEST( gateway_manager,  enqueue_dequeue___expect_message)
      {
         common::unittest::Trace trace;

         constexpr auto queue_manager = R"(
domain: 
   servers:
      - path: ${CASUAL_MAKE_SOURCE_ROOT}/middleware/queue/bin/casual-queue-manager
        memberships: [ base]
)";
         constexpr auto queue_configuration = R"(
domain:
   queue:
      groups:
         -  name: A
            queuebase: ":memory:"
            queues:
               - name: a
)";

         auto b = local::domain( local::configuration::inbound, queue_manager, queue_configuration);
         auto a = local::domain( local::configuration::outbound, queue_manager);

         unittest::fetch::until( unittest::fetch::predicate::outbound::connected());

         const auto payload = unittest::random::binary( 1000);

         // enqueue
         {
            EXPECT_NO_THROW({
               queue::enqueue( "a", { { "json", payload}});
            });
         }

         // dequeue
         {
            auto message = queue::dequeue( "a");

            ASSERT_TRUE( message.size() == 1);
            
            EXPECT_TRUE( message.front().payload.data == payload);
            EXPECT_TRUE( message.front().payload.type == "json");
         }
      }

      TEST( gateway_manager,  enqueue_dequeue___restart_queuemanager_expect_message)
      {
         common::unittest::Trace trace;

         constexpr auto queue_manager = R"(
domain:
   servers:
      - path: ${CASUAL_MAKE_SOURCE_ROOT}/middleware/queue/bin/casual-queue-manager
        memberships: [ base]
        restart: true
)";
         constexpr auto queue_configuration = R"(
domain:
   queue:
      groups:
         -  name: A
            queuebase: ":memory:"
            queues:
               - name: a
)";

         auto b = local::domain( local::configuration::inbound, queue_manager, queue_configuration);
         auto a = local::domain( local::configuration::outbound, queue_manager);

         auto qm = common::communication::instance::fetch::handle( common::communication::instance::identity::queue::manager.id);

         unittest::fetch::until( unittest::fetch::predicate::outbound::connected());

         const auto payload = unittest::random::binary( 1000);

         // enqueue
         {
            EXPECT_NO_THROW({
               queue::enqueue( "a", { { "json", payload}});
            });
         }

         // dequeue
         {
            auto message = queue::dequeue( "a");

            ASSERT_TRUE( message.size() == 1);

            EXPECT_TRUE( message.front().payload.data == payload);
            EXPECT_TRUE( message.front().payload.type == "json");
         }

         // send signal, queue-manager terminates, wait for queue-manager to spawn again (via event::Spawn)
        {
            bool done = false;
            event::listen(
               event::condition::compose(
                  event::condition::prelude( [pid = qm.pid]()
                  {
                     EXPECT_TRUE( signal::send( pid, code::signal::terminate));
                  }),
                  event::condition::done( [&done]()
                  {
                     return done;
                  })
               ),
               [&done]( const common::message::event::process::Spawn& event)
               {
                  log::line( log::debug, "event: ", event);
                  done = event.alias == "casual-queue-manager";
               });
         }

         // because of some unknown factor, the queue-manager needs some time to get ready
         // to handle calls. Strange but is a separate issue.
         process::sleep( std::chrono::milliseconds{ 500});

         // enqueue
         {
            EXPECT_NO_THROW({
               queue::enqueue( "a", { { "json", payload}});
            });
         }

         // dequeue
         {
            auto message = queue::dequeue( "a");

            ASSERT_TRUE( message.size() == 1);

            EXPECT_TRUE( message.front().payload.data == payload);
            EXPECT_TRUE( message.front().payload.type == "json");
         }
      }

      TEST( gateway_manager, kill_outbound__expect_restart)
      {
         common::unittest::Trace trace;

         auto b = local::domain( local::configuration::inbound);
         auto a = local::domain( local::configuration::outbound);

         auto state = unittest::fetch::until( unittest::fetch::predicate::outbound::connected());

         auto& outbound = state.connections.at( 0);

         // send signal, outbound terminates, wait for outbound to spawn again (via event::Spawn)
         {
            bool done = false;
            event::listen( 
               event::condition::compose( 
                  event::condition::prelude( [pid = outbound.process.pid]()
                  {
                     EXPECT_TRUE( signal::send( pid, code::signal::terminate));
                  }),
                  event::condition::done( [&done]()
                  {
                     return done;
                  })
               ),
               [&done]( const common::message::event::process::Spawn& event)
               {
                  log::line( log::debug, "event: ", event);
                  // we're done if outbound spawns
                  done = event.alias == "outbound";
               });
         }
      }

      TEST( gateway_manager, connect_10_outbound_connections)
      {
         common::unittest::Trace trace;

         constexpr auto configuration = R"(
domain: 
   name: multi-connections
   gateway:
      outbound:
         groups: 
            -  connections:
               -  address: 127.0.0.1:6669
               -  address: 127.0.0.1:6669
               -  address: 127.0.0.1:6669
               -  address: 127.0.0.1:6669
               -  address: 127.0.0.1:6669
               -  address: 127.0.0.1:6669
               -  address: 127.0.0.1:6669
               -  address: 127.0.0.1:6669
               -  address: 127.0.0.1:6669
               -  address: 127.0.0.1:6669
)";

         auto b = local::domain( local::configuration::inbound);
         auto a = local::domain( configuration);

         auto state = unittest::fetch::until( unittest::fetch::predicate::outbound::connected( 10));

         using Bound = manager::admin::model::connection::Bound;

         auto bound_count = [&state]( Bound bound)
         {
            return algorithm::accumulate( state.connections, platform::size::type{}, [bound]( auto count, auto& connection)
            {
               if( connection.bound == bound)
                  return count + 1;
               return count;
            });
         };

         EXPECT_TRUE( bound_count( Bound::out) == 10) << "count: " << bound_count( Bound::out) << '\n' << CASUAL_NAMED_VALUE( state);

         b.activate();
         state = unittest::state();

         EXPECT_TRUE( bound_count( Bound::in) == 10) << "count: " << bound_count( Bound::in) << '\n' << CASUAL_NAMED_VALUE( state);
      }

      TEST( gateway_manager, connect__10_outbound_groups__one_connection_each)
      {
         common::unittest::Trace trace;

         constexpr auto configuration = R"(
domain: 
   name: A
   gateway:
      outbound:
         groups: 
            -  connections:
               - address: 127.0.0.1:6669
            -  connections:
               - address: 127.0.0.1:6669
            -  connections:
               - address: 127.0.0.1:6669
            -  connections:
               - address: 127.0.0.1:6669
            -  connections:
               - address: 127.0.0.1:6669
            -  connections:
               - address: 127.0.0.1:6669
            -  connections:
               - address: 127.0.0.1:6669
            -  connections:
               - address: 127.0.0.1:6669
            -  connections:
               - address: 127.0.0.1:6669
            -  connections:
               - address: 127.0.0.1:6669
)";


         auto b = local::domain( local::configuration::inbound);
         auto a = local::domain( configuration);

         auto state = unittest::fetch::until( unittest::fetch::predicate::outbound::connected( 10));

         auto count_bound = []( auto bound)
         {
            return [bound]( auto count, auto& connection)
            {
               if( connection.bound == bound)
                  ++count;
               return count;
            };
         };

         using Bound = manager::admin::model::connection::Bound;

         EXPECT_TRUE( algorithm::accumulate( state.connections, platform::size::type{}, count_bound( Bound::out)) == 10);

         b.activate();
         state = unittest::state();

         EXPECT_TRUE( algorithm::accumulate( state.connections, platform::size::type{}, count_bound( Bound::in)) == 10);
      }

      TEST( gateway_manager, kill_inbound__expect_restart)
      {
         common::unittest::Trace trace;

         auto b = local::domain( local::configuration::inbound);
         auto a = local::domain( local::configuration::outbound);

         unittest::fetch::until( unittest::fetch::predicate::outbound::connected());

         b.activate();
         auto state = unittest::state();
         auto& inbound = state.connections.at( 0);


         // send signal, inbound terminates, wait for inbound and outbound to spawn again (via event::Spawn)
         {
            auto done = false;
            event::listen( 
               event::condition::compose( 
                  event::condition::prelude( [pid = inbound.process.pid]()
                  {
                     EXPECT_TRUE( signal::send( pid, code::signal::terminate));
                  }),
                  event::condition::done( [&done]()
                  {
                     return done;
                  })
               ),
               [&done]( const common::message::event::process::Spawn& event)
               {
                  log::line( log::debug, "event: ", event);

                  done = event.alias == "inbound";
               });
         }
      }

      TEST( gateway_manager, outbound_to_non_existent_inbound)
      {
         common::unittest::Trace trace;

         constexpr auto configuration = R"(
domain: 
   name: gateway-domain

   gateway:
      outbound:
         groups:
            -  connections:
               -  address: 127.0.0.1:6666
)";


         auto domain = local::domain( configuration);

         auto state = unittest::state();
         ASSERT_TRUE( state.connections.size() == 1);
         auto& outbound = state.connections.at( 0);
         auto process = common::communication::instance::fetch::handle( outbound.process.pid);
         EXPECT_TRUE( communication::instance::ping( process.ipc) == process);

      }



      TEST( gateway_manager, rediscover_to_not_connected_outbounds)
      {
         common::unittest::Trace trace;

         constexpr auto configuration = R"(
domain: 
   name: gateway-domain

   gateway:
      outbound:
         groups: 
            -  connections:
               -  address: 127.0.0.1:6666
               -  address: 127.0.0.1:6667
)";


         auto domain = local::domain( configuration);

         auto state = unittest::fetch::until( []( const auto& state){ return state.connections.size() >= 2;});
         ASSERT_TRUE( state.connections.size() == 2);

         EXPECT_NO_THROW(
            communication::ipc::receive< casual::domain::discovery::rediscovery::Reply>( casual::domain::discovery::rediscovery::request());
         );
      }

      TEST( gateway_manager, rediscover__1_outbound_not_connected)
      {
         common::unittest::Trace trace;

         constexpr auto configuration = R"(
domain: 
   name: A
   gateway:
      outbound:
         groups:
            -  connections:
               -  address: 127.0.0.1:6669
               -  address: 127.0.0.1:6670
)";

         auto b = local::domain( local::configuration::inbound);
         auto a = local::domain( configuration);

         auto state = unittest::fetch::until(  unittest::fetch::predicate::outbound::connected( 1));
         EXPECT_TRUE( state.connections.size() == 2);

         EXPECT_NO_THROW(
            communication::ipc::receive< casual::domain::discovery::rediscovery::Reply>( casual::domain::discovery::rediscovery::request());
         );
      }

      TEST( gateway_manager, async_5_remote1_call__expect_pending_metric)
      {
         common::unittest::Trace trace;

         constexpr auto count = 5;
         
         auto b = local::domain( R"(
domain: 
   name: B
   gateway:
      inbound:
         groups:
            -  connections:
               -  address: 127.0.0.1:6669
)");

         // we exposes service "a"
         casual::service::unittest::advertise( { "a"});

         auto a = local::domain( R"(
domain: 
   name: A
   gateway:
      outbound:
         groups:
            -  connections:
               -  address: 127.0.0.1:6669
)");

         auto state = unittest::fetch::until( unittest::fetch::predicate::outbound::connected());

         auto outbound = [ &state]() -> strong::ipc::id
         {            
            if( auto found = algorithm::find_if( state.connections, unittest::fetch::predicate::is::outbound()))
               return found->ipc;

            return {};
         }();

         ASSERT_TRUE( outbound);


         // subscribe to metric events
         event::subscribe( common::process::handle(), { common::message::event::service::Calls::type()});

         // Expect us to reach service remote1 via outbound -> inbound -> <service remote1>
         // we act as the server
         {
            const auto data = common::unittest::random::binary( 128);

            
            auto correlations = algorithm::generate_n< count>( [ &data]()
            {
               return common::unittest::service::send( "a", data);
            });

            b.activate();

            algorithm::for_n< count>( []()
            {
               // echo the call
               local::service::echo();
            });

            a.activate();

            for( auto& correlation : correlations)
            {
               auto reply = common::communication::ipc::receive< common::message::service::call::Reply>( correlation);
               EXPECT_TRUE( reply.buffer.data ==  data);
            }
         }

         // consume the metric events (most likely from inbound cache)
         {
            std::vector< common::message::event::service::Metric> metrics;

            while( metrics.size() < count)
            {
               auto filter_metric = []( auto& metric){ return metric.service == "a";};

               auto event = common::communication::ipc::receive< common::message::event::service::Calls>();

               // filter out only metrics for service 'a' (could come metrics for .casual/gateway/state)
               algorithm::container::append( algorithm::filter( event.metrics, filter_metric), metrics);
            }

            auto order_pending = []( auto& lhs, auto& rhs){ return lhs.pending < rhs.pending;};
            algorithm::sort( metrics, order_pending);

            ASSERT_TRUE( metrics.size() == count) << CASUAL_NAMED_VALUE( metrics);
            // all should be 0 pending
            EXPECT_TRUE(( algorithm::all_of( metrics, []( auto& metric){ return metric.pending == platform::time::unit::zero();})));
         }

      }

      TEST( gateway_manager_connect, inbound__native_tcp_connect_disconnect_10_times___expect_still_listening)
      {
         common::unittest::Trace trace;

         auto b = local::domain( R"(
domain: 
   name: B
   gateway:
      inbound:
         groups:
            -  connections: 
                  -  address: 127.0.0.1:7010
)");

         // make sure the connection listen.
         auto state = unittest::fetch::until( unittest::fetch::predicate::listeners( 1));

         common::algorithm::for_n< 10>( []()
         {
            auto socket = communication::tcp::connect( communication::tcp::Address{ "127.0.0.1:7010"});
            ASSERT_TRUE( socket);
         });

         auto a = local::domain( R"(
domain: 
   name: A
   gateway:
      outbound:
         groups:
            -  connections: 
                  -  address: 127.0.0.1:7010
)");

         unittest::fetch::until( unittest::fetch::predicate::outbound::connected());

      }

      TEST( gateway_manager_connect, reverse_outbound__native_tcp_connect_disconnect_10_times___expect_still_listening)
      {
         common::unittest::Trace trace;

         auto a = local::domain( R"(
domain: 
   name: A
   gateway:
      reverse:
         outbound:
            groups:
               -  connections: 
                     -  address: 127.0.0.1:7010
)");
         // make sure the connection listen.
         unittest::fetch::until( unittest::fetch::predicate::listeners( 1));

         common::algorithm::for_n< 10>( []()
         {
            auto socket = communication::tcp::connect( communication::tcp::Address{ "127.0.0.1:7010"});
            ASSERT_TRUE( socket);
         });

         auto b = local::domain( R"(
domain: 
   name: B
   gateway:
      reverse:
         inbound:
            groups:
               -  connections: 
                     -  address: 127.0.0.1:7010
)");

         a.activate();
         unittest::fetch::until( unittest::fetch::predicate::outbound::connected());

      }

      TEST( gateway_manager_connect, native_tcp_listen_to_outbound__accept_and_disconnect_10_times___expect_still_trying_to_connect)
      {
         common::unittest::Trace trace;

         auto a = local::domain( R"(
domain: 
   name: A
   gateway:
      outbound:
         groups:
            -  connections: 
                  -  address: 127.0.0.1:7010
)");

         // 10 bad listeners
         common::algorithm::for_n< 10>( []()
         {
            communication::tcp::Listener listener{ communication::tcp::Address{ "127.0.0.1:7010"}};
            ASSERT_TRUE( listener());
         });

         auto b = local::domain( R"(
domain: 
   name: B
   gateway:
      inbound:
         groups:
            -  connections: 
                  -  address: 127.0.0.1:7010
)");

         a.activate();
         unittest::fetch::until( unittest::fetch::predicate::outbound::connected());

      }

      TEST( gateway_manager_connect, native_tcp_listen_to_reverse_inbound__accept_and_disconnect_10_times___expect_still_trying_to_connect)
      {
         common::unittest::Trace trace;

         auto b = local::domain( R"(
domain: 
   name: B
   gateway:
      reverse:
         inbound:
            groups:
               -  connections: 
                     -  address: 127.0.0.1:7010
)");

         // 10 bad listeners
         common::algorithm::for_n< 10>( []()
         {
            communication::tcp::Listener listener{ communication::tcp::Address{ "127.0.0.1:7010"}};
            ASSERT_TRUE( listener());
         });

         auto a = local::domain( R"(
domain: 
   name: A
   gateway:
      reverse:
         outbound:
            groups:
               -  connections: 
                     -  address: 127.0.0.1:7010
)");


         unittest::fetch::until( unittest::fetch::predicate::outbound::connected());

      }

      namespace local
      {
         namespace
         {
            constexpr auto is_connecting = []( auto count)
            {
               return [count]( auto& state)
               {
                  return algorithm::count_if( state.connections,
                        unittest::fetch::predicate::is::runlevel::connecting()) == count;
               };
            };

            constexpr auto is_failed_listeners = []( auto count)
            {
               return [count]( auto& state)
               {
                  return algorithm::count_if( state.listeners, unittest::fetch::predicate::is::runlevel::failed()) == count;
               };
            };

         } // <unnamed>
      } // local


      TEST( gateway_manager_outbound_connect, outbound_non_existent_address__expect_retry)
      {
         common::unittest::Trace trace;

         constexpr auto outbound = R"(
domain: 
   name: outbound
   gateway:
      outbound:
         groups:
            -  connections: 
                  -  address: non.existent.local:70001
)";

         auto a = local::domain( outbound);

         unittest::fetch::until( local::is_connecting( 1));
      }

      TEST( gateway_manager_outbound_connect, reverse_inbound_non_existent_address__expect_runlevel_connecting)
      {
         common::unittest::Trace trace;

         constexpr auto reverse_inbound = R"(
domain: 
   name: reverse_inbound
   gateway:
      reverse:
         inbound:
            groups:
               -  connections: 
                     -  address: non.existent.local:70001
)";

         auto a = local::domain( reverse_inbound);

         unittest::fetch::until( local::is_connecting( 1));
      }


      TEST( gateway_manager_inbound_listen, non_existent_address__expect_fail)
      {
         common::unittest::Trace trace;

         constexpr auto inbound = R"(
domain: 
   name: inbound
   gateway:
      inbound:
         groups:
            -  connections: 
                  -  address: non.existent.local:70001
)";

         auto a = local::domain( inbound);

         unittest::fetch::until( local::is_failed_listeners( 1));
      }

      TEST( gateway_manager_outbound, reverse___non_existent_address__expect_fail)
      {
         common::unittest::Trace trace;

         constexpr auto reverse_outbound = R"(
domain: 
   name: reverse_outbound
   gateway:
      reverse:
         outbound:
            groups:
               -  connections: 
                     -  address: non.existent.local:70001
)";

         auto a = local::domain( reverse_outbound);

         unittest::fetch::until( local::is_failed_listeners( 1));
      }

      TEST( gateway_manager_inbound, address_in_use__expect_fail)
      {
         common::unittest::Trace trace;
         
         constexpr auto inbound = R"(
domain: 
   name: inbound
   gateway:
      inbound:
         groups:
            -  connections: 
                  -  address: 127.0.0.1:7010
                  -  address: 127.0.0.1:7010
)";

         auto a = local::domain( inbound);

         unittest::fetch::until( local::is_failed_listeners( 1));
      }

      TEST( gateway_manager, A_B__pending_call_A_to_B__shutdown_B__send_implicit_update_to_A_outbound__expect_direct_reply__not_over_to_inbound)
      {
         common::unittest::Trace trace;
         
         auto b = local::domain( R"(
domain:
   name: B
   gateway:
      inbound:
         groups:
            -  alias: in-b
               connections:
                  -  address: 127.0.0.1:7010
)");
         
         // this unittest advertise service b and x
         casual::service::unittest::advertise( { "b"});
         casual::service::unittest::advertise( { "x"});

         auto a = local::domain( R"(
domain:
   name: A
   gateway:
      outbound:
         groups:
            -  alias: out-a
               connections:
                  -  address: 127.0.0.1:7010
)");

         const auto outbound = unittest::outbound::group( unittest::fetch::until( unittest::fetch::predicate::outbound::connected()), "out-a").value();
      
         const auto payload = common::unittest::random::binary( 128);
         const auto correlation = common::unittest::service::send( "b", payload);

         b.activate();

         const auto inbound = unittest::inbound::group( unittest::state(), "in-b").value();

         // now we've got a in-flight call to b. We emulate a shutdown in B by sending
         // shutdown directly to inbound
         {
            common::message::shutdown::Request shutdown{ common::process::handle()};
            common::communication::device::blocking::send( inbound.process.ipc, shutdown);
         }

         a.activate();

         // send direct::Explore to outbound, outbound should not send discovery to
         // B since B is in disconnect mode.
         {
            casual::domain::message::discovery::topology::direct::Explore request;
            request.content.services = { "b", "x", "y"};
            request.domains = algorithm::transform( unittest::state().connections, []( auto& connection)
            {
               return connection.remote;
            });

            EXPECT_TRUE( common::communication::device::non::blocking::send( outbound.process.ipc, request));
         }

         b.activate();

         // verify that B has not got any (more) discoveries. We need to verify the absent of 
         // a discovery request. Sadly, we need to loop a while.
         algorithm::for_n< 5>( [ &inbound]()
         {
            common::process::sleep( std::chrono::milliseconds{ 1});

            // we expect the inbound to have received 1 discovery_request for the call to "b", it should not 
            // get any more.
            auto reply = communication::ipc::call( inbound.process.ipc, common::message::counter::Request{ common::process::handle()});
            if( auto found = algorithm::find( reply.entries, common::message::Type::domain_discovery_request))
            {
               EXPECT_TRUE( found->received == 1) << CASUAL_NAMED_VALUE( *found);
            }
         });
         
         // act a server and reply
         {
            EXPECT_TRUE( casual::service::unittest::server::echo( correlation));
         }

         // for good measure, receive the reply
         {
            a.activate();
            EXPECT_TRUE( payload == common::unittest::service::receive( correlation));
         }

      }

      TEST( gateway_manager_outbound, transaction_resource_error_reply_on_remote_connection_lost___expect_TX_FAIL)
      {
         common::unittest::Trace trace;

         // Sink child signals
         signal::callback::registration< code::signal::child>( [](){});

         auto b = local::domain( R"(
domain: 
   name: B
   groups:
      -  name: user
         dependencies: [ base]
   
   servers:
      - path: ${CASUAL_MAKE_SOURCE_ROOT}/middleware/example/server/bin/casual-example-server
        memberships: [ user]

   gateway:
      inbound:
         groups:
            -  connections: 
                  -  address: 127.0.0.1:7012
)");

         // get a handle for b:s inbound
         auto inbound = unittest::fetch::until( unittest::fetch::predicate::listeners( 1)).inbound.groups.at( 0).process;
         ASSERT_TRUE( inbound);

         auto a = local::domain( R"(
domain: 
   name: A
   gateway:
      outbound:
         groups:
            -  connections:
                  -  address: 127.0.0.1:7012
)");

         unittest::fetch::until( unittest::fetch::predicate::outbound::connected( 1));
         
         // call service in B in transaction
         {
            using namespace std::literals;

            EXPECT_EQ( transaction::context().begin(), code::tx::ok);

            buffer::Payload payload;
            payload.type = "X_OCTET/";
            common::algorithm::copy( "casual"sv, std::back_inserter( payload.data));

            auto result = common::service::call::context().sync( "casual/example/domain/echo/B", common::buffer::payload::Send{ payload}, {});

            EXPECT_TRUE( result.buffer.data == payload.data);

         }

         // We shutdown b "hard"
         signal::send( inbound.pid, code::signal::terminate);

         // We expect hazard.
         EXPECT_EQ( transaction::context().commit(), code::tx::fail);
      }

      // TODO the pending call hack has been removed and need to be replaced with some sort of _pending timeout_
      TEST( gateway_manager_inbound, DISABLED_A_B_kill_A_rollback_during_pending_call__expect_TPESVCERR_no_pending)
      {
         common::unittest::Trace trace;

         constexpr std::string_view system = R"(
system:
   resources:
      -  key: rm-mockup
         server: "${CASUAL_MAKE_SOURCE_ROOT}/middleware/transaction/bin/rm-proxy-casual-mockup"
         xa_struct_name: casual_mockup_xa_switch_static
         libraries:
            -  casual-mockup-rm
)";

         auto b = local::domain( system, R"(
domain:
   name: B
   transaction:
      resources:
         -  name: example-resource-server
            key: rm-mockup
            instances: 1
   servers:
   -  path: ${CASUAL_MAKE_SOURCE_ROOT}/middleware/example/server/bin/casual-example-resource-server
      arguments: [ --nested-calls, x ]
      memberships: [ user]
   gateway:
      inbound:
         groups:
            -  connections: 
                  -  address: 127.0.0.1:7001
)");


         // we advertise service a
         casual::service::unittest::advertise( { "x"});

         auto a = local::domain( system, R"(
domain:
   name: A
   transaction:
      resources:
         -  name: example-resource-server
            key: rm-mockup
            instances: 1
   servers:
      -  alias: a-forward
         path: ${CASUAL_MAKE_SOURCE_ROOT}/middleware/example/server/bin/casual-example-resource-server
         arguments: [ --nested-calls, casual/example/resource/nested/calls/B, casual/example/resource/nested/calls/B]
         memberships: [ user]
   gateway:
      outbound:
         groups:
            -  connections: 
                  -  address: 127.0.0.1:7001
)");

         unittest::fetch::until( unittest::fetch::predicate::outbound::connected());

         auto call_service = []( auto service)
         {
            buffer::Payload payload;
            payload.type = "X_OCTET/";
            payload.data = common::unittest::random::binary( 512);

            return common::service::call::context().async( service, common::buffer::payload::Send{ payload}, {});
         };

         [[maybe_unused]] auto send_reply = []( auto& request)
         {
            auto reply = common::message::reverse::type( request);
            reply.transaction.trid = request.trid;
            reply.code.result = decltype( reply.code.result)::ok;
            reply.buffer = request.buffer;

            EXPECT_TRUE( request.service.name == "x");

            // ack to SM
            casual::service::unittest::send::ack( request);
            common::communication::device::blocking::send( request.process.ipc, reply);
         };

         auto receive_reply = []()
         {
            auto reply = common::communication::ipc::receive< common::message::service::call::Reply>();
            return reply.code.result;
         };

         // get the pid of the A-forward
         auto a_forward = casual::domain::unittest::server( casual::domain::unittest::state(), "a-forward");
         EXPECT_TRUE( a_forward);

         // The following is the flow:
         // * call nested/calls/B via nested/calls/A  (two calls to nested/calls/B)
         // * first on will be in progress, since we will not reply ( from x)
         // * second will be pending in B inbound
         // * kill a-forward -> TPESVCERR 
         // * TM issue rollback
         // * B inbound just replies TPETIME (to the died process) and clean up
         // * We reply from x to nested/calls/B -> B inbound, ack to B SM
         // * B get a lookup::Reply from B SM -> inbound does not have any pending any more (replied during rollback) -> lookup::discard to B SM.
         // * We just call domain/echo/B to ensure that example-server instance in B is not reserved (lookup discard)

         call_service( "casual/example/resource/nested/calls/A");

         // receive the first request
         auto request = common::communication::ipc::receive< common::message::service::call::callee::Request>();

         // kill the one who has started the transaction
         common::signal::send( a_forward.pid, common::code::signal::kill);

         // we should get a service-error from our call to casual/example/resource/nested/calls/A
         EXPECT_EQ( receive_reply(), common::code::xatmi::service_error);
         
         // switch to B
         b.activate();
         // reply to the only call that got through, the other one was pending and inbound replied to A 
         // when the rollback from A's TM arrived (the call was pending)
         send_reply( request);

         // call casual/example/resource/domain/echo//B to make sure instance reservation was released
         {
            a.activate();

            call_service( "casual/example/resource/domain/echo/B");
            EXPECT_EQ( receive_reply(), common::code::xatmi::ok);
         }
      }

   } // gateway

} // casual

//! 
//! Copyright (c) 2015, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!

#include "common/unittest.h"

#include "gateway/manager/admin/model.h"
#include "gateway/manager/admin/server.h"


#include "common/environment.h"
#include "common/service/lookup.h"
#include "common/communication/instance.h"
#include "common/event/listen.h"

#include "common/message/domain.h"
#include "common/message/queue.h"
#include "common/algorithm/is.h"

#include "serviceframework/service/protocol/call.h"

#include "domain/manager/unittest/process.h"
#include "service/unittest/advertise.h"

namespace casual
{
   using namespace common;
   namespace gateway
   {

      namespace local
      {
         namespace
         {
            struct Domain
            {
               
               Domain( std::string configuration) : domain{ { std::move( configuration)}} 
               {
               }

               Domain() : Domain( Domain::configuration) {}

               casual::domain::manager::unittest::Process domain;

               static constexpr auto configuration = R"(
domain: 
   name: gateway-domain

   groups: 
      - name: base
      - name: gateway
        dependencies: [ base]
   
   servers:
      - path: "${CASUAL_HOME}/bin/casual-service-manager"
        memberships: [ base]
      - path: "${CASUAL_HOME}/bin/casual-transaction-manager"
        memberships: [ base]
      - path: "./bin/casual-gateway-manager"
        memberships: [ gateway]
   gateway:
      listeners: 
         - address: 127.0.0.1:6669
      connections:
         - address: 127.0.0.1:6669
)";

            };



            namespace call
            {

               auto state()
               {
                  serviceframework::service::protocol::binary::Call call;
                  auto reply = call( manager::admin::service::name::state);

                  manager::admin::model::State result;
                  reply >> CASUAL_NAMED_VALUE( result);

                  return result;
               }

               auto rediscover()
               {
                  serviceframework::service::protocol::binary::Call call;
                  auto reply = call( manager::admin::service::name::rediscover);

                  Uuid result;
                  reply >> CASUAL_NAMED_VALUE( result);

                  return result;
               }


               namespace wait
               {
                  namespace ready
                  {
                     manager::admin::model::State state()
                     {
                        auto state = local::call::state();

                        auto count = 100;

                        auto ready = []( auto& state)
                        {
                           if( state.connections.empty())
                              return false;

                           return algorithm::all_of( state.connections, []( auto& connection){
                              return connection.runlevel >= manager::admin::model::Connection::Runlevel::online &&
                                 connection.process == communication::instance::ping( connection.process.ipc);
                           });
                        };

                        while( ! ready( state) && count-- > 0)
                        {
                           common::process::sleep( std::chrono::milliseconds{ 10});
                           state = local::call::state();
                        }

                        return state;
                     }

                  } // ready
               } // wait
            } // call
         } // <unnamed>
      } // local



      TEST( gateway_manager_tcp, listen_on_127_0_0_1__6666)
      {
         common::unittest::Trace trace;

         EXPECT_NO_THROW( {
            local::Domain domain;
         });
      }

      TEST( gateway_manager_tcp, listen_on_127_0_0_1__6666__outbound__127_0_0_1__6666__expect_connection)
      {
         common::unittest::Trace trace;

         local::Domain domain;

         EXPECT_TRUE( common::communication::instance::fetch::handle( common::communication::instance::identity::gateway::manager));

         auto state = local::call::wait::ready::state();

         EXPECT_TRUE( state.connections.size() == 2);
         EXPECT_TRUE( algorithm::any_of( state.connections, []( const manager::admin::model::Connection& c){
            return c.bound == manager::admin::model::Connection::Bound::out;
         }));
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
                  
                  auto reply = message::reverse::type( request);
                  reply.buffer = std::move( request.buffer);
                  reply.transaction.trid = std::move( request.trid);

                  // emulate some sort of work..
                  process::sleep( std::chrono::milliseconds{ 1});

                  communication::device::blocking::send( request.process.ipc, reply);

                  {
                     common::message::service::call::ACK ack;
                     ack.metric.process = process::handle();
                     ack.metric.trid = reply.transaction.trid;
                     ack.metric.pending = request.pending;
                     ack.metric.start = platform::time::clock::type::now();
                     ack.metric.end = platform::time::clock::type::now();
                     ack.metric.service = request.service.name;

                     communication::device::blocking::send( communication::instance::outbound::service::manager::device(), ack);
                  }
               };
            } // service
         } // <unnamed>
      } // local
      
      TEST( gateway_manager_tcp, connect_to_our_self__remote1_call__expect_service_remote1)
      {
         common::unittest::Trace trace;

         
         local::Domain domain;

         // we exposes service "remote1"
         casual::service::unittest::advertise( { "remote1"});

         EXPECT_TRUE( common::communication::instance::fetch::handle( common::communication::instance::identity::gateway::manager));

         auto state = local::call::wait::ready::state();

         ASSERT_TRUE( state.connections.size() == 2);

         algorithm::sort( state.connections);

         auto data = common::unittest::random::binary( 128);

         // Expect us to reach service remote1 via outbound -> inbound -> <service remote1>
         // we act as the server
         {
            {
               common::message::service::call::callee::Request request;
               request.process = common::process::handle();
               request.service.name = "remote1";
               request.buffer.memory = data;
               
               common::communication::device::blocking::send( state.connections.at( 0).process.ipc, request);
            }

            // echo the call
            local::service::echo();

            common::message::service::call::Reply reply;
            common::communication::device::blocking::receive( common::communication::ipc::inbound::device(), reply);

            EXPECT_TRUE( reply.buffer.memory ==  data);
         }
      }



      TEST( gateway_manager_tcp, connect_to_our_self__remote1_call_in_transaction___expect_same_transaction_in_reply)
      {
         common::unittest::Trace trace;

         local::Domain domain;

         // we exposes service "remote1"
         casual::service::unittest::advertise( { "remote1"});

         EXPECT_TRUE( common::communication::instance::fetch::handle( common::communication::instance::identity::gateway::manager));

         auto state = local::call::wait::ready::state();

         ASSERT_TRUE( state.connections.size() == 2);

         algorithm::sort( state.connections);

         auto data = common::unittest::random::binary( 128);

         auto trid = common::transaction::id::create( common::process::handle());

         // Expect us to reach service remote1 via outbound -> inbound -> <service remote1>
         {   
            {
               common::message::service::call::callee::Request request;
               request.process = common::process::handle();
               request.service.name = "remote1";
               request.trid = trid;
               request.buffer.memory = data;
               
               common::communication::device::blocking::send( state.connections.at( 0).process.ipc, request);
            }

            // echo the call
            local::service::echo();

            common::message::service::call::Reply reply;
            common::communication::device::blocking::receive( common::communication::ipc::inbound::device(), reply);

            EXPECT_TRUE( reply.buffer.memory == data);
            EXPECT_TRUE( reply.transaction.trid == trid)  << "reply.transaction.trid: " << reply.transaction.trid << "\ntrid: " << trid;
         }

         // send prepare
         {
            common::message::transaction::resource::prepare::Request request;
            request.trid = trid;
            request.process = common::process::handle();
            request.resource = common::strong::resource::id{ 42};
            common::communication::device::blocking::send( state.connections.at( 0).process.ipc, request);
         }

         // get prepare reply
         {
            common::message::transaction::resource::prepare::Reply reply;
            common::communication::device::blocking::receive( common::communication::ipc::inbound::device(), reply);
            EXPECT_TRUE( reply.trid == trid);
         }
      }


      TEST( gateway_manager_tcp,  connect_to_our_self__enqueue_dequeue___expect_message)
      {
         common::unittest::Trace trace;

         static constexpr auto configuration = R"(
domain: 
   name: gateway-domain

   groups: 
      - name: base
      - name: gateway
        dependencies: [ base]
   
   servers:
      - path: "${CASUAL_HOME}/bin/casual-service-manager"
        memberships: [ base]
      - path: "${CASUAL_HOME}/bin/casual-transaction-manager"
        memberships: [ base]
      - path: "${CASUAL_HOME}/bin/casual-queue-manager"
        memberships: [ base]
      - path: "./bin/casual-gateway-manager"
        memberships: [ gateway]
   gateway:
      listeners: 
         - address: 127.0.0.1:6666
      connections:
         - address: 127.0.0.1:6666
   queue:
      groups:
         - name: groupA
           queuebase: ":memory:"
           queues:
            - name: queue1
)";


         local::Domain domain{ configuration};

         EXPECT_TRUE( common::communication::instance::fetch::handle( common::communication::instance::identity::gateway::manager));

         auto state = local::call::wait::ready::state();
         ASSERT_TRUE( state.connections.size() == 2);
         algorithm::sort( state.connections);

         // Gateway is connected to it self. Hence we can send a request to the outbound, and it
         // will send it to the corresponding inbound, and back in the current (mockup) domain

         ASSERT_TRUE( state.connections.at( 0).bound == manager::admin::model::Connection::Bound::out);
         auto outbound =  state.connections.at( 0).process;

         const auto payload = unittest::random::binary( 1000);

         // enqueue
         {
            message::queue::enqueue::Request request{ process::handle()};
            request.name = "queue1";
            request.message.type = "json";
            request.message.payload = payload;


            auto reply = communication::ipc::call( outbound.ipc, request);
            EXPECT_TRUE( ! reply.id.empty());
         }

         // dequeue
         {
            message::queue::dequeue::Request request{ process::handle()};
            request.name = "queue1";

            auto reply = communication::ipc::call( outbound.ipc, request);
            ASSERT_TRUE( ! reply.message.empty());
            EXPECT_TRUE( reply.message.front().payload == payload);
            EXPECT_TRUE( reply.message.front().type == "json");
         }
      }

      TEST( gateway_manager_tcp, connect_to_our_self__kill_outbound__expect_restart)
      {
         common::unittest::Trace trace;

         local::Domain domain;

         EXPECT_TRUE( common::communication::instance::fetch::handle( common::communication::instance::identity::gateway::manager));

         auto state = local::call::wait::ready::state();

         ASSERT_TRUE( state.connections.size() == 2);

         algorithm::sort( state.connections);
         auto& outbound = state.connections.at( 0);

         EXPECT_TRUE( common::communication::instance::fetch::handle( outbound.process.pid) == outbound.process);

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
                  done = event.alias == "casual-gateway-outbound";
               });
         }
      }

      TEST( gateway_manager_tcp, connect_to_our_self__10_outbound_connections)
      {
         common::unittest::Trace trace;

         static constexpr auto configuration = R"(
domain: 
   name: gateway-domain

   groups: 
      - name: base
      - name: gateway
        dependencies: [ base]
   
   servers:
      - path: "${CASUAL_HOME}/bin/casual-service-manager"
        memberships: [ base]
      - path: "${CASUAL_HOME}/bin/casual-transaction-manager"
        memberships: [ base]
      - path: "./bin/casual-gateway-manager"
        memberships: [ gateway]
   gateway:
      listeners: 
         - address: 127.0.0.1:6666
      connections:
         - address: 127.0.0.1:6666
         - address: 127.0.0.1:6666
         - address: 127.0.0.1:6666
         - address: 127.0.0.1:6666
         - address: 127.0.0.1:6666
         - address: 127.0.0.1:6666
         - address: 127.0.0.1:6666
         - address: 127.0.0.1:6666
         - address: 127.0.0.1:6666
         - address: 127.0.0.1:6666
)";


         local::Domain domain{ configuration};

         EXPECT_TRUE( common::communication::instance::fetch::handle( common::communication::instance::identity::gateway::manager));

         auto state = local::call::wait::ready::state();

         EXPECT_TRUE( state.connections.size() == 2 * 10) << state.connections;
      }

      TEST( gateway_manager_tcp, connect_to_our_self__kill_inbound__expect_restart)
      {
         common::unittest::Trace trace;

         local::Domain domain;

         EXPECT_TRUE( common::communication::instance::fetch::handle( common::communication::instance::identity::gateway::manager)); 

         auto state = local::call::wait::ready::state();

         ASSERT_TRUE( state.connections.size() == 2);

         algorithm::sort( state.connections);
         auto& inbound = state.connections.at( 1);

         EXPECT_TRUE( common::communication::instance::fetch::handle( inbound.process.pid) == inbound.process);

         // send signal, inbound terminates, wait for inbound and outbound to spawn again (via event::Spawn)
         {
            auto done = std::make_tuple( false, false);
            event::listen( 
               event::condition::compose( 
                  event::condition::prelude( [pid = inbound.process.pid]()
                  {
                     EXPECT_TRUE( signal::send( pid, code::signal::terminate));
                  }),
                  event::condition::done( [&done]()
                  {
                     return std::get< 0>( done) && std::get< 1>( done);
                  })
               ),
               [&done]( const common::message::event::process::Spawn& event)
               {
                  log::line( log::debug, "event: ", event);

                  if( event.alias == "casual-gateway-inbound")
                     std::get< 0>( done) = true;

                  if( event.alias == "casual-gateway-outbound")
                     std::get< 1>( done) = true;
               });
         }
      }

      TEST( gateway_manager_tcp, outbound_to_non_existent_listener)
      {
         common::unittest::Trace trace;

         static constexpr auto configuration = R"(
domain: 
   name: gateway-domain

   groups: 
      - name: base
      - name: gateway
        dependencies: [ base]
   
   servers:
      - path: "${CASUAL_HOME}/bin/casual-service-manager"
        memberships: [ base]
      - path: "${CASUAL_HOME}/bin/casual-transaction-manager"
        memberships: [ base]
      - path: "./bin/casual-gateway-manager"
        memberships: [ gateway]
   gateway:
      connections:
         - address: 127.0.0.1:6666
)";


         local::Domain domain{ configuration};

         auto state = local::call::state();
         ASSERT_TRUE( state.connections.size() == 1);
         auto& outbound = state.connections.at( 0);
         auto process = common::communication::instance::fetch::handle( outbound.process.pid);
         EXPECT_TRUE( communication::instance::ping( process.ipc) == process);

      }

      namespace local
      {
         namespace
         {
            auto wait_for_outbounds( int count)
            {
               auto number_of_running = []()
               {
                  auto running_outbound = []( auto& connection)
                  {
                     return connection.bound == decltype( connection.bound)::out &&
                        connection.runlevel == decltype( connection.runlevel)::online;
                  };

                  auto state = call::state();

                  return algorithm::count_if( state.connections, running_outbound);
               };

               while( number_of_running() < count)
                  process::sleep( std::chrono::milliseconds{ 2});
            }


            namespace rediscover
            {
               auto listen()
               {
                  Uuid correlation;

                  auto condition = event::condition::compose( 
                     event::condition::prelude( [&correlation]() { correlation = call::rediscover();}),
                     event::condition::done( [&correlation](){ return correlation.empty();}));
                  
                  event::listen( condition, 
                     [&correlation]( const common::message::event::Task& task)
                     {
                        if( task.correlation != correlation)
                           return;

                        common::message::event::terminal::print( std::cout, task);

                        if( task.done())
                           correlation = {};
                     },
                     [&correlation]( const common::message::event::sub::Task& task)
                     {
                        if( task.correlation != correlation)
                           return;
                        common::message::event::terminal::print( std::cout, task);
                     }
                  );
               }
               
            } // rediscover
         } // <unnamed>
      } // local


      TEST( gateway_manager_tcp, rediscover_to_not_connected_outbounds)
      {
         common::unittest::Trace trace;

         static constexpr auto configuration = R"(
domain: 
   name: gateway-domain

   groups: 
      - name: base
      - name: gateway
        dependencies: [ base]
   
   servers:
      - path: "${CASUAL_HOME}/bin/casual-service-manager"
        memberships: [ base]
      - path: "${CASUAL_HOME}/bin/casual-transaction-manager"
        memberships: [ base]
      - path: "./bin/casual-gateway-manager"
        memberships: [ gateway]
   gateway:
      connections:
         - address: 127.0.0.1:6666
         - address: 127.0.0.1:6667
)";


         local::Domain domain{ configuration};

         EXPECT_NO_THROW(
            local::rediscover::listen();
         );
      }

      TEST( gateway_manager_tcp, rediscover_to_1_self_connected__1_not_connected__outbounds)
      {
         common::unittest::Trace trace;

         static constexpr auto configuration = R"(
domain: 
   name: gateway-domain

   groups: 
      - name: base
      - name: gateway
        dependencies: [ base]
   
   servers:
      - path: "${CASUAL_HOME}/bin/casual-service-manager"
        memberships: [ base]
      - path: "${CASUAL_HOME}/bin/casual-transaction-manager"
        memberships: [ base]
      - path: "./bin/casual-gateway-manager"
        memberships: [ gateway]
   gateway:
      listeners: 
         - address: 127.0.0.1:6666
      connections:
         - address: 127.0.0.1:6666
         - address: 127.0.0.1:6667
)";


         local::Domain domain{ configuration};

         local::wait_for_outbounds( 1);

         EXPECT_NO_THROW(
            local::rediscover::listen();
         );
      }

      TEST( gateway_manager_tcp, async_5_remote1_call__expect_pending_metric)
      {
         common::unittest::Trace trace;

         constexpr auto count = 5;
         
         local::Domain domain;

         EXPECT_TRUE( common::communication::instance::fetch::handle( common::communication::instance::identity::gateway::manager)); 

         local::wait_for_outbounds( 1);

         // we exposes service "remote1"
         casual::service::unittest::advertise( { "remote1"});

         auto outbound = []() -> process::Handle
         {
            auto state = local::call::wait::ready::state();

            auto is_outbound = []( auto& connection)
            { 
               return connection.bound == decltype( connection.bound)::out;
            };
            
            if( auto found = algorithm::find_if( state.connections, is_outbound))
               return found->process;

            return {};
         }();

         ASSERT_TRUE( outbound);

         // subscribe to metric events
         event::subscribe( common::process::handle(), { message::event::service::Calls::type()});

         

         // Expect us to reach service remote1 via outbound -> inbound -> <service remote1>
         // we act as the server
         {
            auto data = common::unittest::random::binary( 128);
            

            algorithm::for_n< count>( [&]()
            {
               common::message::service::call::callee::Request request;
               request.process = common::process::handle();
               request.service.name = "remote1";
               request.buffer.memory = data;
               
               ASSERT_TRUE( common::communication::device::blocking::optional::send( outbound.ipc, request)) 
                  << CASUAL_NAMED_VALUE( outbound);
            });

            algorithm::for_n< count>( [&]()
            {
               // echo the call
               local::service::echo();
            });

            algorithm::for_n< count>( [&]()
            {
               common::message::service::call::Reply reply;
               common::communication::device::blocking::receive( common::communication::ipc::inbound::device(), reply);

               EXPECT_TRUE( reply.buffer.memory ==  data);
            });
         }

         // consume the metric events (most likely from inbound cache)
         {
            std::vector< message::event::service::Metric> metrics;

            while( metrics.size() < count)
            {
               message::event::service::Calls event;
               common::communication::device::blocking::receive( common::communication::ipc::inbound::device(), event);
               //EXPECT_TRUE( false) << CASUAL_NAMED_VALUE( event);
               algorithm::append( event.metrics, metrics);
            }

            auto order_pending = []( auto& lhs, auto& rhs){ return lhs.pending < rhs.pending;};
            algorithm::sort( metrics, order_pending);

            ASSERT_TRUE( metrics.size() == count) << CASUAL_NAMED_VALUE( metrics);
            // first should have 0 pending
            EXPECT_TRUE( metrics.front().pending == platform::time::unit::zero());
            // last should have more than 0 pending. could be several with 0 pending
            // since messages buffers over tcp and it's not predictable when messages arrives.
            EXPECT_TRUE( metrics.back().pending > platform::time::unit::zero()) << CASUAL_NAMED_VALUE( metrics);

         }

      }


   } // gateway

} // casual

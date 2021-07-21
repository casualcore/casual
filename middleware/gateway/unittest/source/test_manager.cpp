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
#include "common/environment/scoped.h"
#include "common/service/lookup.h"
#include "common/communication/instance.h"
#include "common/event/listen.h"
#include "common/message/event.h"

#include "common/message/domain.h"
#include "common/algorithm/is.h"
#include "common/result.h"

#include "serviceframework/service/protocol/call.h"

#include "domain/manager/unittest/process.h"
#include "domain/manager/unittest/configuration.h"
#include "domain/discovery/api.h"

#include "service/unittest/utility.h"

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
      - name: gateway
        dependencies: [ base]
   
   servers:
      - path: ${CASUAL_MAKE_SOURCE_ROOT}/middleware/service/bin/casual-service-manager
        memberships: [ base]
      - path: ${CASUAL_MAKE_SOURCE_ROOT}/middleware/transaction/bin/casual-transaction-manager
        memberships: [ base]
      - path: bin/casual-gateway-manager
        memberships: [ gateway]
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
               return casual::domain::manager::unittest::process( configuration::servers, std::forward< C>( configurations)...);
            }

            //! default domain
            auto domain()
            {
               return domain( configuration::gateway);
            }

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

               template< typename P>
               auto state( P&& predicate)
               {
                  auto state = local::call::state();
                  auto count = 500;

                  while( ! predicate( state) && count-- > 0)
                  {
                     common::process::sleep( std::chrono::milliseconds{ 10});
                     state = local::call::state();
                  }

                  return state;
               }

            } // call

            namespace predicate
            {
               namespace is
               {
                  auto outbound()
                  { 
                     return []( auto& connection)
                     {
                        return connection.bound == decltype( connection.bound)::out;
                     };
                  }

               } // is
               namespace outbound
               {
                  auto connected( platform::size::type count)
                  {
                     return [count]( auto& state)
                     {
                        return count <= algorithm::count_if( state.connections, []( auto& connection)
                        {
                           return is::outbound()( connection) && connection.remote.id;
                        });

                     };
                  }

                  auto routing( std::vector< std::string_view> services, std::vector< std::string_view> queues)
                  {
                     return [services = std::move( services), queues = std::move( queues)]( auto& state)
                     {
                        return algorithm::includes( state.services, services)
                           && algorithm::includes( state.queues, queues);
                     };
                  }
               } // outbound
                  
            } // predicate

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

         auto origin = local::configuration::load( local::configuration::servers, local::configuration::gateway);

         auto model = casual::configuration::model::transform( casual::domain::manager::unittest::configuration::get());

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

         // make sure the wanted differs (otherwise we're not testing anyting...)
         ASSERT_TRUE( wanted.gateway != casual::configuration::model::transform( casual::domain::manager::unittest::configuration::get()).gateway);

         // post the wanted model (in transformed user representation)
         auto updated = casual::configuration::model::transform( 
            casual::domain::manager::unittest::configuration::post( casual::configuration::model::transform( wanted)));

         EXPECT_TRUE( wanted.gateway == updated.gateway) << CASUAL_NAMED_VALUE( wanted.gateway) << '\n' << CASUAL_NAMED_VALUE( updated.gateway);

      }

      TEST( gateway_manager, listen_on_127_0_0_1__6666__outbound__127_0_0_1__6666__expect_connection)
      {
         common::unittest::Trace trace;

         auto domain = local::domain(); 

         EXPECT_TRUE( common::communication::instance::fetch::handle( common::communication::instance::identity::gateway::manager.id));

         auto state = local::call::state( local::predicate::outbound::connected( 1));

         EXPECT_TRUE( state.connections.size() == 2);
      }

      TEST( gateway_manager, outbound_connect_non_existent__expect_keep_trying)
      {
         common::unittest::Trace trace;

         constexpr auto configuration = R"(
domain:
   name: A
   gateway:
      outbound:
         groups:
            -  connections:
                  -  address: non.existent.local:70001

)";

         auto domain = local::domain( configuration);

         EXPECT_TRUE( common::communication::instance::fetch::handle( common::communication::instance::identity::gateway::manager.id));

         auto state = local::call::state( []( auto& state)
         {
            return state.connections.size() >= 1;
         });

         ASSERT_TRUE( state.connections.size() == 1) << CASUAL_NAMED_VALUE( state);
         EXPECT_TRUE( state.connections.at( 0).bound == decltype( state.connections.at( 0).bound)::out);
      }

      TEST( gateway_manager, outbound_connect_empty_address__expect_keep_trying)
      {
         common::unittest::Trace trace;

         constexpr auto configuration = R"(
domain:
   name: A
   gateway:
      outbound:
         groups:
            -  connections:
                  -  address: ${non_existent_environment}

)";

         auto domain = local::domain( configuration);

         EXPECT_TRUE( common::communication::instance::fetch::handle( common::communication::instance::identity::gateway::manager.id));

         auto state = local::call::state( []( auto& state)
         {
            return state.connections.size() >= 1;
         });

         ASSERT_TRUE( state.connections.size() == 1) << CASUAL_NAMED_VALUE( state);
         EXPECT_TRUE( state.connections.at( 0).bound == decltype( state.connections.at( 0).bound)::out);
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

         auto state = local::call::state( local::predicate::outbound::connected( 3));
         
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
      
      TEST( gateway_manager, connect_to_our_self__remote1_call__expect_service_remote1)
      {
         common::unittest::Trace trace;

         
         auto domain = local::domain(); 

         {
            auto state = local::call::state( local::predicate::outbound::connected( 1));
            ASSERT_TRUE( state.connections.size() == 2);
         }

         // we exposes service a
         casual::service::unittest::advertise( { "a"});

         // discover to make sure outbound(s) knows about wanted services
         unittest::discover( { "a"}, {});

         // wait until outbound knows about 'a'
         auto state = local::call::state( local::predicate::outbound::routing( { "a"}, {}));

         algorithm::sort( state.connections);

         auto data = common::unittest::random::binary( 128);

         // Expect us to reach service remote1 via outbound -> inbound -> <service remote1>
         // we act as the server
         {
            {
               common::message::service::call::callee::Request request;
               request.process = common::process::handle();
               request.service.name = "a";
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



      TEST( gateway_manager, connect_to_our_self__remote1_call_in_transaction___expect_same_transaction_in_reply)
      {
         common::unittest::Trace trace;

         auto domain = local::domain(); 

         {
            auto state = local::call::state( local::predicate::outbound::connected( 1));
            ASSERT_TRUE( state.connections.size() == 2);
         }

         // we exposes service "a"
         casual::service::unittest::advertise( { "a"});

         // discover to make sure outbound(s) knows about wanted services
         unittest::discover( { "a"}, {});

         // wait until outbound knows about 'a'
         auto state = local::call::state( local::predicate::outbound::routing( { "a"}, {}));

         algorithm::sort( state.connections);

      
         const auto trid = common::transaction::id::create( common::process::handle());

         // Expect us to reach service remote1 via outbound -> inbound -> <service remote1>
         {
            const auto data = common::unittest::random::binary( 128);   
            
            {
               common::message::service::call::callee::Request request;
               request.process = common::process::handle();
               request.service.name = "a";
               request.trid = trid;
               request.buffer.memory = data;
               
               common::communication::device::blocking::send( state.connections.at( 0).process.ipc, request);
            }

            // echo the call
            local::service::echo();

            common::message::service::call::Reply reply;
            common::communication::device::blocking::receive( common::communication::ipc::inbound::device(), reply);

            EXPECT_TRUE( reply.buffer.memory == data);
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
            common::message::transaction::commit::Reply reply;
            communication::device::blocking::receive( common::communication::ipc::inbound::device(), reply);
            EXPECT_TRUE( reply.trid == trid) << CASUAL_NAMED_VALUE( reply.trid) << "\n" << CASUAL_NAMED_VALUE( trid);
         }
      }


      TEST( gateway_manager,  connect_to_our_self__enqueue_dequeue___expect_message)
      {
         common::unittest::Trace trace;

         constexpr auto configuration = R"(
domain: 
   name: queue
   
   servers:
      - path: ${CASUAL_MAKE_SOURCE_ROOT}/middleware/queue/bin/casual-queue-manager
        memberships: [ base]

   queue:
      groups:
         -  name: A
            queuebase: ":memory:"
            queues:
               - name: a
)";


         auto domain = local::domain( local::configuration::gateway, configuration);

         EXPECT_TRUE( common::communication::instance::fetch::handle( common::communication::instance::identity::gateway::manager.id));
         
         {
            auto state = local::call::state( local::predicate::outbound::connected( 1));
         }

         

         // discover to make sure outbound(s) knows about wanted queues
         unittest::discover( { }, { "a"});
         
         // wait for the state
         auto state = local::call::state( local::predicate::outbound::routing( {}, { "a"}));

         algorithm::sort( state.connections);

         // Gateway is connected to it self. Hence we can send a request to the outbound, and it
         // will send it to the corresponding inbound, and back in the current (mockup) domain

         ASSERT_TRUE( state.connections.at( 0).bound == manager::admin::model::connection::Bound::out);
         auto outbound =  state.connections.at( 0).process;

         const auto payload = unittest::random::binary( 1000);

         // enqueue
         {
            casual::queue::ipc::message::group::enqueue::Request request{ process::handle()};
            request.name = "a";
            request.message.type = "json";
            request.message.payload = payload;


            auto reply = communication::ipc::call( outbound.ipc, request);
            EXPECT_TRUE( ! reply.id.empty());
         }

         // dequeue
         {
            casual::queue::ipc::message::group::dequeue::Request request{ process::handle()};
            request.name = "a";

            auto reply = communication::ipc::call( outbound.ipc, request);
            ASSERT_TRUE( ! reply.message.empty());
            EXPECT_TRUE( reply.message.front().payload == payload);
            EXPECT_TRUE( reply.message.front().type == "json");
         }
      }

      TEST( gateway_manager, connect_to_our_self__kill_outbound__expect_restart)
      {
         common::unittest::Trace trace;

         auto domain = local::domain(); 

         EXPECT_TRUE( common::communication::instance::fetch::handle( common::communication::instance::identity::gateway::manager.id));

         auto state = local::call::state( local::predicate::outbound::connected( 1));

         ASSERT_TRUE( state.connections.size() == 2);

         algorithm::sort( state.connections);
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
                  done = event.alias == "outbound-1";
               });
         }
      }

      TEST( gateway_manager, connect_to_our_self__10_outbound_connections)
      {
         common::unittest::Trace trace;

         static constexpr auto configuration = R"(
domain: 
   name: multi-connections
   gateway:
      inbound: 
         groups:
            -  connections: 
               -  address: 127.0.0.1:6666
      outbound:
         groups: 
            -  connections:
               -  address: 127.0.0.1:6666
               -  address: 127.0.0.1:6666
               -  address: 127.0.0.1:6666
               -  address: 127.0.0.1:6666
               -  address: 127.0.0.1:6666
               -  address: 127.0.0.1:6666
               -  address: 127.0.0.1:6666
               -  address: 127.0.0.1:6666
               -  address: 127.0.0.1:6666
               -  address: 127.0.0.1:6666
)";

         auto domain = local::domain( configuration);

         EXPECT_TRUE( common::communication::instance::fetch::handle( common::communication::instance::identity::gateway::manager.id));

         auto state = local::call::state( local::predicate::outbound::connected( 10));

         using Bound = manager::admin::model::connection::Bound;

         auto bound_count = [&state]( Bound bound)
         {
            return algorithm::accumulate( state.connections, platform::size::type{}, [bound]( auto count, auto& connection)
            {
               if( connection.bound == bound)
                  ++count;
               return count;
            });
         };

         EXPECT_TRUE( bound_count( Bound::in) == 10) << "count: " << bound_count( Bound::in) << '\n' << CASUAL_NAMED_VALUE( state);
         EXPECT_TRUE( bound_count( Bound::out) == 10) << "count: " << bound_count( Bound::out) << '\n' << CASUAL_NAMED_VALUE( state);
      }

      TEST( gateway_manager, connect_to_our_self__10_outbound_groups__one_connection_each)
      {
         common::unittest::Trace trace;

         static constexpr auto configuration = R"(
domain: 
   name: gateway-domain

   gateway:
      inbound:
         groups:
            -  connections: 
               -  address: 127.0.0.1:6666
      outbound:
         groups: 
            -  connections:
               - address: 127.0.0.1:6666
            -  connections:
               - address: 127.0.0.1:6666
            -  connections:
               - address: 127.0.0.1:6666
            -  connections:
               - address: 127.0.0.1:6666
            -  connections:
               - address: 127.0.0.1:6666
            -  connections:
               - address: 127.0.0.1:6666
            -  connections:
               - address: 127.0.0.1:6666
            -  connections:
               - address: 127.0.0.1:6666
            -  connections:
               - address: 127.0.0.1:6666
            -  connections:
               - address: 127.0.0.1:6666
)";


         auto domain = local::domain( configuration);

         EXPECT_TRUE( common::communication::instance::fetch::handle( common::communication::instance::identity::gateway::manager.id));

         auto state = local::call::state( local::predicate::outbound::connected( 10));

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

         EXPECT_TRUE( algorithm::accumulate( state.connections, platform::size::type{}, count_bound( Bound::in)) == 10);
         EXPECT_TRUE( algorithm::accumulate( state.connections, platform::size::type{}, count_bound( Bound::out)) == 10);
      }

      TEST( gateway_manager, connect_to_our_self__kill_inbound__expect_restart)
      {
         common::unittest::Trace trace;

         auto domain = local::domain(); 

         EXPECT_TRUE( common::communication::instance::fetch::handle( common::communication::instance::identity::gateway::manager.id)); 

         auto state = local::call::state( local::predicate::outbound::connected( 1));

         ASSERT_TRUE( state.connections.size() == 2);

         algorithm::sort( state.connections);
         auto& inbound = state.connections.at( 1);


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

                  done = event.alias == "inbound-1";
               });
         }
      }

      TEST( gateway_manager, outbound_to_non_existent_inbound)
      {
         common::unittest::Trace trace;

         static constexpr auto configuration = R"(
domain: 
   name: gateway-domain

   gateway:
      outbound:
         groups:
            -  connections:
               -  address: 127.0.0.1:6666
)";


         auto domain = local::domain( configuration);

         auto state = local::call::state();
         ASSERT_TRUE( state.connections.size() == 1);
         auto& outbound = state.connections.at( 0);
         auto process = common::communication::instance::fetch::handle( outbound.process.pid);
         EXPECT_TRUE( communication::instance::ping( process.ipc) == process);

      }



      TEST( gateway_manager, rediscover_to_not_connected_outbounds)
      {
         common::unittest::Trace trace;

         static constexpr auto configuration = R"(
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

         auto state = local::call::state( []( const auto& state){ return state.connections.size() >= 2;});
         ASSERT_TRUE( state.connections.size() == 2);

         EXPECT_NO_THROW(
            casual::domain::discovery::rediscovery::blocking::request();
         );
      }

      TEST( gateway_manager, rediscover_to_1_self_connected__1_not_connected__outbounds)
      {
         common::unittest::Trace trace;

         static constexpr auto configuration = R"(
domain: 
   name: gateway-domain

   gateway:
      inbound:
         groups:
            -  connections: 
               -  address: 127.0.0.1:6666
      outbound:
         groups:
            -  connections:
               -  address: 127.0.0.1:6666
               -  address: 127.0.0.1:6667
)";


         auto domain = local::domain( configuration);

         auto state = local::call::state( local::predicate::outbound::connected( 1));
         EXPECT_TRUE( state.connections.size() == 3);

         EXPECT_NO_THROW(
            casual::domain::discovery::rediscovery::blocking::request();
         );
      }

      TEST( gateway_manager, async_5_remote1_call__expect_pending_metric)
      {
         common::unittest::Trace trace;

         constexpr auto count = 5;
         
         auto domain = local::domain(); 

         EXPECT_TRUE( common::communication::instance::fetch::handle( common::communication::instance::identity::gateway::manager.id)); 

         {
            auto state = local::call::state( local::predicate::outbound::connected( 1));
         }

         // we exposes service "a"
         casual::service::unittest::advertise( { "a"});

         // discover to make sure outbound(s) knows about wanted services
         unittest::discover( { "a"}, {});

         auto state = local::call::state( local::predicate::outbound::routing( { "a"}, {}));


         auto outbound = [&state]() -> process::Handle
         {            
            if( auto found = algorithm::find_if( state.connections, local::predicate::is::outbound()))
               return found->process;

            return {};
         }();

         ASSERT_TRUE( outbound);


         // subscribe to metric events
         event::subscribe( common::process::handle(), { common::message::event::service::Calls::type()});

         // Expect us to reach service remote1 via outbound -> inbound -> <service remote1>
         // we act as the server
         {
            auto data = common::unittest::random::binary( 128);
            

            algorithm::for_n< count>( [&]()
            {
               common::message::service::call::callee::Request request;
               request.process = common::process::handle();
               request.service.name = "a";
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
            std::vector< common::message::event::service::Metric> metrics;

            while( metrics.size() < count)
            {
               common::message::event::service::Calls event;
               common::communication::device::blocking::receive( common::communication::ipc::inbound::device(), event);

               auto is_sequential = []( auto& metric)
               {
                  return metric.type == decltype( metric.type)::sequential;
               };
               
               // since we only have one domain, we will (possible) get the metric from outbound also,
               // and we're only interested in _sequential_ (local) services metric
               algorithm::append( algorithm::filter( event.metrics, is_sequential), metrics);
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

      TEST( gateway_manager, native_tcp_connect__disconnect____expect_robust_connection_disconnect)
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

)";

         auto domain = local::domain( configuration);

         EXPECT_TRUE( common::communication::instance::fetch::handle( common::communication::instance::identity::gateway::manager.id));

         {
            auto state = local::call::state( local::predicate::outbound::connected( 1));
            ASSERT_TRUE( state.connections.size() == 2);
         }

         {
            auto socket = communication::tcp::connect( communication::tcp::Address{ "127.0.0.1:7001"});
            EXPECT_TRUE( socket);

            // send two bytes
            posix::result( 
               ::send( socket.descriptor().value(), "AB", 2, 0));

            // we should have got a new connection
            auto state = local::call::state( []( auto& state){ return state.connections.size() >= 3;});
            ASSERT_TRUE( state.connections.size() == 3) << "connections: " << state.connections.size();
         }

         // we still got two connections
         auto state = local::call::state( []( auto& state){ return state.connections.size() <= 2;});
         ASSERT_TRUE( state.connections.size() == 2);

      }

   } // gateway

} // casual

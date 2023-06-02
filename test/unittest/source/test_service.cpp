//!
//! Copyright (c) 2021, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!

#include "common/unittest.h"

#include "domain/unittest/manager.h"

#include "service/unittest/utility.h"

#include "gateway/unittest/utility.h"

#include "common/communication/instance.h"
#include "common/message/event.h"
#include "common/event/listen.h"
#include "common/service/lookup.h"
#include "common/communication/ipc/send.h"

#include "casual/xatmi.h"

namespace casual
{
   using namespace common;

   namespace test::domain::service
   {
      namespace local
      {
         namespace
         {


            namespace configuration
            {
               constexpr auto base =  R"(
domain: 
   groups: 
      -  name: base
      -  name: user
         dependencies: [ base]
      -  name: gateway
         dependencies: [ user]
   
   servers:
      -  path: "${CASUAL_MAKE_SOURCE_ROOT}/middleware/service/bin/casual-service-manager"
         memberships: [ base]
      -  path: "${CASUAL_MAKE_SOURCE_ROOT}/middleware/transaction/bin/casual-transaction-manager"
         memberships: [ base]
      -  path: "${CASUAL_MAKE_SOURCE_ROOT}/middleware/gateway/bin/casual-gateway-manager"
         memberships: [ gateway]
)";
            } // configuration

            namespace is
            {
               auto service( std::string_view name)
               {
                  return [name]( auto& service)
                  {
                     return service.name == name;
                  };
               }
                        
            } // is   

         } // <unnamed>
      } // local

      TEST( test_service, two_server_alias__service_restriction)
      {
         common::unittest::Trace trace;

         auto domain = casual::domain::unittest::manager( local::configuration::base, R"(
domain: 
   name: A
   servers:         
      -  alias: A
         path: "${CASUAL_MAKE_SOURCE_ROOT}/middleware/example/server/bin/casual-example-server"
         memberships: [ user]
         restrictions:
            -  casual/example/echo

      -  alias: B
         path: "${CASUAL_MAKE_SOURCE_ROOT}/middleware/example/server/bin/casual-example-server"
         memberships: [ user]
         restrictions:
            -  casual/example/sink

)");

         auto state = casual::service::unittest::state();

         // check that we excluded services that doesn't match the restrictions
         EXPECT_TRUE( ! algorithm::find_if( state.services, local::is::service( "casual/example/uppercase")));


         {
            auto service = algorithm::find_if( state.services, local::is::service( "casual/example/echo"));
            ASSERT_TRUE( service);
            EXPECT_TRUE( service->instances.sequential.size() == 1) << CASUAL_NAMED_VALUE( service->instances.sequential);
            EXPECT_TRUE( service->instances.concurrent.empty());
         }

         {
            auto service = algorithm::find_if( state.services, local::is::service( "casual/example/sink"));
            ASSERT_TRUE( service);
            EXPECT_TRUE( service->instances.sequential.size() == 1) << CASUAL_NAMED_VALUE( service->instances.sequential);
            EXPECT_TRUE( service->instances.concurrent.empty());
         }
      }

      TEST( test_service, service_restriction_regex)
      {
         common::unittest::Trace trace;

         auto domain = casual::domain::unittest::manager( local::configuration::base, R"(
domain: 
   name: A
   servers:         
      -  alias: A
         path: "${CASUAL_MAKE_SOURCE_ROOT}/middleware/example/server/bin/casual-example-server"
         memberships: [ user]
         restrictions:
            -  ".*/echo$"
            -  ".*/sink$"

)");

         auto state = casual::service::unittest::state();

         // check that we excluded services that doesn't match the restrictions
         EXPECT_TRUE( ! algorithm::find_if( state.services, local::is::service( "casual/example/uppercase")));


         {
            auto service = algorithm::find_if( state.services, local::is::service( "casual/example/echo"));
            ASSERT_TRUE( service);
            EXPECT_TRUE( service->instances.sequential.size() == 1) << CASUAL_NAMED_VALUE( service->instances.sequential);
            EXPECT_TRUE( service->instances.concurrent.empty());
         }

         {
            auto service = algorithm::find_if( state.services, local::is::service( "casual/example/sink"));
            ASSERT_TRUE( service);
            EXPECT_TRUE( service->instances.sequential.size() == 1) << CASUAL_NAMED_VALUE( service->instances.sequential);
            EXPECT_TRUE( service->instances.concurrent.empty());
         }
      }

      namespace local
      {
         namespace
         {
            auto lookup = []( auto service, auto context)
            {
               auto& manager = communication::instance::outbound::service::manager::device();
               casual::message::service::lookup::Request request( process::handle());
               request.context = context;
               request.requested = service;

               auto reply = communication::ipc::call( manager, request);

               // discard the lookup
               {
                  common::message::service::lookup::discard::Request message{ process::handle()};
                  message.correlation = reply.correlation;
                  message.reply = false;
                  communication::device::blocking::send( manager, message);
               }

               return reply;
            };
            
         } // <unnamed>
      } // local

      TEST( test_service, lookup_restriction_depending_on_where_the_lookup_comes_from)
      {
         common::unittest::Trace trace;


         auto b = casual::domain::unittest::manager( local::configuration::base, R"(
domain: 
   name: B
   servers:         
      -  path: "${CASUAL_MAKE_SOURCE_ROOT}/middleware/example/server/bin/casual-example-server"
         memberships: [ user]
   services:
      -  name: casual/example/echo
         routes: [ b]
   gateway:
      inbound:
         groups:
            -  connections:
                  -  address: 127.0.0.1:7010
)");

         auto a = casual::domain::unittest::manager( local::configuration::base, R"(
domain: 
   name: A
   servers:         
      -  path: "${CASUAL_MAKE_SOURCE_ROOT}/middleware/example/server/bin/casual-example-server"
         memberships: [ user]
   services:
      -  name: casual/example/echo
         routes: [ a]
   gateway:
      outbound:
         groups:
            -  connections:
                  -  address: 127.0.0.1:7010
)");
         auto state = gateway::unittest::fetch::until( gateway::unittest::fetch::predicate::outbound::connected( 1));

         // internal
         {
            casual::message::service::lookup::request::Context context;
            context.requester = decltype( context.requester)::internal;
            auto reply = local::lookup( "b", context);
            EXPECT_TRUE( reply.state == decltype( reply.state)::idle);
         }

         // external
         {
            casual::message::service::lookup::request::Context context;
            context.requester = decltype( context.requester)::external;
            // remote service - absent
            auto reply = local::lookup( "b", context);
            EXPECT_TRUE( reply.state == decltype( reply.state)::absent);
            // local service
            reply = local::lookup( "a", context);
            EXPECT_TRUE( reply.state == decltype( reply.state)::idle);
         }

         // external with discover forward
         {
            casual::message::service::lookup::request::Context context;
            context.requester = decltype( context.requester)::external_discovery;

            {
               // local service
               auto reply = local::lookup( "a", context);
               EXPECT_TRUE( reply.state == decltype( reply.state)::idle);
            }
            {
               // remote service - idle (found)
               auto reply = local::lookup( "b", context);
               EXPECT_TRUE( reply.state == decltype( reply.state)::idle);
            }
         }

      }

      TEST( test_service, service_call_to_route__expect_metric_to_use_route_name)
      {
         common::unittest::Trace trace;

         auto domain = casual::domain::unittest::manager( local::configuration::base, R"(
domain:
   servers:
      -  path: ${CASUAL_MAKE_SOURCE_ROOT}/middleware/example/server/bin/casual-example-server
         memberships: [ user]

   services:
      -  name: casual/example/echo
         routes: [ a, b]
)");

         // Subscribe to the metric event
         common::message::event::service::Calls event;
         common::event::subscribe( common::process::handle(), { event.type()});

         // Call service to generate metric
         {
            auto buffer = tpalloc( X_OCTET, nullptr, 128);
            auto len = tptypes( buffer, nullptr, nullptr);
            tpcall( "b", buffer, len, &buffer, &len, 0);
            EXPECT_TRUE( tperrno == 0) << "tperrno: " << tperrnostring( tperrno);
         }

         // Wait for event to be delivered
         {
            common::communication::device::blocking::receive(
               common::communication::ipc::inbound::device(),
               event);

            ASSERT_TRUE( event.metrics.size() == 1);
            auto& metric = event.metrics.at( 0);
            ASSERT_TRUE( metric.service == "b") << "received: " << metric.service;
         }
      }

      TEST( test_service, nested_service_call__expect_metric_to_contain_parent)
      {
         common::unittest::Trace trace;

         auto domain = casual::domain::unittest::manager( local::configuration::base, R"(
domain:
   servers:
      -  path: ${CASUAL_MAKE_SOURCE_ROOT}/middleware/example/server/bin/casual-example-server
         memberships: [ user]

   services:
      -  name: casual/example/echo
)");

         // Set the name of the current service
         const std::string calling_service{ "caller"};
         execution::service::name( calling_service);

         // Subscribe to the metric event
         common::message::event::service::Calls event;
         common::event::subscribe( common::process::handle(), { event.type()});

         // Call service to generate metric
         {
            auto buffer = tpalloc( X_OCTET, nullptr, 128);
            auto len = tptypes( buffer, nullptr, nullptr);
            tpcall( "casual/example/echo", buffer, len, &buffer, &len, 0);
            EXPECT_TRUE( tperrno == 0) << "tperrno: " << tperrnostring( tperrno);
         }

         // Wait for event to be delivered
         {
            common::communication::device::blocking::receive(
               common::communication::ipc::inbound::device(),
               event);

            ASSERT_TRUE( event.metrics.size() == 1);
            auto& metric = event.metrics.at( 0);
            ASSERT_TRUE( metric.parent == calling_service) << "received: " << metric.parent;
         }
      }

      TEST( test_service, service_call_to_atomic_service__expect_trid)
      {
         common::unittest::Trace trace;

         auto domain = casual::domain::unittest::manager( local::configuration::base, R"(
domain:
   servers:
      -  path: ${CASUAL_MAKE_SOURCE_ROOT}/middleware/example/server/bin/casual-example-server
         memberships: [ user]
         restrictions:
            - casual/example/atomic/echo
)");

         // Subscribe to the metric event
         common::message::event::service::Calls event;
         common::event::subscribe( common::process::handle(), { event.type()});

         // Call service to generate metric
         {
            auto buffer = tpalloc( X_OCTET, nullptr, 128);
            auto len = tptypes( buffer, nullptr, nullptr);
            tpcall( "casual/example/atomic/echo", buffer, len, &buffer, &len, 0);
            EXPECT_TRUE( tperrno == 0) << "tperrno: " << tperrnostring( tperrno);
         }

         // Wait for event to be delivered
         {
            common::communication::device::blocking::receive(
               common::communication::ipc::inbound::device(),
               event);

            ASSERT_TRUE( event.metrics.size() == 1);
            auto& metric = event.metrics.at( 0);
            ASSERT_TRUE( metric.service == "casual/example/atomic/echo") << "received: " << metric.service;
            ASSERT_TRUE( metric.trid);
         }
      }

      TEST( test_service, multiplex_call_to_service__kill_server__expect_multiplex_detect_ipc_device_is_gone)
      {
         common::unittest::Trace trace;

         auto domain = casual::domain::unittest::manager( local::configuration::base, R"(
domain:
   servers:
      -  path: ${CASUAL_MAKE_SOURCE_ROOT}/middleware/example/server/bin/casual-example-server
         memberships: [ user]
         instances: 1
         restrictions:
            - casual/example/echo
)");

         // lookup / reserve the instance
         auto lookup = common::service::Lookup{ "casual/example/echo"}();
         ASSERT_TRUE( ! lookup.busy());

         communication::ipc::inbound::Device inbound;


         communication::select::Directive directive;
         communication::ipc::send::Coordinator coordinator{ directive};

         long error_count = 0;


         // we send as many request we can until the send::coordinator fails to send, due to 
         // example server can't send the replies (our inbound gets "full").
         // we want the send::Coordinator to create a socket to the example server ipc device
         {
            auto handle = process::Handle{ process::handle().pid, inbound.connector().handle().ipc()};

            message::service::call::callee::Request request{ handle};
            request.buffer.type = buffer::type::binary;
            request.buffer.data = unittest::random::binary( 64);
            request.service = lookup.service;

            auto error_callback = [ &error_count]( auto& ipc, auto& complete){ ++error_count;};

            // this will render unexpected unreserves for casual/example/echo to SM -> error logs
            // We'll just ignore this, to keep the unittest less complex.
            while( coordinator.empty())
               coordinator.send( lookup.process.ipc, request, error_callback);

            // add a few extra...
            algorithm::for_n( 20, [ &]()
            {
                coordinator.send( lookup.process.ipc, request, error_callback);
            });

            EXPECT_TRUE( ! coordinator.empty());
         }

         // Ok, now we've got send::Coordinator to be "bound"  to example-server inbound
         // ipc device.
         // If we kill example-server, send::Coordinator should detect that the ipc-device is
         // logically removed (even if the socket keeps it alive due to fd reference counting and such)
         {
            common::message::event::process::Exit event;
            auto guard = common::event::scope::subscribe( common::process::handle(), { event.type()});

            signal::send( lookup.process.pid, code::signal::kill);

            // wait until DM knows about it, and has removed the ipc device
            while( event.state.pid != lookup.process.pid)
            {
               common::communication::device::blocking::receive(
                  common::communication::ipc::inbound::device(),
                  event);
            }

            // try to send all "cached" messages -> invoke error-callback
            while( ! coordinator.empty())
               coordinator.send();

            EXPECT_TRUE( error_count > 0);
         }



      }


   } // test::domain::service

} // casual

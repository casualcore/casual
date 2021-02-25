//! 
//! Copyright (c) 2015, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!

// to be able to use 'raw' flags and codes
// since we undefine 'all' of them in common
#define CASUAL_NO_XATMI_UNDEFINE

#include "common/unittest.h"

#include "domain/manager/unittest/process.h"

#include "common/communication/instance.h"
#include "serviceframework/service/protocol/call.h"

#include "gateway/manager/admin/model.h"
#include "gateway/manager/admin/server.h"

#include "queue/api/queue.h"

#include "casual/xatmi.h"

namespace casual
{
   using namespace common;

   namespace test::domain
   {

      namespace local
      {
         namespace
         {
            using Manager = casual::domain::manager::unittest::Process;

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
      -  name: gateway
         dependencies: [ user]
   
   servers:
      - path: "${CASUAL_REPOSITORY_ROOT}/middleware/service/bin/casual-service-manager"
        memberships: [ base]
      - path: "${CASUAL_REPOSITORY_ROOT}/middleware/transaction/bin/casual-transaction-manager"
        memberships: [ base]
      - path: "${CASUAL_REPOSITORY_ROOT}/middleware/gateway/bin/casual-gateway-manager"
        memberships: [ gateway]
)";
               
            } // configuration

            namespace state::gateway
            {
               auto call()
               {
                  serviceframework::service::protocol::binary::Call call;
                  auto reply = call( casual::gateway::manager::admin::service::name::state);

                  casual::gateway::manager::admin::model::State result;
                  reply >> CASUAL_NAMED_VALUE( result);

                  return result;
               }

               template< typename P>
               auto until( P&& predicate)
               {
                  auto state = state::gateway::call();

                  auto count = 1000;

                  while( ! predicate( state) && count-- > 0)
                  {
                     process::sleep( std::chrono::milliseconds{ 2});
                     state = state::gateway::call();
                  }

                  return state;
               }

               namespace predicate::outbound
               {
                  auto connected()
                  {
                     return []( auto& state)
                     {
                        return algorithm::find_if( state.connections, []( auto& connection)
                        {
                           return connection.bound == decltype( connection.bound)::out && connection.remote.id;
                        }); 
                     };
                  };

                  auto disconnected()
                  {
                     return []( auto& state)
                     {
                        return algorithm::find_if( state.connections, []( auto& connection)
                        {
                           return connection.bound == decltype( connection.bound)::out && ! connection.remote.id;
                        }); 
                     };
                  };
               } // predicate::outbound

            } // state::gateway

            auto allocate( platform::size::type size = 128)
            {
               auto buffer = tpalloc( X_OCTET, nullptr, size);
               unittest::random::range( range::make( buffer, size));
               return buffer;
            }

            auto call = []( std::string_view service, auto code)
            {
               auto buffer = local::allocate( 128);
               auto len = tptypes( buffer, nullptr, nullptr);

               tpcall( service.data(), buffer, 128, &buffer, &len, 0);
               EXPECT_TRUE( tperrno == code) << "tperrno: " << tperrnostring( tperrno) << " code: " << tperrnostring( code);

               tpfree( buffer);

            };

            template< typename T>
            void sink( T&& value)
            {
               auto sinked = std::move( value);
            }
         } // <unnamed>
      } // local

      
      TEST( test_domain_gateway, queue_service_forward___from_A_to_B__expect_discovery)
      {
         common::unittest::Trace trace;

         constexpr auto A = R"(
domain: 
   name: A

   servers:
      - path: "${CASUAL_REPOSITORY_ROOT}/middleware/example/server/bin/casual-example-server"
        memberships: [ user]
   gateway:
      inbound:
         groups:
            -  connections: 
               -  address: 127.0.0.1:6669
)";

         constexpr auto B = R"(
domain: 
   name: B

   servers:
      -  path: "${CASUAL_REPOSITORY_ROOT}/middleware/queue/bin/casual-queue-manager"
         memberships: [ queue]
      
   queue:
      groups:
         -  alias: a
            queuebase: ':memory:'
            queues:
               -  name: a1
               -  name: a2
      forward:
         groups:
            -  services:
                  -  source: a1
                     instances: 1
                     target:
                        service: casual/example/echo
                     reply:
                        queue: a2
   
   gateway:
      outbound:
         groups:
            -  connections:
                  -  address: 127.0.0.1:6669
)";

      
         // startup the domain that will try to do stuff
         local::Manager b{ { local::configuration::base, B}};
         EXPECT_TRUE( communication::instance::ping( b.handle().ipc) == b.handle());

         auto payload = unittest::random::binary( 1024);
         {
            queue::Message message;
            message.payload.type = common::buffer::type::json();
            message.payload.data = payload;
            queue::enqueue( "a1", message);
         }

         // startup the domain that has the service
         local::Manager a{ { local::configuration::base, A}};
         EXPECT_TRUE( communication::instance::ping( a.handle().ipc) == a.handle());

         // activate the b domain, so we can try to dequeue
         b.activate();

         {
            auto message = queue::blocking::dequeue( "a2");

            EXPECT_TRUE( message.payload.type == common::buffer::type::json());
            EXPECT_TRUE( message.payload.data == payload);
         }
         
      }

      TEST( test_domain_gateway, domains_A_B__B_has_echo__call_echo_from_A__expect_discovery__shutdown_B__expect_no_ent__boot_B__expect_discovery)
      {
         common::unittest::Trace trace;

         constexpr auto A = R"(
domain: 
   name: A

   gateway:
      outbound:
         groups:
            -  connections:
                  -  address: 127.0.0.1:7770

)";

         constexpr auto B = R"(
domain: 
   name: B

   servers:
      -  path: "${CASUAL_REPOSITORY_ROOT}/middleware/example/server/bin/casual-example-server"
         memberships: [ user]

   gateway:
      inbound:
         groups:
            -  connections: 
               -  address: 127.0.0.1:7770

)";

         // sink child signals 
         signal::callback::registration< code::signal::child>( [](){});

         auto create_domain = []( auto&& config)
         {
            return local::Manager{ { local::configuration::base, config}};
         };

         auto b = create_domain( B);
         auto a = create_domain( A);

         local::state::gateway::until( local::state::gateway::predicate::outbound::connected());

         local::call( "casual/example/echo", 0);

         log::line( verbose::log, "before shutdown of B");

         // "shutdown" B
         local::sink( std::move( b));

         local::state::gateway::until( local::state::gateway::predicate::outbound::disconnected());

         log::line( verbose::log, "after shutdown of B");

         local::call( "casual/example/echo", TPENOENT);

         log::line( verbose::log, "before boot of B");

         // boot B again
         b = create_domain( B);
         a.activate();

         local::state::gateway::until( local::state::gateway::predicate::outbound::connected());

         log::line( verbose::log, "after boot of B");

         // expect to discover echo...
         local::call( "casual/example/echo", 0);
      }


      TEST( test_domain_gateway, domains_A_B__B_has_echo__A_has_route_foo_to_echo__call_route_foo_from_A__expect_discovery__shutdown_B__expect_no_ent__boot_B__expect_discovery)
      {
         common::unittest::Trace trace;

         constexpr auto A = R"(
domain: 
   name: A

   services:
      -  name: casual/example/echo
         routes: [ foo]

   gateway:
      outbound:
         groups:
            -  connections:
                  -  address: 127.0.0.1:7770

)";

         constexpr auto B = R"(
domain: 
   name: B

   servers:
      -  path: "${CASUAL_REPOSITORY_ROOT}/middleware/example/server/bin/casual-example-server"
         memberships: [ user]

   gateway:
      inbound:
         groups:
            -  connections: 
               -  address: 127.0.0.1:7770

)";

         // sink child signals 
         signal::callback::registration< code::signal::child>( [](){});

         auto create_domain = []( auto&& config)
         {
            return local::Manager{ { local::configuration::base, config}};
         };

         auto b = create_domain( B);
         auto a = create_domain( A);

         local::state::gateway::until( local::state::gateway::predicate::outbound::connected());

         local::call( "foo", 0);

         log::line( verbose::log, "before shutdown of B");

         // "shutdown" B
         local::sink( std::move( b));
         local::state::gateway::until( local::state::gateway::predicate::outbound::disconnected());

         log::line( verbose::log, "after shutdown of B");

         local::call( "foo", TPENOENT);

         log::line( verbose::log, "before boot of B");

         // boot B again
         b = create_domain( B);
         a.activate();

         local::state::gateway::until( local::state::gateway::predicate::outbound::connected());

         log::line( verbose::log, "after boot of B");
         
         // expect to discover echo...
         local::call( "foo", 0);
      }


      TEST( test_domain_gateway, domains_A_B__B_has_echo__A_has_route_foo_and_bar_to_echo__call_route_foo_from_A__expect_discovery__shutdown_B__expect_no_ent__boot_B__expect_discovery)
      {
         common::unittest::Trace trace;

         constexpr auto A = R"(
domain: 
   name: A

   services:
      -  name: casual/example/echo
         routes: [ foo, bar]

   gateway:
      outbound:
         groups:
            -  connections:
                  -  address: 127.0.0.1:7770

)";

         constexpr auto B = R"(
domain: 
   name: B

   servers:
      -  path: "${CASUAL_REPOSITORY_ROOT}/middleware/example/server/bin/casual-example-server"
         memberships: [ user]

   gateway:
      inbound:
         groups:
            -  connections: 
               -  address: 127.0.0.1:7770

)";

         // sink child signals 
         signal::callback::registration< code::signal::child>( [](){});

         auto create_domain = []( auto&& config)
         {
            return local::Manager{ { local::configuration::base, config}};
         };

         auto b = create_domain( B);
         auto a = create_domain( A);

         local::state::gateway::until( local::state::gateway::predicate::outbound::connected());

         local::call( "foo", 0);

         log::line( verbose::log, "before shutdown of B");

         // "shutdown" B
         local::sink( std::move( b));
         local::state::gateway::until( local::state::gateway::predicate::outbound::disconnected());

         log::line( verbose::log, "after shutdown of B");

         local::call( "foo", TPENOENT);

         log::line( verbose::log, "before boot of B");

         // boot B again
         b = create_domain( B);
         a.activate();

         local::state::gateway::until( local::state::gateway::predicate::outbound::connected());

         log::line( verbose::log, "after boot of B");
         
         // expect to discover echo...
         local::call( "foo", 0);

      }

   } // test::domain
} // casual

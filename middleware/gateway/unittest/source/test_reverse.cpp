//!
//! Copyright (c) 2020, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!

#include "common/unittest.h"

#include "gateway/unittest/utility.h"
#include "gateway/manager/admin/model.h"
#include "gateway/manager/admin/server.h"

#include "domain/manager/unittest/process.h"
#include "service/unittest/utility.h"

#include "serviceframework/service/protocol/call.h"

#include "common/communication/instance.h"


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
   name: gateway-reverse

   groups: 
      - name: base
      - name: gateway
        dependencies: [ base]
   
   servers:
      - path: "${CASUAL_HOME}/bin/casual-service-manager"
        memberships: [ base]
      - path: "${CASUAL_HOME}/bin/casual-transaction-manager"
        memberships: [ base]
      - path: "bin/casual-gateway-manager"
        memberships: [ gateway]
   gateway:
      reverse:
         outbound:
            groups:
               -  connections:
                  -  address: 127.0.0.1:6669
                     services:
                        - a
                        - b

         inbound:
            groups:
               -  connections: 
                  - address: 127.0.0.1:6669
                  - address: 127.0.0.1:6669
                  - address: 127.0.0.1:6669
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
            }

            namespace state
            {
               template< typename P>
               auto until( P&& predicate)
               {
                  auto state = call::state();

                  auto count = 1000;

                  while( ! predicate( state) && count-- > 0)
                  {
                     process::sleep( std::chrono::milliseconds{ 2});
                     state = call::state();
                  }

                  return state;
               }
               
            } // state


         } // unnamed
      } // local

      TEST( gateway_manager_reverse, inbounds_outbounds_127_0_0_1__6669)
      {
         common::unittest::Trace trace;

         local::Domain domain;

         auto state = local::state::until( []( auto& state)
         { 
            return ! state.connections.empty() && ! state.connections[ 0].remote.name.empty();
         });

         ASSERT_TRUE( ! state.connections.empty()) << CASUAL_NAMED_VALUE( state);
         EXPECT_TRUE( state.connections[ 0].remote.name == "gateway-reverse");
      }

      TEST( gateway_manager_reverse, advertise_a__discovery__expect_to_find_a)
      {
         common::unittest::Trace trace;

         local::Domain domain;

         casual::service::unittest::advertise( { "a"});

         auto state = local::state::until( []( auto& state)
         { 
            return ! state.connections.empty() && ! state.connections[ 0].remote.name.empty();
         });

         unittest::discover( { "a"}, {});

         // check that service has concurrent instances
         {
            auto service = unittest::service::state();
            auto found = algorithm::find( service.services, "a");
            ASSERT_TRUE( found);
            EXPECT_TRUE( ! found->instances.concurrent.empty()) << CASUAL_NAMED_VALUE( *found);
         }

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

      TEST( gateway_manager_reverse, advertise_a__discovery__call_outbound)
      {
         common::unittest::Trace trace;

         local::Domain domain;

         casual::service::unittest::advertise( { "a"});


         auto state = local::state::until( []( auto& state)
         { 
            return ! state.connections.empty() && ! state.connections[ 0].remote.name.empty();
         });

         unittest::discover( { "a"}, {});


         auto outbound_processes = []() -> std::vector< process::Handle>
         {
            auto state = unittest::service::state();
            if( auto found = algorithm::find( state.services, "a"))
            {
               return algorithm::accumulate( found->instances.concurrent, std::vector< process::Handle>{}, [&state]( auto result, auto& instance)
               {
                  if( auto found = algorithm::find( state.instances.concurrent, instance.pid))
                     result.push_back( found->process);
                  return result;
               });
            }

            return {};
         };

         
         auto outbounds = outbound_processes();
         ASSERT_TRUE( ! outbounds.empty()) << CASUAL_NAMED_VALUE( outbounds);

         const auto data = common::unittest::random::binary( 128);

         algorithm::for_n< 10>( [&]()
         {
            algorithm::rotate( outbounds, std::begin( outbounds) + 1);
            auto& process = range::front( outbounds);

            common::message::service::call::callee::Request request{ common::process::handle()};
            request.service.name = "a";
            request.buffer.memory = data;
            
            common::communication::device::blocking::send( process.ipc, request);

            // echo the call
            local::service::echo();

            common::message::service::call::Reply reply;
            common::communication::device::blocking::receive( common::communication::ipc::inbound::device(), reply);

            EXPECT_TRUE( reply.buffer.memory ==  data);
         });
      }

   } // gateway
} // casual
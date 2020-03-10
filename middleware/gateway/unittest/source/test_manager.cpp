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

#include "common/message/domain.h"
#include "common/message/queue.h"

#include "serviceframework/service/protocol/call.h"
#include "serviceframework/log.h"

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
         - address: 127.0.0.1:6666
      connections:
         - address: 127.0.0.1:6666
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

                           return algorithm::all_of( state.connections, []( const manager::admin::model::Connection& c){
                              return c.runlevel >= manager::admin::model::Connection::Runlevel::online &&
                                 c.process == communication::instance::ping( c.process.ipc);
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



      TEST( casual_gateway_manager_tcp, listen_on_127_0_0_1__6666)
      {
         common::unittest::Trace trace;

         EXPECT_NO_THROW( {
            local::Domain domain;
         });
      }

      TEST( casual_gateway_manager_tcp, listen_on_127_0_0_1__6666__outbound__127_0_0_1__6666__expect_connection)
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
                  common::communication::ipc::blocking::receive( common::communication::ipc::inbound::device(), request);
                  
                  common::message::service::call::Reply reply;
                  reply.correlation = request.correlation;
                  reply.buffer = std::move( request.buffer);
                  reply.transaction.trid = std::move( request.trid);

                  common::communication::ipc::blocking::send( request.process.ipc, reply);
               };
            } // service
         } // <unnamed>
      } // local
      TEST( casual_gateway_manager_tcp, connect_to_our_self__remote1_call__expect_service_remote1)
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
               
               common::communication::ipc::blocking::send( state.connections.at( 0).process.ipc, request);
            }

            // echo the call
            local::service::echo();

            common::message::service::call::Reply reply;
            common::communication::ipc::blocking::receive( common::communication::ipc::inbound::device(), reply);

            EXPECT_TRUE( reply.buffer.memory ==  data);
         }
      }



      TEST( casual_gateway_manager_tcp, connect_to_our_self__remote1_call_in_transaction___expect_same_transaction_in_reply)
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
               
               common::communication::ipc::blocking::send( state.connections.at( 0).process.ipc, request);
            }

            // echo the call
            local::service::echo();

            common::message::service::call::Reply reply;
            common::communication::ipc::blocking::receive( common::communication::ipc::inbound::device(), reply);

            EXPECT_TRUE( reply.buffer.memory == data);
            EXPECT_TRUE( reply.transaction.trid == trid)  << "reply.transaction.trid: " << reply.transaction.trid << "\ntrid: " << trid;
         }

         // send prepare
         {
            common::message::transaction::resource::prepare::Request request;
            request.trid = trid;
            request.process = common::process::handle();
            request.resource = common::strong::resource::id{ 42};
            common::communication::ipc::blocking::send( state.connections.at( 0).process.ipc, request);
         }

         // get prepare reply
         {
            common::message::transaction::resource::prepare::Reply reply;
            common::communication::ipc::blocking::receive( common::communication::ipc::inbound::device(), reply);
            EXPECT_TRUE( reply.trid == trid);
         }
      }


      TEST( casual_gateway_manager_tcp,  connect_to_our_self__enqueue_dequeue___expect_message)
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
            message::queue::enqueue::Request request;
            request.process = process::handle();
            request.name = "queue1";
            request.message.type = "json";
            request.message.payload = payload;


            auto reply = communication::ipc::call( outbound.ipc, request);
            EXPECT_TRUE( ! reply.id.empty());
         }

         // dequeue
         {
            message::queue::dequeue::Request request;
            request.process = process::handle();
            request.name = "queue1";

            auto reply = communication::ipc::call( outbound.ipc, request);
            ASSERT_TRUE( ! reply.message.empty());
            EXPECT_TRUE( reply.message.front().payload == payload);
            EXPECT_TRUE( reply.message.front().type == "json");
         }
      }

   } // gateway

} // casual

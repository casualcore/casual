//!
//! Copyright (c) 2025, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!

#include "common/unittest.h"

#include "gateway/unittest/utility.h"
#include "gateway/message.h"

#include "common/message/transaction.h"
#include "common/message/service.h"
#include "common/communication/tcp.h"
#include "common/communication/instance.h"

#include "domain/message/discovery.h"

#include "domain/unittest/manager.h"
#include "domain/unittest/discover.h"



namespace casual
{
   namespace gateway
   {
      using namespace common;

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
)";
     
            } // configuration

            template< typename... C>
            auto domain( C&&... configurations)
            {
               return casual::domain::unittest::manager( configuration::servers, std::forward< C>( configurations)...);
            }

            auto domain_identity( std::string name)
            {
               return common::domain::Identity{ strong::domain::id::generate(), name};
            }
           
         } // <unnamed>
      } // local
      

      TEST( gateway_protocol_1_0_manager, domain_A__connect_as_inbound__call_x___expect_correct_call_request)
      {
         common::unittest::Trace trace;

         auto a = local::domain( R"(
domain:
   name: A
   servers:
      - path: bin/casual-gateway-manager
        memberships: [ gateway]
   gateway:
      reverse:
         outbound:
            groups:
               -  connections: 
                  -  address: 127.0.0.1:7010
         )");

        
         auto device = unittest::tcp::connect::in( "127.0.0.1:7010", message::protocol::Version::v1_0);
         EXPECT_TRUE( device.connector().socket());

         const auto payload = unittest::random::binary( 1000);
         const auto trid = common::transaction::id::create();

         // make sure outbound has register as discover provider
         {
            using Ability = casual::domain::message::discovery::api::provider::registration::Ability;

            casual::domain::unittest::discover::fetch::until( 
               casual::domain::unittest::discover::fetch::predicate::provider( Ability::discover, 1));
         }

         // send service lookup
         {
            common::message::service::lookup::Request request{ common::process::handle()}; 
            request.trid = trid;
            request.requested = "x";
            communication::device::blocking::send( communication::instance::outbound::service::manager::device(), request);
         }
         
         // take care of the discovery request
         {
            auto request = communication::device::receive< casual::domain::message::discovery::Request>( device);
            ASSERT_TRUE( request.content.services.size() == 1);
            EXPECT_TRUE( request.content.services.at( 0) == "x");
      
            casual::domain::message::discovery::v1_3::Reply reply;
            reply.correlation = request.correlation;
            reply.content.services.push_back( casual::domain::message::discovery::reply::content::Service{ .name = "x",});

            communication::device::blocking::send( device, reply);
         }
         
         // make the call
         {
            auto reply = communication::ipc::receive< common::message::service::lookup::Reply>();
            ASSERT_TRUE( reply.state == decltype( reply.state)::idle);

            common::message::service::call::callee::Request request{ common::process::handle()};
            request.correlation = reply.correlation;
            request.service.name = "x";
            request.header.add( { "foo", "bar"});
            request.trid = trid;
            request.deadline.remaining = std::chrono::seconds{ 10};
            request.buffer.data = payload;
            request.buffer.type = common::buffer::type::binary;

            communication::device::blocking::send( reply.process.ipc, request);
         }

         // take care of the service call
         {
            auto request = communication::device::receive< common::message::service::call::v1_2::callee::Request>( device);
            ASSERT_TRUE( request.service.name == "x");
            EXPECT_TRUE( request.buffer.data == payload);
            EXPECT_TRUE( request.correlation );

            EXPECT_TRUE( request.trid == trid);

            auto reply = common::message::reverse::type( request);
            reply.buffer.data = payload;
            reply.transaction.trid = request.trid;

            communication::device::blocking::send( device, reply);
         }

         // take care of the initial service call
         {  
            auto reply = communication::ipc::receive< common::message::service::call::Reply>();
            ASSERT_TRUE( reply.code.result == code::xatmi::ok);
            EXPECT_TRUE( reply.buffer.data == payload);
         }

         
         const auto transaction_correlation = common::strong::correlation::id::generate();
         
         // send commit request to transaction manager
         {
            common::message::transaction::commit::Request request{ common::process::handle()};
            request.correlation = transaction_correlation;
            request.trid = trid;
            communication::device::blocking::send( communication::instance::outbound::transaction::manager::device(), request);
         }

         // act as TM in our "virtual" domain and echo the resource commit request
         {
            auto request = communication::device::receive< common::message::transaction::resource::commit::Request>( device);
            EXPECT_TRUE( request.trid == trid);
            EXPECT_TRUE( common::flag::contains( request.flags, decltype( request.flags)::one_phase));

            auto reply = common::message::reverse::type( request);
            reply.trid = request.trid;

            communication::device::blocking::send( device, reply);
         }

         // receive the commit reply from TM
         {
            auto reply = communication::ipc::receive< common::message::transaction::commit::Reply>( transaction_correlation);
            EXPECT_TRUE( reply.trid == trid);
         }
      }

      TEST( gateway_protocol_1_0_manager, domain_A_connect_as_inbound_and_outbound__call_x___expect_correct_call_request_and_branched_trid)
      {
         common::unittest::Trace trace;


         auto a = local::domain( R"(
domain:
   name: A
   servers:
      - path: bin/casual-gateway-manager
        memberships: [ gateway]
   gateway:
      inbound:
         groups:
            -  connections: 
               -  address: 127.0.0.1:7010
                  discovery: 
                     forward: true
      reverse:
         outbound:
            groups:
               -  connections: 
                  -  address: 127.0.0.1:7011
         )");

         const auto domain_in = local::domain_identity( "in");
         const auto domain_out = local::domain_identity( "out");

         auto out = unittest::tcp::connect::out( "127.0.0.1:7010", message::protocol::Version::v1_0, domain_out);
         EXPECT_TRUE( out.connector().socket());

         auto in = unittest::tcp::connect::in( "127.0.0.1:7011", message::protocol::Version::v1_0, domain_in);
         EXPECT_TRUE( in.connector().socket());
         
         // maker sure the domain is ready
         unittest::fetch::until( predicate::conjunction( 
            unittest::fetch::predicate::outbound::connected( 1), unittest::fetch::predicate::inbound::connected( 1)));

         // make sure outbound has register as discover provider
         {
            using Ability = casual::domain::message::discovery::api::provider::registration::Ability;

            casual::domain::unittest::discover::fetch::until( 
               casual::domain::unittest::discover::fetch::predicate::provider( Ability::discover, 1));
         }

         const auto payload = unittest::random::binary( 1000);
         const auto trid = common::transaction::id::create();



         // send discovery to inbound
         {
            casual::domain::message::discovery::Request request{ common::process::handle()};
            request.content.services.push_back( "x");
            request.domain = domain_out;
            communication::device::blocking::send( out, request);
         }
         
         // take care of the discovery request from outbound
         {
            auto request = communication::device::receive< casual::domain::message::discovery::Request>( in);
            ASSERT_TRUE( request.content.services.size() == 1);
            EXPECT_TRUE( request.content.services.at( 0) == "x");
      
            casual::domain::message::discovery::v1_3::Reply reply;
            reply.correlation = request.correlation;
            reply.domain = domain_in;
            reply.content.services.push_back( casual::domain::message::discovery::reply::content::Service{ .name = "x",});

            communication::device::blocking::send( in, reply);
         }

         // receive the discovery reply from inbound
         {
            auto reply = communication::device::receive< casual::domain::message::discovery::v1_3::Reply>( out);
            ASSERT_TRUE( reply.content.services.size() == 1);
            EXPECT_TRUE( reply.content.services.at( 0).name == "x");
         }
         
         // send call request to inbound
         {
            common::message::service::call::v1_2::callee::Request request{ common::process::handle()};
            request.service.name = "x";
            request.header.add( { "foo", "bar"});
            request.trid = trid;
            request.service.timeout.duration = std::chrono::seconds{ 10};
            request.buffer.data = payload;
            request.buffer.type = common::buffer::type::binary;

            communication::device::blocking::send( out, request);
         }

         common::transaction::ID branched_trid;

         // receive call request from outbound
         {
            auto request = communication::device::receive< common::message::service::call::v1_2::callee::Request>( in);
            ASSERT_TRUE( request.service.name == "x");
            EXPECT_TRUE( request.buffer.data == payload);
            EXPECT_TRUE( request.correlation );
            
            branched_trid = request.trid;

            EXPECT_TRUE( request.trid.global() == trid.global());
            // the trid should be branched
            EXPECT_TRUE( ! request.trid.branch().empty());
            EXPECT_TRUE( request.trid.branch() != trid.branch());

            auto reply = common::message::reverse::type( request);
            reply.buffer.data = payload;
            reply.transaction.trid = request.trid;

            communication::device::blocking::send( in, reply);
         }

         // receive call reply from inbound
         {  
            auto reply = communication::device::receive< common::message::service::call::v1_2::Reply>( out);
            ASSERT_TRUE( reply.code.result == code::xatmi::ok);
            EXPECT_TRUE( reply.buffer.data == payload);
            EXPECT_TRUE( reply.transaction.trid == trid);
         }

         // send transaction commit to inbound
         {
            common::message::transaction::resource::commit::Request request;
            request.trid = trid;
            request.flags =  decltype( request.flags)::one_phase;     
            communication::device::blocking::send( out, request);
         }

         // receive transaction prepare from outbound
         {
            auto request = communication::device::receive< common::message::transaction::resource::prepare::Request>( in);
            
            EXPECT_TRUE( request.trid == branched_trid);
         
            auto reply = common::message::reverse::type( request);
            reply.trid = request.trid;
            reply.state = code::xa::ok;
            reply.resource = request.resource;
            communication::device::blocking::send( in, reply);
         }

         // receive transaction commit from outbound
         {
            auto request = communication::device::receive< common::message::transaction::resource::commit::Request>( in);
            
            EXPECT_TRUE( request.trid == branched_trid);
            
            auto reply = common::message::reverse::type( request);
            reply.trid = request.trid;
            reply.state = code::xa::ok;
            reply.resource = request.resource;
            communication::device::blocking::send( in, reply);
         }

         // receive transaction commit reply from inbound
         {
            auto reply = communication::device::receive< common::message::transaction::resource::commit::Reply>( out);
            ASSERT_TRUE( reply.state == code::xa::ok);
            EXPECT_TRUE( reply.trid == trid);
         }

      }

   } // gateway  
} // casual
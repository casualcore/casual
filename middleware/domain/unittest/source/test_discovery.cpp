//!
//! Copyright (c) 2021, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!

#include "common/unittest.h"

#include "domain/discovery/api.h"
#include "domain/manager/unittest/process.h"

#include "common/communication/device.h"
#include "common/communication/ipc.h"
#include "common/array.h"

namespace casual
{
   using namespace std::literals;

   using namespace common;
   namespace domain::discovery
   {   

      TEST( domain_discovery, no_discoverable__outbound_request___expect_empty_message)
      {
         unittest::Trace trace;

         manager::unittest::Process manager;

         auto correlation = discovery::request( {}, {});

         // wait for the reply
         {
            message::discovery::api::Reply reply;
            communication::device::blocking::receive( communication::ipc::inbound::device(), reply);
            EXPECT_TRUE( reply.correlation == correlation);
         }
      }

      TEST( domain_discovery, no_discoverable__inbound_request___expect_empty_message)
      {
         unittest::Trace trace;

         manager::unittest::Process manager;

         auto correlation = discovery::request( message::discovery::Request{ process::handle()});

         // wait for the reply
         {
            message::discovery::Reply reply;
            communication::device::blocking::receive( communication::ipc::inbound::device(), reply);
            EXPECT_TRUE( reply.correlation == correlation);
         }
      }


      TEST( domain_discovery, register_as_discover_provieder__send_api_request___expect_request__then_reply)
      {
         unittest::Trace trace;

         manager::unittest::Process manager;

         // we register our self
         discovery::provider::registration( discovery::provider::Ability::discover_external);

         auto correlation = discovery::request( { "a"}, {});

         // wait for the request, and send reply reply
         {
            message::discovery::Request request;
            communication::device::blocking::receive( communication::ipc::inbound::device(), request);
            auto reply = common::message::reverse::type( request, process::handle());
            communication::device::blocking::send( request.process.ipc, reply);
         }

         // wait for the reply
         {
            message::discovery::api::Reply reply;
            communication::device::blocking::receive( communication::ipc::inbound::device(), reply);
            EXPECT_TRUE( reply.correlation == correlation);
         }
      }

      TEST( domain_discovery, register_as_means_provider__send_discovery_request___expect_request__then_reply)
      {
         unittest::Trace trace;

         manager::unittest::Process manager;

         // we register our self
         discovery::provider::registration( discovery::provider::Ability::discover_internal);

         auto request = message::discovery::Request{ process::handle()};
         request.content.services = { "a"};
         auto correlation = discovery::request( request);

         // wait for the request, and send reply reply
         {
            message::discovery::Request request;
            communication::device::blocking::receive( communication::ipc::inbound::device(), request);
            EXPECT_TRUE( algorithm::equal( request.content.services, array::make( "a")));
            auto reply = common::message::reverse::type( request, process::handle());
            reply.content.services.emplace_back( "a", "foo", common::service::transaction::Type::branch);
            communication::device::blocking::send( request.process.ipc, reply);
         }

         // wait for the reply
         {
            message::discovery::Reply reply;
            communication::device::blocking::receive( communication::ipc::inbound::device(), reply);
            EXPECT_TRUE( reply.correlation == correlation);
            ASSERT_TRUE( reply.content.services.size() == 1);
         }
      }


      TEST( domain_discovery, register_as_discover_provieder_x2__send_outbound_request___expect_2_request__then_reply)
      {
         unittest::Trace trace;

         manager::unittest::Process manager;

         // we register our self
         discovery::provider::registration( discovery::provider::Ability::discover_external);
         // we fake our next registration...
         discovery::provider::registration( process::Handle{ strong::process::id{ process::id().value() + 1}, process::handle().ipc}, discovery::provider::Ability::discover_external);

         auto correlation = discovery::request( { "a"}, {});

         auto handle_request = []()
         {
            message::discovery::Request request;
            communication::device::blocking::receive( communication::ipc::inbound::device(), request);
            auto reply = common::message::reverse::type( request, process::handle());
            communication::device::blocking::send( request.process.ipc, reply);
         };

         // handle both request
         handle_request();
         handle_request();

         // wait for the reply
         {
            message::discovery::api::Reply reply;
            communication::device::blocking::receive( communication::ipc::inbound::device(), reply);
            EXPECT_TRUE( reply.correlation == correlation);
         }
      }

      TEST( domain_discovery, register_as_external_and_internal_discover_provider__send_forward_request___expect_internal_and_external_discovery_requests__then_reply)
      {
         unittest::Trace trace;

         manager::unittest::Process manager;

         // we register our self
         discovery::provider::registration( discovery::provider::Ability::discover_external);
         discovery::provider::registration( { strong::process::id{ process::id().underlaying() + 1}, process::handle().ipc}, discovery::provider::Ability::discover_internal);

         auto correlation = discovery::request( [](){
            message::discovery::Request request{ process::handle()};
            request.directive = decltype( request.directive)::forward;
            request.content.queues = { "a", "b"};
            return request;
         }());
         
         auto handle_request = []( auto request)
         {
            communication::device::blocking::receive( communication::ipc::inbound::device(), request);
            auto reply = common::message::reverse::type( request, process::handle());
            reply.content.queues.emplace_back( "a");
            communication::device::blocking::send( request.process.ipc, reply);
         };

         // handle the means request and the discovery request
         handle_request( message::discovery::Request{});
         handle_request( message::discovery::Request{});

         // wait for the reply
         {
            message::discovery::Reply reply;
            communication::device::blocking::receive( communication::ipc::inbound::device(), reply);
            EXPECT_TRUE( reply.correlation == correlation);
            ASSERT_TRUE( reply.content.queues.size() == 1) << CASUAL_NAMED_VALUE( reply);
            EXPECT_TRUE( reply.content.queues.at( 0).name == "a");
         }
      }

      TEST( domain_discovery, register_discover_external_internal_and_needs___rediscover___expect_needs_and_discover_external_request__then_reply)
      {
         unittest::Trace trace;

         manager::unittest::Process manager;
         using Ability = discovery::provider::Ability;

         discovery::provider::registration( { Ability::discover_external, Ability::discover_internal, Ability::needs});

         auto correlation = discovery::rediscovery::request();
         
         // needs
         {
            auto request = communication::ipc::receive< message::discovery::needs::Request>();
            auto reply = common::message::reverse::type( request, process::handle());
            reply.content.queues = { "a", "b"};
            communication::device::blocking::send( request.process.ipc, reply);
         }

         // discovery external
         {
            auto request = communication::ipc::receive< message::discovery::Request>();
            EXPECT_TRUE( algorithm::equal( request.content.queues, array::make( "a"sv, "b"sv)));
            auto reply = common::message::reverse::type( request);
            reply.content.queues.emplace_back( "a");
            communication::device::blocking::send( request.process.ipc, reply);
         };


         // wait for the reply
         {
            auto reply = communication::ipc::receive< message::discovery::api::rediscovery::Reply>();
            EXPECT_TRUE( reply.correlation == correlation);
            EXPECT_TRUE( algorithm::equal( reply.content.queues, array::make( "a"sv)));
         }
      }

      TEST( domain_discovery, register_discover_external_and_needs_and_topology__send_topology_update___expect_needs_and_discover_external_request_and_topology_update)
      {
         unittest::Trace trace;

         manager::unittest::Process manager;
         using Ability = discovery::provider::Ability;

         discovery::provider::registration( { Ability::discover_external, Ability::topology, Ability::needs});

         discovery::topology::update();
         
         // needs
         {
            auto request = communication::ipc::receive< message::discovery::needs::Request>();
            auto reply = common::message::reverse::type( request, process::handle());
            reply.content.queues = { "a", "b"};
            communication::device::blocking::send( request.process.ipc, reply);
         }

         // discovery external
         {
            auto request = communication::ipc::receive< message::discovery::Request>();
            EXPECT_TRUE( algorithm::equal( request.content.queues, array::make( "a"sv, "b"sv)));
            auto reply = common::message::reverse::type( request);
            reply.content.queues.emplace_back( "a");
            communication::device::blocking::send( request.process.ipc, reply);
         };

         // we get the topology update
         {
            auto reply = communication::ipc::receive< message::discovery::topology::Update>();
            ASSERT_TRUE( reply.domains.size() == 1);
            EXPECT_TRUE( reply.domains.at( 0) == common::domain::identity());
         }
      }

   } // domain::discovery
   
} // casual
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

namespace casual
{
   using namespace common;
   namespace domain::discovery
   {
      TEST( domain_discovery, no_discoverable__outbound_request___expect_empty_message)
      {
         unittest::Trace trace;

         manager::unittest::Process manager;

         auto correlation = discovery::external::request( message::discovery::external::Request{ process::handle()});

         // wait for the reply
         {
            message::discovery::external::Reply reply;
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


      TEST( domain_discovery, our_self_as_outbound_discoverable__send_outbound_request___expect_request__then_reply)
      {
         unittest::Trace trace;

         manager::unittest::Process manager;

         // we register our self
         discovery::external::registration();

         auto correlation = discovery::external::request( message::discovery::external::Request{ process::handle()});

         // wait for the request, and send reply reply
         {
            message::discovery::Request request;
            communication::device::blocking::receive( communication::ipc::inbound::device(), request);
            auto reply = common::message::reverse::type( request, process::handle());
            communication::device::blocking::send( request.process.ipc, reply);
         }

         // wait for the reply
         {
            message::discovery::external::Reply reply;
            communication::device::blocking::receive( communication::ipc::inbound::device(), reply);
            EXPECT_TRUE( reply.correlation == correlation);
         }
      }

      TEST( domain_discovery, our_self_as_inbound_discoverable__send_inbound_request___expect_request__then_reply)
      {
         unittest::Trace trace;

         manager::unittest::Process manager;

         // we register our self
         discovery::internal::registration();

         auto correlation = discovery::request( message::discovery::Request{ process::handle()});

         // wait for the request, and send reply reply
         {
            message::discovery::Request request;
            communication::device::blocking::receive( communication::ipc::inbound::device(), request);
            auto reply = common::message::reverse::type( request, process::handle());
            communication::device::blocking::send( request.process.ipc, reply);
         }

         // wait for the reply
         {
            message::discovery::Reply reply;
            communication::device::blocking::receive( communication::ipc::inbound::device(), reply);
            EXPECT_TRUE( reply.correlation == correlation);
         }
      }


      TEST( domain_discovery, our_self_as_2_outbound_discoverable__send_outbound_request___expect_2_request__then_reply)
      {
         unittest::Trace trace;

         manager::unittest::Process manager;

         // we register our self
         discovery::external::registration();
         // we fake our next registration...
         discovery::external::registration( process::Handle{ strong::process::id{ process::id().value() + 1}, process::handle().ipc});

         auto correlation = discovery::external::request( message::discovery::external::Request{ process::handle()});

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
            message::discovery::external::Reply reply;
            communication::device::blocking::receive( communication::ipc::inbound::device(), reply);
            EXPECT_TRUE( reply.correlation == correlation);
         }
      }

      TEST( domain_discovery, our_self_outbound_and_inbound_discoverable__send_forward_request___expect_2_request__then_reply)
      {
         unittest::Trace trace;

         manager::unittest::Process manager;

         // we register our self
         discovery::external::registration();
         // we fake our next registration...
         discovery::internal::registration( process::Handle{ strong::process::id{ process::id().value() + 1}, process::handle().ipc});

         auto correlation = discovery::request( [](){
            message::discovery::Request request{ process::handle()};
            request.directive = decltype( request.directive)::forward;
            request.content.queues = { "a", "b"};
            return request;
         }());

         auto handle_request = []()
         {
            message::discovery::Request request;
            communication::device::blocking::receive( communication::ipc::inbound::device(), request);
            auto reply = common::message::reverse::type( request, process::handle());
            reply.content.queues.emplace_back( "a");
            communication::device::blocking::send( request.process.ipc, reply);
         };

         // handle both request
         handle_request();
         handle_request();

         // wait for the reply
         {
            message::discovery::Reply reply;
            communication::device::blocking::receive( communication::ipc::inbound::device(), reply);
            EXPECT_TRUE( reply.correlation == correlation);
            ASSERT_TRUE( reply.content.queues.size() == 1) << CASUAL_NAMED_VALUE( reply);
            EXPECT_TRUE( reply.content.queues.at( 0).name == "a");
         }
      }

      TEST( domain_discovery, our_self_as_2_outbound_rediscoverable__send_rediscover_request___expect_2_request__then_reply)
      {
         unittest::Trace trace;

         manager::unittest::Process manager;

         // we register our self
         discovery::external::registration( discovery::external::Directive::rediscovery);
         // we fake our next registration...
         discovery::external::registration( 
            process::Handle{ strong::process::id{ process::id().value() + 1}, process::handle().ipc}, 
            discovery::external::Directive::rediscovery);

         auto correlation = discovery::rediscovery::request();

         auto handle_request = []()
         {
            message::discovery::rediscovery::Request request;
            communication::device::blocking::receive( communication::ipc::inbound::device(), request);
            auto reply = common::message::reverse::type( request);
            communication::device::blocking::send( request.process.ipc, reply);
         };

         // handle both request
         handle_request();
         handle_request();

         // wait for the reply
         {
            message::discovery::rediscovery::Reply reply;
            communication::device::blocking::receive( communication::ipc::inbound::device(), reply);
            EXPECT_TRUE( reply.correlation == correlation);
         }
      }


   } // domain::discovery
   
} // casual
//!
//! test_mockup.cpp
//!
//! Created on: May 25, 2014
//!     Author: Lazan
//!

#include <gtest/gtest.h>

#include "common/mockup/ipc.h"

#include "common/message/service.h"

#include "common/communication/ipc.h"
#include "common/log.h"
#include "common/trace.h"
#include "common/internal/log.h"

#include "common/signal.h"


#include "common/environment.h"


namespace casual
{
   namespace common
   {

      TEST( casual_common_mockup, ipc_Instance_startup)
      {
         EXPECT_NO_THROW({
            mockup::ipc::Instance instance;
         });
      }


      TEST( casual_common_mockup, ipc_Instance_one_message)
      {
         trace::Scope trace{ "TEST( casual_common_mockup, ipc_Instance_one_message)", log::internal::ipc};

         // so we don't hang for ever, if something is wrong...
         common::signal::timer::Scoped timout( std::chrono::seconds( 5));

         mockup::ipc::Instance instance;

         {
            message::service::lookup::Request request;
            request.requested = "someService";
            request.process = process::handle();

            communication::ipc::blocking::send( instance.input(), request);
         }

         {
            message::service::lookup::Request request;
            communication::ipc::blocking::receive( instance.output(), request);

            EXPECT_TRUE( request.requested == "someService");
            EXPECT_TRUE( request.process.queue == communication::ipc::inbound::id());

         }
      }


      TEST( casual_common_mockup, ipc_link_2_instances__send_one_message)
      {
         trace::Scope trace{ "TEST( casual_common_mockup, ipc_Instance_one_message)", log::internal::ipc};

         // so we don't hang for ever, if something is wrong...
         common::signal::timer::Scoped timout( std::chrono::seconds( 5));

         mockup::ipc::Instance source;
         mockup::ipc::Instance destination;

         //
         // Link "output" of source to "input" of destination
         //
         mockup::ipc::Link link{ source.output().connector().id(), destination.input()};

         {
            message::service::lookup::Request request;
            request.requested = "someService";
            request.process = process::handle();

            communication::ipc::blocking::send( source.input(), request);
         }

         {
            message::service::lookup::Request request;
            communication::ipc::blocking::receive( destination.output(), request);

            EXPECT_TRUE( request.requested == "someService");
            EXPECT_TRUE( request.process.queue == communication::ipc::inbound::id());

         }
      }


      TEST( casual_common_mockup, ipc_Instance_200_messages)
      {
         trace::Scope trace{ "TEST( casual_common_mockup, ipc_Instance_200_messages)",  log::internal::ipc};

         // so we don't hang for ever, if something is wrong...
         common::signal::timer::Scoped timout( std::chrono::seconds( 5));

         mockup::ipc::Instance instance;


         {
            trace::Scope trace( "sender.add  200");
            message::service::lookup::Request request;
            request.requested = "someService";
            request.process = process::handle();

            const std::string temp = "service_";

            for( int count = 0; count < 200; ++count)
            {
               request.requested = temp + std::to_string( count);
               communication::ipc::blocking::send( instance.input(), request);
            }
         }

         {
            trace::Scope trace( "read( ipc::receive::queue())  200");

            message::service::lookup::Request request;

            const std::string temp = "service_";

            for( int count = 0; count < 200; ++count)
            {
               const auto service = temp + std::to_string( count);

               communication::ipc::blocking::receive( instance.output(), request);

               EXPECT_TRUE( request.requested == service) << "want: " << request.requested << " have: " << service;
               EXPECT_TRUE( request.process.queue == communication::ipc::inbound::id());
            }
         }
      }

      TEST( casual_common_mockup, ipc_link_2_instances__send_200_messages)
      {

         // so we don't hang for ever, if something is wrong...
         common::signal::timer::Scoped timout( std::chrono::seconds( 5));

         mockup::ipc::Instance source;
         mockup::ipc::Instance destination;

         //
         // Link "output" of source to "input" of destination
         //
         mockup::ipc::Link link{ source.output().connector().id(), destination.input()};


         {
            message::service::lookup::Request request;
            request.requested = "someService";
            request.process = process::handle();

            const std::string temp = "service_";

            for( int count = 0; count < 200; ++count)
            {
               request.requested = temp + std::to_string( count);
               communication::ipc::blocking::send( source.input(), request);
            }
         }

         {
            trace::Scope trace( "read( ipc::receive::queue())  200");

            message::service::lookup::Request request;

            const std::string temp = "service_";

            for( int count = 0; count < 200; ++count)
            {
               const auto service = temp + std::to_string( count);

               communication::ipc::blocking::receive( destination.output(), request);

               EXPECT_TRUE( request.requested == service) << "want: " << request.requested << " have: " << service;
               EXPECT_TRUE( request.process.queue == communication::ipc::inbound::id());
            }
         }
      }


      TEST( casual_common_mockup, ipc_router_one_messages)
      {
         trace::Scope trace{ "TEST( casual_common_mockup, handle_router_one_messages)", log::internal::ipc};

         // so we don't hang for ever, if something is wrong...
         common::signal::timer::Scoped timout( std::chrono::seconds( 5));

         mockup::ipc::Router router{ communication::ipc::inbound::id()};

         {
            message::service::lookup::Request request;
            request.requested = "someService";
            request.process = process::handle();

            communication::ipc::blocking::send( router.input(), request);
         }

         {

            message::service::lookup::Request request;
            communication::ipc::blocking::receive( communication::ipc::inbound::device(), request);

            EXPECT_TRUE( request.requested == "someService");
            EXPECT_TRUE( request.process.queue == communication::ipc::inbound::id());

         }
      }


      TEST( casual_common_mockup, ipc_router_200_messages)
      {
         trace::Scope trace{ "TEST( casual_common_mockup, handle_router_200_messages)", log::internal::ipc};

         // so we don't hang for ever, if something is wrong...
         common::signal::timer::Scoped timout( std::chrono::seconds( 5));

         mockup::ipc::Router router{ communication::ipc::inbound::id()};

         //ipc::receive::queue().clear();

         {
            message::service::lookup::Request request;
            request.requested = "someService";
            request.process = process::handle();

            for( auto count = 0; count < 200; ++count)
            {
               communication::ipc::blocking::send( router.input(), request);
            }


            common::log::debug << "wrote 200 messages to " << router.input() << std::endl;
         }

         {
            for( auto count = 0; count < 200; ++count)
            {
               message::service::lookup::Request request;
               communication::ipc::blocking::receive( communication::ipc::inbound::device(), request);

               //EXPECT_TRUE( read( request)) << "count: " << count;
               EXPECT_TRUE( request.requested == "someService");
               EXPECT_TRUE( request.process.queue == communication::ipc::inbound::id());
            }
            common::log::debug << "environment::directory::domain(): " << environment::directory::domain() << std::endl;

         }
      }





   } // common
} // casual

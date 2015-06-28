//!
//! test_mockup.cpp
//!
//! Created on: May 25, 2014
//!     Author: Lazan
//!

#include <gtest/gtest.h>

#include "common/mockup/ipc.h"

#include "common/message/server.h"

#include "common/queue.h"
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
            message::service::name::lookup::Request request;
            request.requested = "someService";
            request.process = process::handle();

            queue::blocking::Writer write( instance.input());

            write( request);
         }

         {
            queue::blocking::Reader read( instance.output());
            message::service::name::lookup::Request request;
            read( request);

            EXPECT_TRUE( request.requested == "someService");
            EXPECT_TRUE( request.process.queue == ipc::receive::id());

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
         mockup::ipc::Link link{ source.output().id(), destination.input()};

         {
            message::service::name::lookup::Request request;
            request.requested = "someService";
            request.process = process::handle();

            queue::blocking::Writer write( source.input());

            write( request);
         }

         {
            queue::blocking::Reader read( destination.output());
            message::service::name::lookup::Request request;
            read( request);

            EXPECT_TRUE( request.requested == "someService");
            EXPECT_TRUE( request.process.queue == ipc::receive::id());

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
            message::service::name::lookup::Request request;
            request.requested = "someService";
            request.process = process::handle();

            queue::blocking::Writer write( instance.input());

            const std::string temp = "service_";

            for( int count = 0; count < 200; ++count)
            {
               request.requested = temp + std::to_string( count);
               write( request);
            }
         }

         {
            trace::Scope trace( "read( ipc::receive::queue())  200");

            queue::blocking::Reader read( instance.output());
            message::service::name::lookup::Request request;

            const std::string temp = "service_";

            for( int count = 0; count < 200; ++count)
            {
               const auto service = temp + std::to_string( count);

               read( request);

               EXPECT_TRUE( request.requested == service) << "want: " << request.requested << " have: " << service;
               EXPECT_TRUE( request.process.queue == ipc::receive::id());
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
         mockup::ipc::Link link{ source.output().id(), destination.input()};


         {
            message::service::name::lookup::Request request;
            request.requested = "someService";
            request.process = process::handle();

            queue::blocking::Writer write( source.input());

            const std::string temp = "service_";

            for( int count = 0; count < 200; ++count)
            {
               request.requested = temp + std::to_string( count);
               write( request);
            }
         }

         {
            trace::Scope trace( "read( ipc::receive::queue())  200");

            queue::blocking::Reader read( destination.output());
            message::service::name::lookup::Request request;

            const std::string temp = "service_";

            for( int count = 0; count < 200; ++count)
            {
               const auto service = temp + std::to_string( count);

               read( request);

               EXPECT_TRUE( request.requested == service) << "want: " << request.requested << " have: " << service;
               EXPECT_TRUE( request.process.queue == ipc::receive::id());
            }
         }
      }

      TEST( casual_common_mockup, ipc_router_one_messages)
      {
         trace::Scope trace{ "TEST( casual_common_mockup, handle_router_one_messages)", log::internal::ipc};

         // so we don't hang for ever, if something is wrong...
         common::signal::timer::Scoped timout( std::chrono::seconds( 5));

         mockup::ipc::Router router{ ipc::receive::id()};

         //ipc::receive::queue().clear();

         {
            message::service::name::lookup::Request request;
            request.requested = "someService";
            request.process = process::handle();

            queue::blocking::Writer write( router.input());
            write( request);
         }

         {

            queue::blocking::Reader read( ipc::receive::queue());
            message::service::name::lookup::Request request;

            read( request);
            EXPECT_TRUE( request.requested == "someService");
            EXPECT_TRUE( request.process.queue == ipc::receive::id());

         }
      }


      TEST( casual_common_mockup, ipc_router_200_messages)
      {
         trace::Scope trace{ "TEST( casual_common_mockup, handle_router_200_messages)", log::internal::ipc};

         // so we don't hang for ever, if something is wrong...
         common::signal::timer::Scoped timout( std::chrono::seconds( 5));

         mockup::ipc::Router router{ ipc::receive::id()};

         //ipc::receive::queue().clear();

         {
            message::service::name::lookup::Request request;
            request.requested = "someService";
            request.process = process::handle();

            queue::blocking::Writer write( router.input());

            for( auto count = 0; count < 200; ++count)
            {
               write( request);
            }


            common::log::debug << "wrote 200 messages to " << router.input() << std::endl;
         }

         {
            auto read = queue::blocking::reader( ipc::receive::queue());
            message::service::name::lookup::Request request;


            for( auto count = 0; count < 200; ++count)
            {

               read( request);
               //EXPECT_TRUE( read( request)) << "count: " << count;
               EXPECT_TRUE( request.requested == "someService");
               EXPECT_TRUE( request.process.queue == ipc::receive::id());
            }
            common::log::debug << "environment::directory::domain(): " << environment::directory::domain() << std::endl;

         }
      }





   } // common
} // casual

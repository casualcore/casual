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

      TEST( casual_common_mockup, handle_replier_startup)
      {
         EXPECT_NO_THROW({
            mockup::ipc::Sender sender;
         });
      }


      TEST( casual_common_mockup, handle_sender_one_message)
      {

         trace::Scope trace{ "TEST( casual_common_mockup, handle_sender_one_message)", log::internal::ipc};

         mockup::ipc::Sender sender;
         {
            message::service::name::lookup::Request request;
            request.requested = "someService";
            request.server = message::server::Id::current();

            sender.add( ipc::receive::id(), request);
         }

         {
            queue::blocking::Reader read( ipc::receive::queue());
            message::service::name::lookup::Request request;
            read( request);

            EXPECT_TRUE( request.requested == "someService");
            EXPECT_TRUE( request.server.queue_id == ipc::receive::id());

         }
      }

      TEST( casual_common_mockup, handle_sender_200_messages)
      {
         trace::Scope trace{ "TEST( casual_common_mockup, handle_sender_200_messages)",  log::internal::ipc};

         mockup::ipc::Sender sender;
         //
         // We use a receiver to not reach ipc-limits.
         mockup::ipc::Receiver receiver;

         {
            trace::Scope trace( "sender.add  200");
            message::service::name::lookup::Request request;
            request.requested = "someService";
            request.server = message::server::Id::current();

            for( int count = 0; count < 200; ++count)
            {
               sender.add( receiver.id(), request);
            }
         }

         {

            trace::Scope trace( "read( ipc::receive::queue())  200");

            auto read = queue::blocking::reader( receiver);
            message::service::name::lookup::Request request;

            for( int count = 0; count < 200; ++count)
            {
               read( request);
               EXPECT_TRUE( request.requested == "someService");
               EXPECT_TRUE( request.server.queue_id == ipc::receive::id());
            }
         }
      }

      TEST( casual_common_mockup, handle_reciver_one_messages)
      {
         trace::Scope trace{ "TEST( casual_common_mockup, handle_reciver_one_messages)", log::internal::ipc};

         mockup::ipc::Receiver receiver;

         {
            message::service::name::lookup::Request request;
            request.requested = "someService";
            request.server = message::server::Id::current();

            queue::blocking::Writer write( receiver.id());
            write( request);
         }

         {
            auto read = queue::blocking::reader( receiver);
            message::service::name::lookup::Request request;

            read( request);
            EXPECT_TRUE( request.requested == "someService");
            EXPECT_TRUE( request.server.queue_id == ipc::receive::id());

         }
      }

      TEST( casual_common_mockup, handle_reciver_200_messages)
      {
         trace::Scope trace{ "TEST( casual_common_mockup, handle_reciver_200_messages)", log::internal::ipc};

         mockup::ipc::Receiver receiver;

         {
            message::service::name::lookup::Request request;
            request.requested = "someService";
            request.server = message::server::Id::current();

            queue::blocking::Writer write( receiver.id());

            for( auto count = 0; count < 200; ++count)
               write( request);

            common::log::debug << "wrote 200 messages to " << receiver.id() << std::endl;

         }

         {
            auto read = queue::blocking::reader( receiver);
            message::service::name::lookup::Request request;

            for( auto count = 0; count < 200; ++count)
            {
               read( request);
               //EXPECT_TRUE( read( request)) << "count: " << count;
               EXPECT_TRUE( request.requested == "someService");
               EXPECT_TRUE( request.server.queue_id == ipc::receive::id());
            }

            common::log::debug << "environment::directory::domain(): " << environment::directory::domain() << std::endl;

         }
      }




   } // common
} // casual

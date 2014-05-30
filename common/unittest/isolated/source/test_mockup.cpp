//!
//! test_mockup.cpp
//!
//! Created on: May 25, 2014
//!     Author: Lazan
//!

#include <gtest/gtest.h>

#include "common/mockup/ipc.h"
#include "common/queue.h"
#include "common/log.h"

#include "common/signal.h"


#include "common/environment.h"


namespace casual
{
   namespace common
   {

      TEST( casual_common_mockup, handle_replier_startup)
      {
         signal::clear();

         mockup::ipc::Sender replie1;

      }


      TEST( casual_common_mockup, handle_sender_one_message)
      {
         signal::clear();

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
         signal::clear();

         mockup::ipc::Sender sender;

         {
            message::service::name::lookup::Request request;
            request.requested = "someService";
            request.server = message::server::Id::current();

            for( int count = 0; count < 200; ++count)
            {
               sender.add( ipc::receive::id(), request);
            }
         }

         {
            queue::blocking::Reader read( ipc::receive::queue());
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
         signal::clear();

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
         signal::clear();

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

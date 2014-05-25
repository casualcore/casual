//!
//! casual_isolatedunittest_queue.cpp
//!
//! Created on: Jun 9, 2012
//!     Author: Lazan
//!

#include <gtest/gtest.h>

#include "common/queue.h"
#include "common/ipc.h"
#include "common/message.h"
#include "common/exception.h"
#include "common/signal.h"

//temp



namespace casual
{
   namespace common
   {

      namespace queue
      {

         TEST( casual_common, queue_writer_blocking_reader)
         {
            ipc::receive::Queue receive;

            //ipc::send::Queue send( receive.id());

            {
               blocking::Writer writer( receive.id());
               message::service::Advertise message;

               message.server.queue_id = 666;
               message.serverPath = "banan";

               message::Service service;
               service.name = "korv";
               message.services.push_back( service);

               writer( message);
            }

            {
               blocking::Reader reader( receive);

               auto marshal = reader.next();

               EXPECT_TRUE( marshal.type() == message::service::Advertise::message_type);

               message::service::Advertise message;
               marshal >> message;

               EXPECT_TRUE( message.server.queue_id == 666);
               EXPECT_TRUE( message.serverPath == "banan");

               ASSERT_TRUE( message.services.size() == 1);
               EXPECT_TRUE( message.services.front().name == "korv");
            }

         }

         TEST( casual_common, queue_reader_timeout)
         {
            ipc::receive::Queue receive;
            blocking::Reader reader( receive);

            common::signal::alarm::Scoped timeout( 1);

            message::service::Advertise message;

            EXPECT_THROW({
               reader( message);
            }, common::exception::signal::Timeout);


         }

         TEST( casual_common, queue_non_blocking_reader_no_messages)
         {
            ipc::receive::Queue receive;
            non_blocking::Reader reader( receive);

            message::service::Advertise message;

            EXPECT_FALSE( reader( message));

         }

         TEST( casual_common, queue_non_blocking_reader_message)
         {
            ipc::receive::Queue receive;
            non_blocking::Reader reader( receive);

            //ipc::send::Queue send( receive.id());
            blocking::Writer writer( receive.id());

            message::service::Advertise sendMessage;
            sendMessage.serverPath = "banan";
            writer( sendMessage);

            message::service::Advertise receiveMessage;
            EXPECT_TRUE( reader( receiveMessage));
            EXPECT_TRUE( receiveMessage.serverPath == "banan");

         }

         TEST( casual_common, queue_non_blocking_reader_big_message)
         {
            ipc::receive::Queue receive;
            non_blocking::Reader reader( receive);

            //ipc::send::Queue send( receive.id());
            blocking::Writer writer( receive.id());

            message::service::Advertise sendMessage;
            sendMessage.serverPath = "banan";
            sendMessage.services.resize( 50);
            writer( sendMessage);

            message::service::Advertise receiveMessage;
            EXPECT_TRUE( reader( receiveMessage));
            EXPECT_TRUE( receiveMessage.serverPath == "banan");
            EXPECT_TRUE( receiveMessage.services.size() == 50);

         }
      }
   }
}




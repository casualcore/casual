//!
//! casual_isolatedunittest_queue.cpp
//!
//! Created on: Jun 9, 2012
//!     Author: Lazan
//!

#include <gtest/gtest.h>

#include "common/queue.h"
#include "common/ipc.h"
#include "common/mockup/ipc.h"
#include "common/message/service.h"
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

               message.process.queue = 666;
               message.serverPath = "banan";

               message::Service service;
               service.name = "korv";
               message.services.push_back( service);

               writer( message);
            }

            {
               blocking::Reader reader( receive);

               auto marshal = reader.next();

               EXPECT_TRUE( marshal.type == message::service::Advertise::message_type);

               message::service::Advertise message;
               marshal >> message;

               EXPECT_TRUE( message.process.queue == 666);
               EXPECT_TRUE( message.serverPath == "banan");

               ASSERT_TRUE( message.services.size() == 1);
               EXPECT_TRUE( message.services.front().name == "korv");
            }

         }

         TEST( casual_common, queue_reader_timeout_2ms)
         {
            ipc::receive::Queue receive;
            blocking::Reader reader( receive);

            common::signal::timer::Scoped timeout( std::chrono::milliseconds( 2));

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

            blocking::Writer writer( receive.id());

            message::service::Advertise sendMessage;
            sendMessage.serverPath = "banan";
            sendMessage.services.resize( 30);
            writer( sendMessage);

            message::service::Advertise receiveMessage;
            EXPECT_TRUE( reader( receiveMessage));
            EXPECT_TRUE( receiveMessage.serverPath == "banan");
            EXPECT_TRUE( receiveMessage.services.size() == 30);

         }

         TEST( casual_common, queue_non_blocking_reader_next_filter__no_messages)
         {
            {
               non_blocking::Writer send{ ipc::receive::id()};
               message::flush::IPC message;
               EXPECT_FALSE( send( message).empty());

            }

            non_blocking::Reader reader( ipc::receive::queue());

            auto result = reader.next( { message::Type::cTrafficMonitorConnect, message::Type::cTrafficEvent});

            EXPECT_TRUE( result.empty());
         }


         TEST( casual_common, queue_non_blocking_reader_next_filter__messages)
         {
            mockup::ipc::Router route{ ipc::receive::id()};
            {
               blocking::Writer send{ route.input()};


               {
                  message::flush::IPC message;
                  send( message);
               }
               /*
               {
                  message::server::connect::Request message;
                  message.services.resize( 100);
                  send( message);
               }
               */
               {
                  message::service::Advertise message;
                  message.serverPath = "banan";
                  message.services.resize( 50);
                  send( message);
               }

            }

            blocking::Reader reader( ipc::receive::queue());

            auto result = reader.next( { message::Type::cTrafficMonitorConnect, message::service::Advertise::message_type});
            EXPECT_TRUE( result.type == message::service::Advertise::message_type);
         }


      } // queue
   } // common
} // casual




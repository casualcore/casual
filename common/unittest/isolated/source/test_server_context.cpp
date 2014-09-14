//!
//! test_server_context.cpp
//!
//! Created on: Dec 8, 2012
//!     Author: Lazan
//!

#include <gtest/gtest.h>


#include "common/server_context.h"
#include "common/process.h"

#include "common/mockup/ipc.h"
#include "common/buffer_context.h"


// we need some values
#include <xatmi.h>


#include <fstream>


extern "C"
{

   int tpsvrinit(int argc, char **argv)
   {
      return 0;
   }

   void tpsvrdone()
   {

   }
}


namespace casual
{
   namespace common
   {
      namespace local
      {


         namespace
         {

            const std::string& replyMessage()
            {
               static const std::string reply( "reply messsage");
               return reply;
            }

            void test_service( TPSVCINFO *serviceInfo)
            {

               auto buffer = buffer::Context::instance().allocate( {"X_OCTET", ""}, 1024);

               std::copy( replyMessage().begin(), replyMessage().end(), buffer);
               buffer[ replyMessage().size()] = '\0';

               server::Context::instance().longJumpReturn( TPSUCCESS, 0, buffer, replyMessage().size(), 0);
            }

            server::Arguments arguments()
            {
               server::Arguments arguments{ { "/test/path"}};

               arguments.services.emplace_back( "test_service", &test_service, 0, server::Service::cNone);

               return arguments;
            }


            message::service::callee::Call callMessage( platform::queue_id_type id)
            {
               message::service::callee::Call message;

               message.buffer = { { "X_OCTET", ""}, 1024};
               message.callDescriptor = 10;
               message.service.name = "test_service";
               message.reply.queue_id = id;

               return message;
            }


            namespace sender
            {
               mockup::ipc::Sender& queue()
               {
                  static mockup::ipc::Sender singleton;
                  return singleton;
               }

            } // sender

            namespace transaction
            {
               namespace manager
               {
                  mockup::ipc::Receiver& queue()
                  {
                     static mockup::ipc::Receiver singleton;
                     return singleton;
                  }

               } // manager
            } // transaction

            namespace broker
            {
               void prepare()
               {
                  // server connect
                  {
                     message::server::connect::Reply reply;
                     local::sender::queue().add( ipc::receive::id(), reply);
                  }

                  // transaction client connect
                  {
                     message::transaction::client::connect::Reply reply;
                     reply.domain = "unittest-domain";
                     reply.transactionManagerQueue = transaction::manager::queue().id();
                     local::sender::queue().add( ipc::receive::id(), reply);
                  }

               }
            } // broker

         } // <unnamed>
      } // local


      TEST( casual_common_service_context, arguments)
      {
         server::Arguments arguments{ { "arg1", "arg2"}};

         ASSERT_TRUE( arguments.argc == 2);
         EXPECT_TRUE( arguments.argv[ 0] == std::string( "arg1"));
         EXPECT_TRUE( arguments.argv[ 1] == std::string( "arg2"));

         EXPECT_TRUE( arguments.arguments.at( 0) == "arg1");
         EXPECT_TRUE( arguments.arguments.at( 1) == "arg2");
      }

      TEST( casual_common_service_context, arguments_move)
      {
         server::Arguments origin{ { "arg1", "arg2"}};

         server::Arguments arguments = std::move( origin);

         ASSERT_TRUE( arguments.argc == 2);
         EXPECT_TRUE( arguments.argv[ 0] == std::string( "arg1"));
         EXPECT_TRUE( arguments.argv[ 1] == std::string( "arg2"));

         EXPECT_TRUE( arguments.arguments.at( 0) == "arg1");
         EXPECT_TRUE( arguments.arguments.at( 1) == "arg2");
      }




      TEST( casual_common_service_context, connect)
      {
         auto bq = mockup::ipc::broker::id();

         //
         // Prepare "broker response"
         //
         {
            local::broker::prepare();
         }

         callee::handle::Call callHandler( local::arguments());

         auto reader = queue::blocking::reader( mockup::ipc::broker::queue());
         message::server::connect::Request message;

         reader( message);

         EXPECT_TRUE( message.server.pid == process::id());
         EXPECT_TRUE( message.server.queue_id == ipc::receive::id());

         ASSERT_TRUE( message.services.size() == 1);
         EXPECT_TRUE( message.services.at( 0).name == "test_service");


         mockup::ipc::broker::queue().clear();
         ipc::receive::queue().clear();
      }

      TEST( casual_common_service_context, disconnect)
      {
         {
            local::broker::prepare();
            callee::handle::Call callHandler( local::arguments());
         }

         auto broker = queue::blocking::reader( mockup::ipc::broker::queue());
         message::server::Disconnect message;
         broker( message);

         EXPECT_TRUE( message.server.pid == process::id());

         mockup::ipc::broker::queue().clear();
         ipc::receive::queue().clear();

      }

      TEST( casual_common_service_context, call_service__gives_reply)
      {
         mockup::ipc::Receiver receiver;

         {
            local::broker::prepare();
            callee::handle::Call callHandler( local::arguments());

            auto message = local::callMessage( receiver.id());
            callHandler.dispatch( message);
         }


         auto reader = queue::blocking::reader( receiver);
         message::service::Reply message;

         reader( message);
         EXPECT_TRUE( message.buffer.raw() == local::replyMessage());

         mockup::ipc::broker::queue().clear();
         ipc::receive::queue().clear();
      }

      TEST( casual_common_service_context, call_service__gives_broker_ack)
      {
         mockup::ipc::Receiver receiver;
         {
            local::broker::prepare();
            callee::handle::Call callHandler( local::arguments());

            auto message = local::callMessage( receiver.id());
            callHandler.dispatch( message);
         }

         auto broker = queue::blocking::reader( mockup::ipc::broker::queue());
         message::service::ACK message;

         broker( message);
         EXPECT_TRUE( message.service == "test_service");
         EXPECT_TRUE( message.server.queue_id == ipc::receive::id());

         mockup::ipc::broker::queue().clear();
         ipc::receive::queue().clear();

      }

      TEST( casual_common_service_context, call_non_existing_service__throws)
      {
         mockup::ipc::Receiver receiver;

         local::broker::prepare();
         callee::handle::Call callHandler( local::arguments());

         auto message = local::callMessage( receiver.id());
         message.service.name = "non_existing";


         EXPECT_THROW( {
            callHandler.dispatch( message);
         }, exception::xatmi::SystemError);

         mockup::ipc::broker::queue().clear();
         ipc::receive::queue().clear();
      }


      TEST( casual_common_service_context, call_service__gives_monitor_notify)
      {
         mockup::ipc::Receiver receiver;
         mockup::ipc::Receiver monitor;

         {
            local::broker::prepare();

            callee::handle::Call callHandler( local::arguments());

            auto message = local::callMessage( receiver.id());
            message.service.monitor_queue = monitor.id();
            callHandler.dispatch( message);
         }

         auto reader = queue::blocking::reader( monitor);

         message::monitor::Notify message;
         reader( message);
         EXPECT_TRUE( message.service == "test_service");

         mockup::ipc::broker::queue().clear();
         ipc::receive::queue().clear();
      }

   } // common
} // casual




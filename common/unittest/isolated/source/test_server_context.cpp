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
#include "common/buffer/pool.h"


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

               auto buffer = buffer::pool::Holder::instance().allocate( {"X_OCTET", "binary"}, 1024);

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

               message.buffer = { { "X_OCTET", "binary"}, platform::binary_type( 1024)};
               message.callDescriptor = 10;
               message.service.name = "test_service";
               message.reply.queue = id;

               return message;
            }



            namespace broker
            {
               void prepare( platform::queue_id_type id)
               {
                  common::queue::blocking::Writer send( id);
                  // server connect
                  {
                     message::server::connect::Reply reply;
                     send( reply);
                  }

                  // transaction client connect
                  {
                     message::transaction::client::connect::Reply reply;
                     reply.domain = "unittest-domain";
                     reply.transactionManagerQueue = mockup::ipc::transaction::manager::queue().id();
                     send( reply);
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
         mockup::ipc::clear();

         mockup::ipc::Router router{ ipc::receive::id()};

         //
         // Prepare "broker response"
         //
         {
            local::broker::prepare( router.id());
         }

         callee::handle::Call callHandler( local::arguments());

         message::server::connect::Request message;

         queue::blocking::Reader read( mockup::ipc::broker::queue().receive());
         read( message);

         EXPECT_TRUE( message.process == process::handle());

         ASSERT_TRUE( message.services.size() == 1);
         EXPECT_TRUE( message.services.at( 0).name == "test_service");

      }


      TEST( casual_common_service_context, disconnect)
      {
         trace::internal::Scope trace( "casual_common_service_context, disconnect");

         mockup::ipc::clear();

         mockup::ipc::Router router{ ipc::receive::id()};

         {
            local::broker::prepare( router.id());
            callee::handle::Call callHandler( local::arguments());
         }

         auto reader = queue::blocking::reader( mockup::ipc::broker::queue().receive());
         message::server::Disconnect message;
         reader( message);

         EXPECT_TRUE( message.process.pid == process::id());


      }




      TEST( casual_common_service_context, call_service__gives_reply)
      {
         mockup::ipc::clear();

         // just a cache to keep queue writable
         mockup::ipc::Router router{ ipc::receive::id()};

         // instance that "calls" a service in callee::handle::Call, and get's a reply
         mockup::ipc::Instance caller( 10);

         {
            local::broker::prepare( router.id());
            callee::handle::Call callHandler( local::arguments());

            auto message = local::callMessage( caller.id());
            callHandler.dispatch( message);
         }


         queue::blocking::Reader reader( caller.receive());
         message::service::Reply message;

         reader( message);
         EXPECT_TRUE( message.buffer.memory.data() == local::replyMessage());

      }


      TEST( casual_common_service_context, call_service__gives_broker_ack)
      {
         mockup::ipc::clear();

         // just a cache to keep queue writable
         mockup::ipc::Router router{ ipc::receive::id()};

         // instance that "calls" a service in callee::handle::Call, and get's a reply
         mockup::ipc::Instance caller( 10);

         {
            local::broker::prepare( router.id());
            callee::handle::Call callHandler( local::arguments());

            auto message = local::callMessage( caller.id());
            callHandler.dispatch( message);
         }

         queue::blocking::Reader broker( mockup::ipc::broker::queue().receive());
         message::service::ACK message;

         broker( message);
         EXPECT_TRUE( message.service == "test_service");
         EXPECT_TRUE( message.process.queue == ipc::receive::id());
      }


      TEST( casual_common_service_context, call_non_existing_service__throws)
      {
         mockup::ipc::clear();

         // just a cache to keep queue writable
         mockup::ipc::Router router{ ipc::receive::id()};

         // instance that "calls" a service in callee::handle::Call, and get's a reply
         mockup::ipc::Instance caller( 10);

         local::broker::prepare( router.id());
         callee::handle::Call callHandler( local::arguments());

         auto message = local::callMessage( caller.id());
         message.service.name = "non_existing";


         EXPECT_THROW( {
            callHandler.dispatch( message);
         }, exception::xatmi::SystemError);
      }



      TEST( casual_common_service_context, call_service__gives_monitor_notify)
      {
         mockup::ipc::clear();

         // just a cache to keep queue writable
         mockup::ipc::Router router{ ipc::receive::id()};

         // instance that "calls" a service in callee::handle::Call, and get's a reply
         mockup::ipc::Instance caller( 10);

         mockup::ipc::Instance monitor( 42);

         {
            local::broker::prepare( router.id());

            callee::handle::Call callHandler( local::arguments());

            auto message = local::callMessage( caller.id());
            message.service.monitor_queue = monitor.id();
            callHandler.dispatch( message);
         }

         queue::blocking::Reader reader( monitor.receive());

         message::monitor::Notify message;
         reader( message);
         EXPECT_TRUE( message.service == "test_service");

         mockup::ipc::broker::queue().clear();
         ipc::receive::queue().clear();
      }

      TEST( casual_common_service_context, state_call_descriptor_reserver)
      {
         calling::State state;

         auto first = state.pending.reserve();
         auto second = state.pending.reserve();

         EXPECT_TRUE( first < second);
      }

   } // common
} // casual




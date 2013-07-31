//!
//! test_server_context.cpp
//!
//! Created on: Dec 8, 2012
//!     Author: Lazan
//!

#include <gtest/gtest.h>


#include "common/server_context.h"

#include "common/mockup.h"

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
            struct Policy
            {
               typedef mockup::queue::WriteMessage< message::service::Reply> reply_queue;
               typedef mockup::queue::WriteMessage< message::server::Connect> connect_queue;
               typedef mockup::queue::WriteMessage< message::service::ACK> ack_queue;
               typedef mockup::queue::WriteMessage< message::monitor::Notify> monitor_queue;
               typedef mockup::queue::WriteMessage< message::server::Disconnect> disconnect_queue;
               typedef mockup::queue::ReadMessage< message::server::Configuration> configuration_queue;


               typedef queue::ipc_wrapper< reply_queue> reply_writer;
               typedef queue::ipc_wrapper< monitor_queue> monitor_writer;

            private:

               typedef queue::ipc_wrapper< connect_queue> connect_writer;
               typedef queue::ipc_wrapper< ack_queue> ack_writer;
               typedef queue::ipc_wrapper< disconnect_queue> non_blocking_broker_writer;
               typedef queue::ipc_wrapper< configuration_queue> configuration_reader;

            public:

               static void reset()
               {
                  mockup::queue::WriteMessage< message::service::Reply>::reset();
                  mockup::queue::WriteMessage< message::server::Connect>::reset();
                  mockup::queue::WriteMessage< message::service::ACK>::reset();
                  mockup::queue::WriteMessage< message::server::Disconnect>::reset();

                  mockup::queue::ReadMessage< message::server::Configuration>::reset();

                  // reset global TM queue
                  transaction::Context::instance().state().transactionManagerQueue = 0;

                  // prep the configuration reply - only message we will read
                  message::server::Configuration message;
                  message.transactionManagerQueue = 666;
                  mockup::queue::ReadMessage< message::server::Configuration>::replies.push_back( std::move( message));
               }


               message::server::Configuration connect( message::server::Connect& message)
               {
                  //
                  // Let the broker know about us, and our services...
                  //

                  message.server.queue_id = 500;
                  message.path = "test/path";

                  connect_writer brokerWriter;
                  brokerWriter( message);


                  //
                  // Wait for configuration reply
                  //
                  configuration_reader reader( 500);
                  message::server::Configuration configuration;
                  reader( configuration);

                  return configuration;
               }

               void disconnect()
               {
                  message::server::Disconnect message;

                  //
                  // we can't block here...
                  //
                  non_blocking_broker_writer brokerWriter;
                  brokerWriter( message);
               }


               void ack( const message::service::callee::Call& message)
               {
                  message::service::ACK ack;
                  ack.server.queue_id = 500;
                  ack.service = message.service.name;

                  ack_writer brokerWriter;
                  brokerWriter( ack);
               }
            };

            typedef common::callee::handle::basic_call< Policy> Call;


            const std::string& replyMessage()
            {
               static const std::string reply( "reply messsage");
               return reply;
            }

            void test_service( TPSVCINFO *serviceInfo)
            {

               auto buffer = buffer::Context::instance().allocate( "X_OCTET", "STRING", 1024);

               std::copy( replyMessage().begin(), replyMessage().end(), buffer);
               buffer[ replyMessage().size()] = '\0';

               server::Context::instance().longJumpReturn( TPSUCCESS, 0, buffer, replyMessage().size(), 0);
            }



            server::Arguments arguments()
            {
               server::Arguments arguments;

               arguments.m_services.emplace_back( "test_service", &test_service);

               arguments.m_argc = 1;

               static const char* path{ "/test/path"};
               arguments.m_argv = &const_cast< char*&>( path);

               return arguments;
            }


            message::service::callee::Call callMessage()
            {
               message::service::callee::Call message;

               message.buffer = { "STRING", "", 1024};
               message.callDescriptor = 10;
               message.service.name = "test_service";

               return message;
            }

            struct ScopedBrokerQueue
            {
               ScopedBrokerQueue()
               {
                  if( common::file::exists( common::environment::getBrokerQueueFileName()))
                  {
                     throw exception::QueueFailed( "Broker queue file exists - Can't run tests within an existing casual domain");
                  }

                  path.reset( new file::ScopedPath( common::environment::getBrokerQueueFileName()));

                  std::ofstream out( common::environment::getBrokerQueueFileName());
                  out << brokerQueue.id();

               }

               ScopedBrokerQueue( ScopedBrokerQueue&&) = default;

            private:

               std::unique_ptr< file::ScopedPath> path;
               ipc::receive::Queue brokerQueue;
            };

         } // <unnamed>
      } // local



      TEST( casual_common_service_context, connect)
      {
         local::Policy::reset();

         auto arguments = local::arguments();

         local::Call callHandler( arguments);

         ASSERT_TRUE( local::Policy::connect_queue::replies.size() == 1);
         EXPECT_TRUE( local::Policy::connect_queue::replies.front().server.queue_id == 500);


      }


      TEST( casual_common_service_context, connect_reply)
      {
         local::Policy::reset();

         auto arguments = local::arguments();

         local::Call callHandler( arguments);

         EXPECT_TRUE( transaction::Context::instance().state().transactionManagerQueue == 666);

      }

      TEST( casual_common_service_context, disconnect)
      {
         local::Policy::reset();
         auto arguments = local::arguments();

         {
            local::Call callHandler( arguments);
         }

         ASSERT_TRUE( local::Policy::disconnect_queue::replies.size() == 1);
         EXPECT_TRUE( local::Policy::disconnect_queue::replies.front().server.pid == process::id());

      }

      TEST( casual_common_service_context, call_service__gives_reply)
      {
         local::Policy::reset();
         local::ScopedBrokerQueue scopedQueue;

         auto arguments = local::arguments();
         local::Call callHandler( arguments);

         auto message = local::callMessage();
         callHandler.dispatch( message);


         ASSERT_TRUE( local::Policy::reply_queue::replies.size() == 1);
         EXPECT_TRUE( local::Policy::reply_queue::replies.front().buffer.raw() == local::replyMessage());
      }

      TEST( casual_common_service_context, call_service__gives_broker_ack)
      {
         local::Policy::reset();
         local::ScopedBrokerQueue scopedQueue;

         auto arguments = local::arguments();
         local::Call callHandler( arguments);

         auto message = local::callMessage();
         callHandler.dispatch( message);


         ASSERT_TRUE( local::Policy::ack_queue::replies.size() == 1);
         EXPECT_TRUE( local::Policy::ack_queue::replies.front().service == "test_service");
         EXPECT_TRUE( local::Policy::ack_queue::replies.front().server.queue_id == 500);

      }

      TEST( casual_common_service_context, call_non_existing_service__throws)
      {
         local::Policy::reset();
         local::ScopedBrokerQueue scopedQueue;

         auto arguments = local::arguments();
         local::Call callHandler( arguments);

         auto message = local::callMessage();
         message.service.name = "non_existing";


         EXPECT_THROW( {
            callHandler.dispatch( message);
         }, exception::xatmi::SystemError);
      }


      TEST( casual_common_service_context, call_service__gives_monitor_notify)
      {
         local::Policy::reset();
         local::ScopedBrokerQueue scopedQueue;

         auto arguments = local::arguments();
         local::Call callHandler( arguments);

         auto message = local::callMessage();
         message.service.monitor_queue = 888;
         callHandler.dispatch( message);


         ASSERT_TRUE( local::Policy::monitor_queue::replies.size() == 1);
         EXPECT_TRUE( local::Policy::monitor_queue::replies.front().service == "test_service");
      }

   } // common
} // casual




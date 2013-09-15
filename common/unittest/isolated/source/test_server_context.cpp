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
               template< typename base_type>
               using reader_q = queue::ipc_wrapper< queue::blocking::basic_reader< queue::policy::NoAction, base_type>>;

               template< typename base_type>
               using writer_q = queue::ipc_wrapper< queue::blocking::basic_writer< queue::policy::NoAction, base_type>>;




               typedef mockup::queue::blocking::base_writer< message::service::Reply> reply_queue;
               typedef mockup::queue::blocking::base_writer< message::server::Connect> connect_queue;
               typedef mockup::queue::blocking::base_writer< message::service::ACK> ack_queue;
               typedef mockup::queue::blocking::base_writer< message::monitor::Notify> monitor_queue;
               typedef mockup::queue::blocking::base_writer< message::server::Disconnect> disconnect_queue;
               typedef mockup::queue::blocking::base_reader< message::server::Configuration> configuration_queue;


               typedef writer_q< reply_queue> reply_writer;
               typedef writer_q< monitor_queue> monitor_writer;

            private:

               typedef writer_q< connect_queue> connect_writer;
               typedef writer_q< ack_queue> ack_writer;
               typedef writer_q< disconnect_queue> non_blocking_broker_writer;
               typedef reader_q< configuration_queue> configuration_reader;

            public:

               static void reset()
               {
                  reply_queue::reset();
                  connect_queue::reset();
                  ack_queue::reset();
                  disconnect_queue::reset();
                  configuration_queue::reset();

                  // reset global TM queue
                  transaction::Context::instance().state().transactionManagerQueue = 0;

                  // prep the configuration reply - only message we will read
                  message::server::Configuration message;
                  message.transactionManagerQueue = 666;
                  configuration_queue::queue.push_back( std::move( message));
               }


               message::server::Configuration connect( message::server::Connect& message)
               {
                  //
                  // Let the broker know about us, and our services...
                  //

                  message.server.queue_id = 500;
                  message.path = "test/path";

                  connect_writer brokerWriter{ 10};
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
                  non_blocking_broker_writer brokerWriter{ 10};
                  brokerWriter( message);
               }

               void reply( platform::queue_id_type id, message::service::Reply& message)
               {
                  reply_writer writer{ 10};
                  writer( message);
               }



               void ack( const message::service::callee::Call& message)
               {
                  message::service::ACK ack;
                  ack.server.queue_id = 500;
                  ack.service = message.service.name;

                  ack_writer brokerWriter{ 10};
                  brokerWriter( ack);
               }

               void transaction( const message::service::callee::Call& message)
               {

               }

               void transaction( const message::service::Reply& message)
               {

               }

               void statistics( platform::queue_id_type id, message::monitor::Notify& message)
               {
                  monitor_writer writer{ id};
                  writer( message);
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
                  if( common::file::exists( common::environment::file::brokerQueue()))
                  {
                     throw exception::QueueFailed( "Broker queue file exists - Can't run tests within an existing casual domain");
                  }

                  path.reset( new file::ScopedPath( common::environment::file::brokerQueue()));

                  std::ofstream out( common::environment::file::brokerQueue());
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

         ASSERT_TRUE( local::Policy::connect_queue::queue.size() == 1);
         EXPECT_TRUE( local::Policy::connect_queue::queue.front().server.queue_id == 500);


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

         ASSERT_TRUE( local::Policy::disconnect_queue::queue.size() == 1);
         EXPECT_TRUE( local::Policy::disconnect_queue::queue.front().server.pid == process::id());

      }

      TEST( casual_common_service_context, call_service__gives_reply)
      {
         local::Policy::reset();
         local::ScopedBrokerQueue scopedQueue;

         auto arguments = local::arguments();
         local::Call callHandler( arguments);

         auto message = local::callMessage();
         callHandler.dispatch( message);


         ASSERT_TRUE( local::Policy::reply_queue::queue.size() == 1);
         EXPECT_TRUE( local::Policy::reply_queue::queue.front().buffer.raw() == local::replyMessage());
      }

      TEST( casual_common_service_context, call_service__gives_broker_ack)
      {
         local::Policy::reset();
         local::ScopedBrokerQueue scopedQueue;

         auto arguments = local::arguments();
         local::Call callHandler( arguments);

         auto message = local::callMessage();
         callHandler.dispatch( message);


         ASSERT_TRUE( local::Policy::ack_queue::queue.size() == 1);
         EXPECT_TRUE( local::Policy::ack_queue::queue.front().service == "test_service");
         EXPECT_TRUE( local::Policy::ack_queue::queue.front().server.queue_id == 500);

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


         ASSERT_TRUE( local::Policy::monitor_queue::queue.size() == 1);
         EXPECT_TRUE( local::Policy::monitor_queue::queue.front().service == "test_service");
      }

   } // common
} // casual




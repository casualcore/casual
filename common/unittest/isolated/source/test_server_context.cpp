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

               typedef mockup::queue::blocking::Writer writer_queue;
               typedef mockup::queue::non_blocking::Reader reader_queue;


               struct id
               {
                  static common::platform::queue_id_type instance() { return 10;}
                  static common::platform::queue_id_type broker() { return 1;}
                  static common::platform::queue_id_type monitor() { return 500;}
               };


               static void reset()
               {
                  mockup::queue::clearAllQueues();

                  // reset global TM queue
                  //transaction::Context::instance().state().transactionManagerQueue = 0;

                  // prep the configuration reply - only message we will read
                  message::server::Configuration message;
                  message.transactionManagerQueue = 666;

                  writer_queue instanceQ( id::instance());
                  instanceQ( message);
               }


               message::server::Configuration connect( message::server::Connect& message)
               {
                  //
                  // Let the broker know about us, and our services...
                  //

                  message.server.queue_id = id::instance();
                  message.path = "test/path";

                  writer_queue brokerWriter{ id::broker()};
                  brokerWriter( message);


                  //
                  // Wait for configuration reply
                  //
                  reader_queue reader{ id::instance()};
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
                  writer_queue brokerWriter{ id::broker()};
                  brokerWriter( message);
               }

               void reply( platform::queue_id_type id, message::service::Reply& message)
               {
                  writer_queue writer{ id};
                  writer( message);
               }



               void ack( const message::service::callee::Call& message)
               {
                  message::service::ACK ack;
                  ack.server.queue_id = id::instance();
                  ack.service = message.service.name;

                  writer_queue brokerWriter{ id::broker()};
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
                  writer_queue writer{ id};
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

               auto buffer = buffer::Context::instance().allocate( {"X_OCTET", ""}, 1024);

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

               message.buffer = { { "X_OCTET", ""}, 1024};
               message.callDescriptor = 10;
               message.service.name = "test_service";
               message.reply.queue_id = Policy::id::instance();

               return message;
            }


         } // <unnamed>
      } // local



      TEST( casual_common_service_context, connect)
      {
         local::Policy::reset();

         auto arguments = local::arguments();

         local::Call callHandler( arguments);

         local::Policy::reader_queue broker{ local::Policy::id::broker()};
         message::server::Connect message;

         ASSERT_TRUE( broker( message));
         EXPECT_TRUE( message.server.queue_id == local::Policy::id::instance());
      }


      TEST( casual_common_service_context, connect_reply)
      {
         local::Policy::reset();

         auto arguments = local::arguments();

         local::Call callHandler( arguments);

         EXPECT_TRUE( transaction::Context::instance().state().manager() == 666);

      }

      TEST( casual_common_service_context, disconnect)
      {
         local::Policy::reset();
         auto arguments = local::arguments();

         {
            local::Call callHandler( arguments);
         }

         local::Policy::reader_queue broker{ local::Policy::id::broker()};
         message::server::Disconnect message;

         ASSERT_TRUE( broker( message));
         EXPECT_TRUE( message.server.pid == process::id());

      }

      TEST( casual_common_service_context, call_service__gives_reply)
      {
         local::Policy::reset();
         //local::ScopedBrokerQueue scopedQueue;

         {
            auto arguments = local::arguments();
            local::Call callHandler( arguments);

            auto message = local::callMessage();
            callHandler.dispatch( message);
         }


         local::Policy::reader_queue reader{ local::Policy::id::instance()};
         message::service::Reply message;

         ASSERT_TRUE( reader( message));
         EXPECT_TRUE( message.buffer.raw() == local::replyMessage());
      }

      TEST( casual_common_service_context, call_service__gives_broker_ack)
      {
         local::Policy::reset();
         //local::ScopedBrokerQueue scopedQueue;

         {
            auto arguments = local::arguments();
            local::Call callHandler( arguments);

            auto message = local::callMessage();
            callHandler.dispatch( message);
         }

         local::Policy::reader_queue broker{ local::Policy::id::broker()};
         message::service::ACK message;


         ASSERT_TRUE( broker( message));
         EXPECT_TRUE( message.service == "test_service");
         EXPECT_TRUE( message.server.queue_id == local::Policy::id::instance());

      }

      TEST( casual_common_service_context, call_non_existing_service__throws)
      {
         local::Policy::reset();
         //local::ScopedBrokerQueue scopedQueue;

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
         //local::ScopedBrokerQueue scopedQueue;

         {
            auto arguments = local::arguments();
            local::Call callHandler( arguments);

            auto message = local::callMessage();
            message.service.monitor_queue = local::Policy::id::monitor();
            callHandler.dispatch( message);
         }

         local::Policy::reader_queue monitor{ local::Policy::id::monitor()};
         message::monitor::Notify message;

         ASSERT_TRUE( monitor( message));
         EXPECT_TRUE( message.service == "test_service");
      }

   } // common
} // casual




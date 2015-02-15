//!
//! casual_isolatedunittest_ipc.cpp
//!
//! Created on: May 1, 2012
//!     Author: Lazan
//!


#include <gtest/gtest.h>

#include "common/ipc.h"

#include "common/signal.h"
#include "common/exception.h"
#include "common/message/server.h"

#include "common/marshal/binary.h"


//temp



namespace casual
{
   namespace common
   {
      namespace ipc
      {
         namespace local
         {
            namespace
            {
               common::message::service::name::lookup::Request message()
               {
                  common::message::service::name::lookup::Request message;
                  message.process = process::handle();
                  message.requested = "service1";

                  return message;
               }

            } // <unnamed>
         } // local

         TEST( casual_common, ipc_queue_create)
         {
            EXPECT_NO_THROW({
               receive::Queue queue;
            });

         }

         TEST( casual_common, ipc_queue_send_receive)
         {

            receive::Queue receive;

            send::Queue send( receive.id());

            auto message = local::message();

            message::Complete complete = marshal::complete( message);

            EXPECT_TRUE( ! complete.correlation.empty());

            send( complete);

            auto response = receive( 0);

            EXPECT_TRUE( complete.payload == response.at( 0).payload) << complete.payload.size() << " : "  << response.at( 0).payload.at( 2);
         }

         TEST( casual_common, ipc_queue_send_receive_with_correlation)
         {

            receive::Queue receive;

            send::Queue send( receive.id());

            auto message = local::message();
            message::Complete complete = marshal::complete( message);

            auto correlation = send( complete);

            auto response = receive( correlation, 0);

            EXPECT_TRUE( complete.payload == response.at( 0).payload) << complete.payload.size() << " : "  << response.at( 0).payload.at( 2);
            EXPECT_TRUE( correlation == response.at( 0).correlation) << "correlation: " << correlation;
         }

         TEST( casual_common, ipc_queue_send_receive_discard_correlation__expect_no_message)
         {

            receive::Queue receive;

            send::Queue send( receive.id());

            auto message = local::message();
            message::Complete complete = marshal::complete( message);

            auto correlation = send( complete);

            //
            // Discard the message
            //
            receive.discard( complete.correlation);

            EXPECT_TRUE( receive( correlation, receive::Queue::cNoBlocking).empty());
         }


         TEST( casual_common, ipc_queue_receive_timeout_5ms)
         {
            receive::Queue receive;

            message::Transport response;

            common::signal::timer::Scoped timeout( std::chrono::milliseconds( 5));

            //
            // We don't expect to get any messages, and for the timeout to kick in
            // after 1s
            //
            EXPECT_THROW({
               receive( 0);
            }, common::exception::signal::Timeout);

         }

         TEST( casual_common, ipc_queue_send_receive_one_full_message)
         {

            receive::Queue receive;

            send::Queue send( receive.id());

            message::Complete complete( 2, uuid::make());
            complete.payload.assign( message::Transport::payload_max_size, 'A');

            EXPECT_TRUE( static_cast< bool>( complete.correlation));

            auto correlation = send( complete, send::Queue::cNoBlocking);

            ASSERT_TRUE( static_cast< bool>( correlation)) << "correlation: " << correlation;


            auto response = receive( 0);

            EXPECT_TRUE( complete.payload == response.at( 0).payload);
            // No more messages should be on the queue
            EXPECT_TRUE( receive( receive::Queue::cNoBlocking).empty());
         }

         TEST( casual_common, ipc_queue_send_receive_big_message)
         {

            receive::Queue receive;

            send::Queue send( receive.id());

            message::Complete complete{ 2, uuid::make()};
            complete.payload.assign( message::Transport::payload_max_size * 1.5, 'A');

            ASSERT_TRUE( static_cast< bool>( send( complete, send::Queue::cNoBlocking)));


            auto response = receive( 0);

            EXPECT_TRUE( complete.payload == response.at( 0).payload) << "complete.payload.size(): " << complete.payload.size() << " response.at( 0).payload.size(): " << response.at( 0).payload.size();
         }
      }
	}
}




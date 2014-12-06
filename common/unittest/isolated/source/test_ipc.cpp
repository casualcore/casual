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


//temp



namespace casual
{
   namespace common
   {
      namespace ipc
      {

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


            message::Complete transport{ 2, { 'A', 'B', 'C' } };

            send( transport);

            auto response = receive( 0);

            EXPECT_TRUE( transport.payload == response.at( 0).payload) << transport.payload.size() << " : "  << response.at( 0).payload.at( 2);
         }

         TEST( casual_common, ipc_queue_send_receive_with_correlation)
         {

            receive::Queue receive;

            send::Queue send( receive.id());


            message::Complete transport{ 2, { 'A', 'B', 'C' } };

            auto correlation = send( transport);

            auto response = receive( correlation, 0);

            EXPECT_TRUE( transport.payload == response.at( 0).payload) << transport.payload.size() << " : "  << response.at( 0).payload.at( 2);
            EXPECT_TRUE( correlation == response.at( 0).correlation) << "correlation: " << correlation;
         }


         TEST( casual_common, ipc_queue_receive_timeout_5ms)
         {
            receive::Queue receive;

            message::Transport response;

            common::signal::alarm::Scoped timeout( std::chrono::milliseconds( 5));

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

            message::Complete transport;
            transport.payload.assign( message::Transport::payload_max_size, 'A');
            transport.type = 2;

            EXPECT_TRUE( static_cast< bool>( transport.correlation));

            auto correlation = send( transport, send::Queue::cNoBlocking);

            ASSERT_TRUE( static_cast< bool>( correlation)) << "correlation: " << correlation;


            auto response = receive( 0);

            EXPECT_TRUE( transport.payload == response.at( 0).payload);
            // No more messages should be on the queue
            EXPECT_TRUE( receive( receive::Queue::cNoBlocking).empty());
         }

         TEST( casual_common, ipc_queue_send_receive_big_message)
         {

            receive::Queue receive;

            send::Queue send( receive.id());

            message::Complete transport;
            transport.payload.assign( message::Transport::payload_max_size * 1.5, 'A');
            transport.type = 2;

            ASSERT_TRUE( static_cast< bool>( send( transport, send::Queue::cNoBlocking)));


            auto response = receive( 0);

            EXPECT_TRUE( transport.payload == response.at( 0).payload);
         }
      }
	}
}




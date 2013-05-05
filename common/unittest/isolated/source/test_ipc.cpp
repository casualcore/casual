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

            send::Queue send( receive.getKey());

            message::Transport transport;

            std::string information( "ABC");

            transport.m_payload.m_payload = message::Transport::payload_type{ { 'A', 'B', 'C' } };

            EXPECT_TRUE( information == transport.m_payload.m_payload.data());

            transport.paylodSize( information.size() + 1);
            transport.m_payload.m_type = 2;

            send( transport);

            message::Transport response;

            receive( response);

            std::string receivedInformation{ response.m_payload.m_payload.data()};


            EXPECT_TRUE( information == receivedInformation) << "information: " << information << " - receivedInformation: " << receivedInformation;
            EXPECT_TRUE( transport.size() == response.size());
         }


         TEST( casual_common, ipc_queue_receive_timeout)
         {
            receive::Queue receive;

            message::Transport response;

            utility::signal::alarm::Scoped timeout( 1);

            //
            // We don't expect to get any messages, and for the timeout to kick in
            // after 1s
            //
            EXPECT_THROW({
               receive( response);
            }, utility::exception::signal::Timeout);

         }

         TEST( casual_common, ipc_queue_send_receive_max_message)
         {

            receive::Queue receive;

            send::Queue send( receive.getKey());

            message::Transport transport;

            std::string information;
            information.reserve( message::Transport::payload_max_size - 1);

            for( int count = 0; count < message::Transport::payload_max_size - 1; ++count)
            {
               information.push_back( '0');
            }


            std::copy( information.begin(), information.end(), transport.m_payload.m_payload.data());
            transport.m_payload.m_payload[ information.size()] = '\0';
            transport.paylodSize( information.size() + 1);
            transport.m_payload.m_type = 2;

            send( transport);

            message::Transport response;

            receive( response);

            std::string receivedInformation{ response.m_payload.m_payload.data()};


            EXPECT_TRUE( information == receivedInformation);// << "information: " << information;
            EXPECT_TRUE( transport.size() == response.size());
         }
      }
	}
}




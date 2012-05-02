//!
//! casual_isolatedunittest_ipc.cpp
//!
//! Created on: May 1, 2012
//!     Author: Lazan
//!


#include <gtest/gtest.h>

#include "casual_ipc.h"


//temp



namespace casual
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
			std::copy( information.begin(), information.end(), transport.m_payload.m_payload);
			transport.m_payload.m_payload[ information.size()] = '\0';
			transport.m_size = information.size() + 1;
			transport.m_payload.m_type = 2;

			send( transport);

			message::Transport response;

			receive( response);

			std::string receivedInformation( response.m_payload.m_payload);


			EXPECT_TRUE( information == receivedInformation);
			EXPECT_TRUE( transport.size() == response.size());
		}


		TEST( casual_common, ipc_queue_receive_timeout)
		{
			receive::Queue receive;

			message::Transport response;

			//
			// We don't expect to get any messages, and for the timeout to kick in
			// after 1s
			//
			EXPECT_FALSE( receive( response, 1));

		}

		TEST( casual_common, ipc_queue_send_receive_max_message)
		{

			receive::Queue receive;

			send::Queue send( receive.getKey());

			message::Transport transport;

			std::string information;
			information.reserve( platform::message_size - 1);
			for( int count = 0; count < platform::message_size - 1; ++count)
			{
				information.push_back( '0');
			}


			std::copy( information.begin(), information.end(), transport.m_payload.m_payload);
			transport.m_payload.m_payload[ information.size()] = '\0';
			transport.m_size = information.size() + 1;
			transport.m_payload.m_type = 2;

			send( transport);

			message::Transport response;

			receive( response);

			std::string receivedInformation( response.m_payload.m_payload);


			EXPECT_TRUE( information == receivedInformation);// << "information: " << information;
			EXPECT_TRUE( transport.size() == response.size());
		}

	}
}




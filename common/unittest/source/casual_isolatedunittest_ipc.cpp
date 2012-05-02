//!
//! casual_isolatedunittest_ipc.cpp
//!
//! Created on: May 1, 2012
//!     Author: Lazan
//!


#include <gtest/gtest.h>

#include "casual_ipc.h"


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

		TEST( casual_common, ipc_queue_send)
		{
			receive::Queue receive;

			send::Queue send( receive.getKey());
/*
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
			EXPECT_TRUE( response.size() == transport.size());

*/

		}

	}
}




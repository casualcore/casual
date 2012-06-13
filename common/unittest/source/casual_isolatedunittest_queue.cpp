//!
//! casual_isolatedunittest_queue.cpp
//!
//! Created on: Jun 9, 2012
//!     Author: Lazan
//!

#include <gtest/gtest.h>

#include "casual_queue.h"
#include "casual_ipc.h"
#include "casual_message.h"
#include "casual_exception.h"

//temp



namespace casual
{

	namespace queue
	{

		TEST( casual_common, queue_writer_reader)
		{
			ipc::receive::Queue receive;

			ipc::send::Queue send( receive.getKey());

			{
				Writer writer( send);
				message::ServerConnect message;

				message.queue_key = 666;
				message.serverPath = "banan";

				message::Service service;
				service.name = "korv";
				message.services.push_back( service);

				writer( message);
			}

			{
				Reader reader( receive);

				message::ServerConnect message;

				EXPECT_TRUE( reader.next() == message::ServerConnect::message_type);
				reader( message);

				EXPECT_TRUE( message.queue_key == 666);
				EXPECT_TRUE( message.serverPath == "banan");

				ASSERT_TRUE( message.services.size() == 1);
				EXPECT_TRUE( message.services.front().name == "korv");
			}

		}

		TEST( casual_common, queue_reader_timeout)
		{
			ipc::receive::Queue receive;
			Reader reader( receive);

			message::ServerConnect message;

			EXPECT_THROW({
				reader( message, 1);
			}, exception::signal::Timeout);


		}
	}
}




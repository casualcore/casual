//!
//! casual_isolatedunittest_queue.cpp
//!
//! Created on: Jun 9, 2012
//!     Author: Lazan
//!

#include <gtest/gtest.h>

#include "common/queue.h"
#include "common/ipc.h"
#include "common/message.h"

#include "utility/exception.h"

//temp



namespace casual
{

	namespace queue
	{

		TEST( casual_common, queue_writer_blocking_reader)
		{
			ipc::receive::Queue receive;

			ipc::send::Queue send( receive.getKey());

			{
				blocking::Writer writer( send);
				message::ServiceAdvertise message;

				message.serverId.queue_key = 666;
				message.serverPath = "banan";

				message::Service service;
				service.name = "korv";
				message.services.push_back( service);

				writer( message);
			}

			{
				blocking::Reader reader( receive);

				message::ServiceAdvertise message;

				EXPECT_TRUE( reader.next() == message::ServiceAdvertise::message_type);
				reader( message);

				EXPECT_TRUE( message.serverId.queue_key == 666);
				EXPECT_TRUE( message.serverPath == "banan");

				ASSERT_TRUE( message.services.size() == 1);
				EXPECT_TRUE( message.services.front().name == "korv");
			}

		}

		TEST( casual_common, queue_reader_timeout)
		{
			ipc::receive::Queue receive;
			blocking::Reader reader( receive);

			utility::signal::alarm::Scoped timeout( 1);

			message::ServiceAdvertise message;

			EXPECT_THROW({
				reader( message);
			}, exception::signal::Timeout);


		}

		TEST( casual_common, queue_non_blocking_reader_no_messages)
		{
			ipc::receive::Queue receive;
			non_blocking::Reader reader( receive);

			message::ServiceAdvertise message;

			EXPECT_FALSE( reader( message));

		}

		TEST( casual_common, queue_non_blocking_reader_message)
		{
			ipc::receive::Queue receive;
			non_blocking::Reader reader( receive);

			ipc::send::Queue send( receive.getKey());
			blocking::Writer writer( send);

			message::ServiceAdvertise sendMessage;
			sendMessage.serverPath = "banan";
			writer( sendMessage);

			message::ServiceAdvertise receiveMessage;
			EXPECT_TRUE( reader( receiveMessage));
			EXPECT_TRUE( receiveMessage.serverPath == "banan");

		}

		TEST( casual_common, queue_non_blocking_reader_big_message)
		{
			ipc::receive::Queue receive;
			non_blocking::Reader reader( receive);

			ipc::send::Queue send( receive.getKey());
			blocking::Writer writer( send);

			message::ServiceAdvertise sendMessage;
			sendMessage.serverPath = "banan";
			sendMessage.services.resize( 150);
			writer( sendMessage);

			message::ServiceAdvertise receiveMessage;
			EXPECT_TRUE( reader( receiveMessage));
			EXPECT_TRUE( receiveMessage.serverPath == "banan");
			EXPECT_TRUE( receiveMessage.services.size() == 150);

		}
	}
}




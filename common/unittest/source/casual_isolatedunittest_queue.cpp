//!
//! casual_isolatedunittest_queue.cpp
//!
//! Created on: Jun 9, 2012
//!     Author: Lazan
//!

#include <gtest/gtest.h>

#include "casual_queue.h"
#include "casual_ipc.h"


//temp



namespace casual
{

	namespace queue
	{

		TEST( casual_common, queue_writer)
		{
			ipc::receive::Queue receive;

			ipc::send::Queue send( receive.getKey());

			Writer writer( send);

		}
	}
}




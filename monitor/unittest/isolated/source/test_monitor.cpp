//!
//! casual_isolatedunittet_uuid.cpp
//!
//! Created on: May 1, 2012
//!     Author: Lazan
//!


#include <gtest/gtest.h>

#include "monitor/monitordb.h"
#include "common/message/monitor.h"
#include "common/platform.h"
#include "common/process.h"

#include <iostream>
#include <chrono>

//#include <unistd.h>

namespace casual
{
namespace statistics
{
	namespace monitor
	{

		TEST( casual_monitor, create_database_ok)
		{
         EXPECT_NO_THROW(
         {
				MonitorDB db(":memory:");
			});
		}

		TEST( casual_monitor, create_database_fail)
		{
		   EXPECT_THROW(
		   {
		      MonitorDB db("/path_not_exist/fail.db");
		   },std::exception);
		}

		TEST( casual_monitor, insert_ok)
		{
			EXPECT_NO_THROW(
			{
				MonitorDB db("test.db");
				common::message::monitor::Notify message;

				message.service = "myService";
				message.parentService = "myParentService";

				message.end = common::platform::clock_type::now();
				message.start = message.end - std::chrono::microseconds( 5);

				db.insert(message);
			});
		}


	}
}
}


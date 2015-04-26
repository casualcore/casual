//!
//! casual_isolatedunittet_uuid.cpp
//!
//! Created on: May 1, 2012
//!     Author: Lazan
//!


#include <gtest/gtest.h>

#include "common/message/monitor.h"
#include "common/platform.h"
#include "common/process.h"

#include <iostream>
#include <chrono>

#include "traffic_monitor/database.h"

//#include <unistd.h>

namespace casual
{
	namespace traffic_monitor
	{

		TEST( casual_monitor, create_database_ok)
		{
         EXPECT_NO_THROW(
         {
				Database db(":memory:");
			});
		}

		TEST( casual_monitor, create_database_fail)
		{
		   EXPECT_THROW(
		   {
		      Database db("/path_not_exist/fail.db");
		   },std::exception);
		}

		TEST( casual_monitor, insert_ok)
		{
			EXPECT_NO_THROW(
			{
				Database db("test.db");
				common::message::traffic_monitor::Notify message;

				message.service = "myService";
				message.parentService = "myParentService";

				message.end = common::platform::clock_type::now();
				message.start = message.end - std::chrono::microseconds( 5);

				db.insert(message);
			});
		}


	}
}


//!
//! casual_isolatedunittet_uuid.cpp
//!
//! Created on: May 1, 2012
//!     Author: Lazan
//!


#include <gtest/gtest.h>

#include "monitor/monitordb.h"
#include "common/message.h"

#include <iostream>

#include <chrono>

#include <unistd.h>

namespace casual
{
namespace statistics
{
	namespace monitor
	{

		TEST( casual_monitor, create_database_ok)
		{
			try
			{
				MonitorDB db(":memory:");
			}
			catch (...)
			{
				FAIL();
			}
		}

		TEST( casual_monitor, create_database_fail)
		{
			try
			{
				MonitorDB db("/path_not_exist/fail.db");
				FAIL();
			}
			catch (...)
			{
				SUCCEED();
			}
		}

		TEST( casual_monitor, insert_ok)
		{
			try
			{
				MonitorDB db("test.db");
				common::message::monitor::Notify message;

				message.service = "myService";
				message.parentService = "myParentService";
				message.start = std::chrono::system_clock::now();
				sleep(1);
				message.end = std::chrono::system_clock::now();
				db.insert(message);
			}
			catch (...)
			{
				FAIL();
			}
		}


	}
}
}


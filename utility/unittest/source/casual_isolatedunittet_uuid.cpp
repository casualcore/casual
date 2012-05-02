//!
//! casual_isolatedunittet_uuid.cpp
//!
//! Created on: May 1, 2012
//!     Author: Lazan
//!




#include <gtest/gtest.h>

#include "casual_utility_uuid.h"

#include <iostream>

namespace casual
{

	namespace utility
	{

		TEST( casual_utility, uuid_unique)
		{
			Uuid oneUuid;
			Uuid anotherUuid;

			EXPECT_FALSE( oneUuid == anotherUuid);

		}

		TEST( casual_utility, uuid_equal)
		{
			Uuid oneUuid;

			EXPECT_TRUE( oneUuid == oneUuid);

		}

	}

}


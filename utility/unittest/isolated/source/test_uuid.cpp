//!
//! casual_isolatedunittet_uuid.cpp
//!
//! Created on: May 1, 2012
//!     Author: Lazan
//!




#include <gtest/gtest.h>

#include "utility/uuid.h"

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

		TEST( casual_utility, uuid_getString)
		{
			Uuid oneUuid;

			EXPECT_TRUE( oneUuid.getString().size() == 36);
		}

	}

}


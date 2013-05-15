//!
//! casual_isolatedunittet_uuid.cpp
//!
//! Created on: May 1, 2012
//!     Author: Lazan
//!




#include <gtest/gtest.h>

#include "common/uuid.h"

#include <iostream>

namespace casual
{

	namespace common
	{

		TEST( casual_common_uuid, two_uuid__expect_unique)
		{
			Uuid oneUuid = Uuid::make();
			Uuid anotherUuid = Uuid::make();

			EXPECT_FALSE( oneUuid == anotherUuid);

		}

		TEST( casual_common_uuid, one_uuid__expect_equal)
      {
         Uuid oneUuid = Uuid::make();

         EXPECT_TRUE( oneUuid == oneUuid);
      }

		TEST( casual_common_uuid, two_uuid__expect_equal)
      {
         Uuid oneUuid = Uuid::make();
         Uuid anotherUuid = oneUuid;

         EXPECT_TRUE( oneUuid == anotherUuid);
      }

		TEST( casual_common_uuid, default_constructed__expect_equal_to_empty)
		{
			Uuid oneUuid;

			EXPECT_TRUE( oneUuid == Uuid::empty());
		}

		TEST( casual_common_uuid, getString__expect_36bytes)
		{
			Uuid oneUuid = Uuid::make();

			EXPECT_TRUE( oneUuid.string().size() == 36);
		}

		TEST( casual_common_uuid, default_constructed_getString__expect_00000000_0000_0000_0000_000000000000)
      {
         Uuid oneUuid;

         EXPECT_TRUE( oneUuid.string() == "00000000-0000-0000-0000-000000000000") << oneUuid.string();
      }

	}

}


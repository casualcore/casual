//! 
//! Copyright (c) 2015, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!





#include <common/unittest.h>

#include "common/uuid.h"

#include <iostream>

namespace casual
{

	namespace common
	{

		TEST( casual_common_uuid, two_uuid__expect_unique)
		{
		   common::unittest::Trace trace;

			Uuid oneUuid = uuid::make();
			Uuid anotherUuid = uuid::make();

			EXPECT_FALSE( oneUuid == anotherUuid);

		}

		TEST( casual_common_uuid, one_uuid__expect_equal)
      {
		   common::unittest::Trace trace;

         Uuid oneUuid = uuid::make();

         EXPECT_TRUE( oneUuid == oneUuid);
      }

		TEST( casual_common_uuid, two_uuid__expect_equal)
      {
		   common::unittest::Trace trace;

         Uuid oneUuid = uuid::make();
         Uuid anotherUuid = oneUuid;

         EXPECT_TRUE( oneUuid == anotherUuid);
      }

		TEST( casual_common_uuid, default_constructed__expect_equal_to_empty)
		{
		   common::unittest::Trace trace;

			Uuid oneUuid;

			EXPECT_TRUE( oneUuid == uuid::empty());
		}

		TEST( casual_common_uuid, getString__expect_32B)
		{
		   common::unittest::Trace trace;

			Uuid oneUuid = uuid::make();

			EXPECT_TRUE( uuid::string( oneUuid).size() == 32);
		}

		TEST( casual_common_uuid, default_constructed__uuid_string__expect_00000000000000000000000000000000)
      {
		   common::unittest::Trace trace;

         Uuid oneUuid;

         EXPECT_TRUE( uuid::string( oneUuid) == "00000000000000000000000000000000") << uuid::string( oneUuid);
      }

      TEST( casual_common_uuid, ctor_55d9d19db80b49ac98188deef6fa0d4a__copy_ctor__expect_equal_string)
      {
         common::unittest::Trace trace;

         Uuid uuid1{ "55d9d19db80b49ac98188deef6fa0d4a"};

         EXPECT_TRUE( uuid::string( uuid1) == "55d9d19db80b49ac98188deef6fa0d4a") << uuid::string( uuid1);

         Uuid uuid2{ uuid1};

         EXPECT_TRUE( uuid1 == uuid2) << uuid::string( uuid1);
      }




      TEST( casual_common_uuid, operator_bool_on_default_constructed__expect_false)
      {
         common::unittest::Trace trace;

         Uuid oneUuid;

         EXPECT_FALSE( oneUuid) << uuid::string( oneUuid);
      }


      TEST( casual_common_uuid, operator_bool_on_created__expect_true)
      {
         common::unittest::Trace trace;

         Uuid oneUuid = uuid::make();

         EXPECT_TRUE( static_cast< bool>( oneUuid) ) << uuid::string( oneUuid);
      }
	}

}


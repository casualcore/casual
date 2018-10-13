//! 
//! Copyright (c) 2015, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!


#include "common/unittest.h"

#include "common/service/header.h"


namespace casual
{
   namespace common
   {
      namespace service
      {

         TEST( common_service_header, clear)
         {
            common::unittest::Trace trace;

            header::fields().clear();

            EXPECT_TRUE( header::fields().empty());
         }

         TEST( common_service_header, empty__replace_add__expect_exists)
         {
            common::unittest::Trace trace;

            header::Fields fields;

            fields[ "casual.key.test"] = "42";

            EXPECT_TRUE( fields.exists( "casual.key.test"));
         }

         TEST( common_service_header, empty__replace_add__get_value)
         {
            common::unittest::Trace trace;

            header::Fields fields;

            fields[ "casual.key.test"] = "42";

            EXPECT_TRUE( fields.at( "casual.key.test") == "42");
         }

         TEST( common_service_header, empty__replace_add__get_int_value)
         {
            common::unittest::Trace trace;

            header::Fields fields;
            
            fields[ "casual.key.test"] = "42";

            EXPECT_TRUE( fields.at< int>( "casual.key.test") == 42);
         }

         TEST( common_service_header, empty__replace_add__get_value_with_default__expect_actual_value)
         {
            common::unittest::Trace trace;

            header::Fields fields;
            
            fields[ "casual.key.test"] = "42";

            EXPECT_TRUE( fields.at( "casual.key.test", "poop") == "42");
         }

         TEST( common_service_header, empty__replace_add__get_value_with_default__expect_default_value)
         {
            common::unittest::Trace trace;

            header::Fields fields;
            
            fields[ "casual.key.test"] = "42";

            EXPECT_TRUE( fields.at( "non-existent-key", "poop") == "poop");
         }

         TEST( common_service_header, empty_find__expect_absent)
         {
            common::unittest::Trace trace;

            header::Fields fields;

            EXPECT_TRUE( ! fields.find( "casual.key.test").has_value());
         }

         TEST( common_service_header, one_field___find__expect_found)
         {
            common::unittest::Trace trace;

            header::Fields fields;
            fields[ "casual.key.test"] = "42";

            ASSERT_TRUE( fields.find( "casual.key.test").has_value());
            EXPECT_TRUE( fields.find( "casual.key.test").value() == "42");
         }

         TEST( common_service_header, add_2_empty___expect_empty)
         {
            common::unittest::Trace trace;

            auto fields = header::Fields{} + header::Fields{};

            EXPECT_TRUE( fields.empty());
         }

         TEST( common_service_header, add_2_fields___expect_appended)
         {
            common::unittest::Trace trace;

            auto fields = header::Fields{ { "key1", "42"}} + header::Fields{ { "key2", "43"}};

            ASSERT_TRUE( fields.size() == 2);
            EXPECT_TRUE( fields.front().key == "key1");
            EXPECT_TRUE( fields.front().value == "42");
         }

         TEST( common_service_header, add_assing_fields___expect_appended)
         {
            common::unittest::Trace trace;

            auto fields = header::Fields{ { "key1", "42"}};
            fields += header::Fields{ { "key2", "43"}};

            ASSERT_TRUE( fields.size() == 2);
            EXPECT_TRUE( fields.front().key == "key1");
            EXPECT_TRUE( fields.front().value == "42");
         }

      } // service
   } // common
} // casual

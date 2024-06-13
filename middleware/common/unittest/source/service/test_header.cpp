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

         TEST( common_service_header, field_ctor)
         {
            {
               auto field = header::Field{ "a:b"};
               EXPECT_TRUE( field.name() == "a");
               EXPECT_TRUE( field.value() == "b");
            }
            {
               auto field = header::Field{ " a : b "};
               EXPECT_TRUE( field.name() == "a") << CASUAL_NAMED_VALUE( field);
               EXPECT_TRUE( field.value() == "b");
            }

         }

         TEST( common_service_header, clear)
         {
            common::unittest::Trace trace;

            header::fields().clear();

            EXPECT_TRUE( header::fields().empty());
         }

         TEST( common_service_header, empty__replace_add__expect_contains)
         {
            common::unittest::Trace trace;

            header::Fields fields;

            fields.add( header::Field{ "casual.key.test: 42"});

            EXPECT_TRUE( fields.contains( "casual.key.test"));
         }

         TEST( common_service_header, empty__replace_add__get_value)
         {
            common::unittest::Trace trace;

            header::Fields fields;

            fields.add( header::Field{ "casual.key.test: 42"});

            EXPECT_TRUE( fields.at( "casual.key.test").value() == "42") << CASUAL_NAMED_VALUE( fields);
         }



         TEST( common_service_header, empty_find__expect_absent)
         {
            common::unittest::Trace trace;

            header::Fields fields;

            EXPECT_TRUE( fields.find( "casual.key.test") == nullptr);
         }

         TEST( common_service_header, one_field___find__expect_found)
         {
            common::unittest::Trace trace;

            header::Fields fields;
            fields.add( header::Field{ "casual.key.test: 42"});

            ASSERT_TRUE( fields.find( "casual.key.test") != nullptr);
            EXPECT_TRUE( fields.find( "casual.key.test")->value() == "42");
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

            auto fields = header::Fields{ { header::Field{ "key1:42"}}} + header::Fields{ { header::Field{ "key2:43"}}};

            ASSERT_TRUE( fields.size() == 2);
            EXPECT_TRUE( fields.contains( "key1"));
            EXPECT_TRUE( fields.contains( "key2"));
         }

         TEST( common_service_header, add_assing_fields___expect_appended)
         {
            common::unittest::Trace trace;

            auto fields = header::Fields{ { header::Field{ "key1:42"}}};
            fields += header::Fields{ { header::Field{ "key2:43"}}};

            ASSERT_TRUE( fields.size() == 2);
            EXPECT_TRUE( fields.contains( "key1"));
            EXPECT_TRUE( fields.contains( "key2"));
         }

      } // service
   } // common
} // casual

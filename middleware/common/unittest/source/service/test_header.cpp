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

      } // service
   } // common
} // casual

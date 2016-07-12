//!
//! casual 
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
            CASUAL_UNITTEST_TRACE();

            header::clear();

            EXPECT_TRUE( header::fields().empty());
         }

         TEST( common_service_header, empty__replace_add__expect_exists)
         {
            CASUAL_UNITTEST_TRACE();

            header::clear();

            header::replace::add( { "casual.key.test", "42"});

            EXPECT_TRUE( header::exists( "casual.key.test"));
         }

         TEST( common_service_header, empty__replace_add__get_value)
         {
            CASUAL_UNITTEST_TRACE();

            header::clear();

            header::replace::add( { "casual.key.test", "42"});

            EXPECT_TRUE( header::get( "casual.key.test") == "42");
         }

         TEST( common_service_header, empty__replace_add__get_int_value)
         {
            CASUAL_UNITTEST_TRACE();

            header::clear();

            header::replace::add( { "casual.key.test", "42"});

            EXPECT_TRUE( header::get< int>( "casual.key.test") == 42);
         }

         TEST( common_service_header, empty__replace_add__get_value_with_default__expect_actual_value)
         {
            CASUAL_UNITTEST_TRACE();

            header::clear();

            header::replace::add( { "casual.key.test", "42"});

            EXPECT_TRUE( header::get( "casual.key.test", "poop") == "42");
         }

         TEST( common_service_header, empty__replace_add__get_value_with_default__expect_default_value)
         {
            CASUAL_UNITTEST_TRACE();

            header::clear();

            header::replace::add( { "casual.key.test", "42"});

            EXPECT_TRUE( header::get( "non-existen-key", "poop") == "poop");
         }

      } // service
   } // common
} // casual

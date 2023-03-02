//!
//! Copyright (c) 2019, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!

#include "common/unittest.h"

#include "common/serialize/macro.h"

namespace casual
{
   namespace common
   {
      namespace serialize
      {
         TEST( serialize_named_value, const_lvalue)
         {
            const int value = 42;
            auto named = CASUAL_NAMED_VALUE( value);

            EXPECT_TRUE(( std::is_same< decltype( named), named::Value< const int, named::reference::lvalue>>::value )) << CASUAL_NAMED_VALUE( named);
         }

         TEST( serialize_named_value, lvalue)
         {
            int value = 42;
            auto named = CASUAL_NAMED_VALUE( value);

            EXPECT_TRUE(( std::is_same< decltype( named), named::Value< int, named::reference::lvalue>>::value )) << CASUAL_NAMED_VALUE( named);
         }

         TEST( serialize_named_value, rvalue)
         {
            int value = 42;
            auto named = CASUAL_NAMED_VALUE( std::move( value));

            EXPECT_TRUE(( std::is_same< decltype( named), named::Value< int, named::reference::rvalue>>::value )) << CASUAL_NAMED_VALUE( named);
         }

         TEST( serialize_named_value, instantiation)
         {
            long someLong = 10;

            auto nvp = CASUAL_NAMED_VALUE( someLong);

            EXPECT_TRUE( nvp.name() == std::string( "someLong"));
            EXPECT_TRUE( nvp.value() == 10);
         }

         TEST( serialize_named_value, instantiation_const)
         {
            const long someLong = 10;

            EXPECT_TRUE( CASUAL_NAMED_VALUE( someLong).name() == std::string( "someLong"));
            EXPECT_TRUE( CASUAL_NAMED_VALUE( someLong).value() == 10);

         }

         TEST( serialize_named_value, instantiation_rvalue)
         {
            EXPECT_TRUE( CASUAL_NAMED_VALUE( 10L).name() == std::string( "10L"));
            EXPECT_TRUE( CASUAL_NAMED_VALUE( 10L).value() == 10);
         }

      } // serialize
   } // common
} // casual
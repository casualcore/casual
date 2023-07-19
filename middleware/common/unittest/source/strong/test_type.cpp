//! 
//! Copyright (c) 2015, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!


#include "common/unittest.h"

#include "common/strong/type.h"


namespace casual
{
   namespace common
   {
      namespace local
      {
         namespace
         {
            struct policy
            {
               constexpr static int initialize() { return -1;}
               constexpr static bool valid( int value) { return value != initialize();}
            };

            using Type = strong::Type< int, policy>;

            static_assert( strong::detail::has::valid< Type>);
            static_assert( strong::detail::has::initialize< policy>);

         } // <unnamed>
      } // local
      TEST( common_strong_type, default_ctor)
      {
         common::unittest::Trace trace;

         local::Type value;

         EXPECT_TRUE( ! value);
      }

      TEST( common_strong_type, ctor)
      {
         common::unittest::Trace trace;

         local::Type value{ 42};

         EXPECT_TRUE( value);
         EXPECT_TRUE( value.value() == 42);
      }

      TEST( common_strong_type, valid)
      {
         common::unittest::Trace trace;

         EXPECT_TRUE( ! local::Type{}.valid());
         EXPECT_TRUE( local::Type{ 42}.valid());
      }

      TEST( common_strong_type, invalid_stream_nil)
      {
         common::unittest::Trace trace;
         std::ostringstream out;
         auto value = local::Type{};
         out << value;

         EXPECT_TRUE( out.str() == "nil") << out.str();
      }

      TEST( common_strong_type, in_hash_map)
      {
         common::unittest::Trace trace;

         using opt = local::Type;

         opt value;

         std::unordered_map< opt, int> map{
            { value, 1 },
            { opt( 42), 42 },
         };


         EXPECT_TRUE( map.at( value) == 1);
         EXPECT_TRUE( map.at( opt( 42)) == 42);
      }
   } // common
} // casual

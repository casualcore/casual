//!
//! casual 
//!

#include <gtest/gtest.h>

#include "common/flag.h"

namespace casual
{
   namespace common
   {
      enum class Enum
      {
         flag1 = 1,
         flag2 = 2,
         flag3 = 4,
         flag4 = 8,
         flag5 = 16
      };

      namespace local
      {
         namespace
         {
            template< typename T>
            bool boolean( T&& value)
            {
               return static_cast< bool>( value);
            }

         } // <unnamed>
      } // local



      TEST( casual_common_flags, empty__expect_false)
      {
         Flags< Enum> flags;
         EXPECT_FALSE( local::boolean( flags));
      }

      TEST( casual_common_flags, initialize_flag1__expect_true)
      {
         Flags< Enum> flags{ Enum::flag1};
         EXPECT_TRUE( local::boolean( flags));
      }

      TEST( casual_common_flags, initialize_flag1_flag2__expect_true)
      {
         Flags< Enum> flags{ Enum::flag1, Enum::flag2};
         EXPECT_TRUE( local::boolean( flags));
      }

      TEST( casual_common_flags, initialize_flag1__binary_and__flag1___expect_true)
      {
         Flags< Enum> flags{ Enum::flag1};
         EXPECT_TRUE( local::boolean( flags & Enum::flag1));
      }

      TEST( casual_common_flags, initialize_flag1__binary_and__flag2___expect_false)
      {
         Flags< Enum> flags{ Enum::flag1};
         EXPECT_FALSE( local::boolean( flags & Enum::flag2));
      }

      TEST( casual_common_flags, initialize_flag1_flag2__binary_and__flag2___expect_true)
      {
         Flags< Enum> flags{ Enum::flag1, Enum::flag2};
         EXPECT_TRUE( local::boolean( flags & Enum::flag2));
      }

      TEST( casual_common_flags, initialize_flag1__binary_or_flag2__binary_and__flag2___expect_true)
      {
         Flags< Enum> flags{ Enum::flag1};
         EXPECT_TRUE( local::boolean( ( flags | Enum::flag2) & Enum::flag2));
      }

   } // common
} // casual

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


      //
      // Test the compile time initialization
      //
      static_assert( Flags< Enum>{ Enum::flag1, Enum::flag2}.underlaying() == ( 1 | 2), "bla");
      static_assert( Flags< Enum>{ Enum::flag1} == Flags< Enum>{ Enum::flag1}, "bla");
      static_assert( Flags< Enum>{ Enum::flag1} != Flags< Enum>{ Enum::flag2}, "bla");
      static_assert( Flags< Enum>{ Enum::flag1}, "bla");

      static_assert( Flags< Enum>{ Enum::flag1} == Enum::flag1, "bla");

      static_assert( Flags< Enum>{ Enum::flag1} & Enum::flag1, "bla");
      static_assert( ! ( Flags< Enum>{ Enum::flag1} & Enum::flag2), "bla");
      static_assert( ( Flags< Enum>{ Enum::flag1} | Enum::flag2 | Enum::flag3 ) == Flags< Enum>{ Enum::flag1, Enum::flag2, Enum::flag3} , "bla");


      static_assert( ! Flags< Enum>{}, "bla");


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

      TEST( casual_common_flags, one_flag__equality)
      {
         EXPECT_TRUE( local::boolean( Flags< Enum>{ Enum::flag1} == Flags< Enum>{ Enum::flag1}));
      }

      TEST( casual_common_flags, three_flag__equality)
      {
         EXPECT_TRUE( local::boolean(
               Flags< Enum>{ Enum::flag1, Enum::flag2, Enum::flag3}
            == Flags< Enum>{ Enum::flag1, Enum::flag2, Enum::flag3}));
      }

      TEST( casual_common_flags, initialize_flag1_flag2__binary_and__initialize_flag1_flag2___expect_true)
      {
         Flags< Enum> flags{ Enum::flag1, Enum::flag2};
         EXPECT_TRUE( local::boolean( flags & flags));
      }

      TEST( casual_common_flags, initialize_flag1_flag2_flag3__binary_and__initialize_flag1_flag2___expect_true)
      {
         EXPECT_TRUE( local::boolean(
               Flags< Enum>{ Enum::flag1, Enum::flag2, Enum::flag3}
             & Flags< Enum>{ Enum::flag1, Enum::flag2}));
      }

      TEST( casual_common_flags, initialize_flag1_flag2__binary_and__initialize_flag1_flag2_flag3___expect_true)
      {
         EXPECT_TRUE( local::boolean(
               Flags< Enum>{ Enum::flag1, Enum::flag2}
               & Flags< Enum>{ Enum::flag1, Enum::flag2, Enum::flag3}));
      }

      TEST( casual_common_flags, init_flag1_flag2__binary_or__init_flag2_flag3__equal_to__init_flag1_flag2_flag3)
      {
         EXPECT_TRUE( local::boolean(
               ( Flags< Enum>{ Enum::flag1, Enum::flag2} | Flags< Enum>{ Enum::flag2, Enum::flag3})
                  ==  Flags< Enum>{ Enum::flag1, Enum::flag2, Enum::flag3}));
      }


      TEST( casual_common_flags, exist)
      {
         Flags< Enum> flags{ Enum::flag1, Enum::flag2};
         EXPECT_TRUE( flags.exist( Enum::flag2));
      }

   } // common
} // casual

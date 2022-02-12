//! 
//! Copyright (c) 2015, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!


#include <gtest/gtest.h>

#include "common/flag.h"
#include "common/predicate.h"
#include "common/string.h"

namespace casual
{
   namespace common
   {
      enum struct Enum : std::uint16_t
      {
         flag1 = 1,
         flag2 = 2,
         flag3 = 4,
         flag4 = 8,
         combo3_4 = flag3 | flag4,
         flag5 = 16,
         flag_max = 1 << ( std::numeric_limits< std::uint16_t>::digits - 1)
      };
      constexpr auto description( Enum flag)
      {
         switch( flag)
         {
            case Enum::flag1: return std::string_view{ "flag1"};
            case Enum::flag2: return std::string_view{ "flag2"};
            case Enum::flag3: return std::string_view{ "flag3"};
            case Enum::flag4: return std::string_view{ "flag4"};
            case Enum::flag5: return std::string_view{ "flag5"};
            case Enum::flag_max: return std::string_view{ "flag_max"};
            default: return std::string_view{ "<unknown>"};
         }
      }

      // Test the compile time initialization
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
         EXPECT_FALSE( predicate::boolean( flags));
      }

      TEST( casual_common_flags, initialize_flag1__expect_true)
      {
         Flags< Enum> flags{ Enum::flag1};
         EXPECT_TRUE( predicate::boolean( flags));
      }

      TEST( casual_common_flags, initialize_flag1_flag2__expect_true)
      {
         Flags< Enum> flags{ Enum::flag1, Enum::flag2};
         EXPECT_TRUE( predicate::boolean( flags));
      }

      TEST( casual_common_flags, initialize_flag1__binary_and__flag1___expect_true)
      {
         Flags< Enum> flags{ Enum::flag1};
         EXPECT_TRUE( predicate::boolean( flags & Enum::flag1));
      }

      TEST( casual_common_flags, initialize_flag1__binary_and__flag2___expect_false)
      {
         Flags< Enum> flags{ Enum::flag1};
         EXPECT_FALSE( predicate::boolean( flags & Enum::flag2));
      }

      TEST( casual_common_flags, initialize_flag1_flag2__binary_and__flag2___expect_true)
      {
         Flags< Enum> flags{ Enum::flag1, Enum::flag2};
         EXPECT_TRUE( predicate::boolean( flags & Enum::flag2));
      }

      TEST( casual_common_flags, initialize_flag1__binary_or_flag2__binary_and__flag2___expect_true)
      {
         Flags< Enum> flags{ Enum::flag1};
         EXPECT_TRUE( predicate::boolean( ( flags | Enum::flag2) & Enum::flag2));
      }

      TEST( casual_common_flags, one_flag__equality)
      {
         EXPECT_TRUE( predicate::boolean( Flags< Enum>{ Enum::flag1} == Flags< Enum>{ Enum::flag1}));
      }

      TEST( casual_common_flags, three_flag__equality)
      {
         EXPECT_TRUE( predicate::boolean(
               Flags< Enum>{ Enum::flag1, Enum::flag2, Enum::flag3}
            == Flags< Enum>{ Enum::flag1, Enum::flag2, Enum::flag3}));
      }

      TEST( casual_common_flags, initialize_flag1_flag2__binary_and__initialize_flag1_flag2___expect_true)
      {
         Flags< Enum> flags{ Enum::flag1, Enum::flag2};
         EXPECT_TRUE( predicate::boolean( flags & flags));
      }

      TEST( casual_common_flags, initialize_flag1_flag2_flag3__binary_and__initialize_flag1_flag2___expect_true)
      {
         EXPECT_TRUE( predicate::boolean(
               Flags< Enum>{ Enum::flag1, Enum::flag2, Enum::flag3}
             & Flags< Enum>{ Enum::flag1, Enum::flag2}));
      }

      TEST( casual_common_flags, initialize_flag1_flag2__binary_and__initialize_flag1_flag2_flag3___expect_true)
      {
         EXPECT_TRUE( predicate::boolean(
               Flags< Enum>{ Enum::flag1, Enum::flag2}
               & Flags< Enum>{ Enum::flag1, Enum::flag2, Enum::flag3}));
      }

      TEST( casual_common_flags, init_flag1_flag2__binary_or__init_flag2_flag3__equal_to__init_flag1_flag2_flag3)
      {
         EXPECT_TRUE( predicate::boolean(
               ( Flags< Enum>{ Enum::flag1, Enum::flag2} | Flags< Enum>{ Enum::flag2, Enum::flag3})
                  ==  Flags< Enum>{ Enum::flag1, Enum::flag2, Enum::flag3}));
      }

      TEST( casual_common_flags, exist)
      {
         Flags< Enum> flags{ Enum::flag1, Enum::flag2};
         EXPECT_TRUE( flags.exist( Enum::flag2));
      }

      TEST( casual_common_flags, combo)
      {
         Flags< Enum> flags{ Enum::flag3, Enum::flag4};
         EXPECT_TRUE( flags != Enum::flag1);
         EXPECT_TRUE( flags != Enum::flag2);
         EXPECT_TRUE( flags != Enum::flag3);
         EXPECT_FALSE( flags == Enum::flag3);
         EXPECT_TRUE( flags != Enum::flag4);
         EXPECT_FALSE( flags == Enum::flag4);
         EXPECT_TRUE( flags == Enum::combo3_4);
      }

      TEST( casual_common_flags, stream)
      {
         {
            Flags< Enum> flags{ Enum::flag1, Enum::flag2, Enum::flag5};
            EXPECT_TRUE( string::compose( flags) == "[ flag1, flag2, flag5]") << "flags: " << string::compose( flags);
         }

         {
            Flags< Enum> flags{ Enum::flag_max};
            EXPECT_TRUE( string::compose( flags) == "[ flag_max]") << "flags: " << string::compose( flags);
         }

         {
            Flags< Enum> flags{ Enum::combo3_4};
            EXPECT_TRUE( string::compose( flags) == "[ flag3, flag4]") << "flags: " << string::compose( flags);
         }
      }

   } // common
} // casual

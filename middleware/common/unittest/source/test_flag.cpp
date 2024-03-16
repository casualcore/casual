//! 
//! Copyright (c) 2015, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!


#include "common/unittest.h"

#include "common/flag.h"
#include "common/predicate.h"
#include "common/string/compose.h"

namespace casual
{
   namespace common
   {
      namespace detail
      {
         enum struct Flag : std::uint16_t
         {
            a = 0b00000001,
            b = 0b00000010,
            c = 0b00000100,
            d = 0b00001000,
            e = 0b00010000,
            a_c_d = ( a | c | d),
         };

         consteval void casual_enum_as_flag( Flag);

         constexpr std::string_view description( Flag value)
         {
            switch( value)
            {
               case Flag::a: return "a";
               case Flag::b: return "b";
               case Flag::c: return "c";
               case Flag::d: return "d";
               case Flag::e: return "e";
               case Flag::a_c_d: return "a_c_d";
            }
            return "<unknown>";
         }
      } // detail

      TEST( common_flag, exist)
      {
         unittest::Trace trace;
         using Flag = detail::Flag;
   
         auto flags = Flag::a | Flag::b;

         EXPECT_TRUE( flag::exists( flags, Flag::a));
         EXPECT_TRUE( flag::exists( flags, Flag::b));
         EXPECT_TRUE( ! flag::exists( flags, Flag::c));
         EXPECT_TRUE( ! flag::exists( flags, Flag{}));
      }

      TEST( common_flags, empty)
      {
         unittest::Trace trace;
         using Flag = detail::Flag;

         EXPECT_TRUE( flag::empty( Flag{}));
         EXPECT_TRUE( ! flag::empty( Flag::b));
      }

      TEST( common_flags, count)
      {
         unittest::Trace trace;
         using Flag = detail::Flag;

         EXPECT_TRUE( flag::count( Flag{}) == 0);
         EXPECT_TRUE( flag::count( Flag::a) == 1);
         EXPECT_TRUE( flag::count( Flag::a | Flag::b | Flag::c) == 3);
      }

      TEST( common_flags, print)
      {
         unittest::Trace trace;
         using Flag = detail::Flag;

         EXPECT_TRUE( string::compose( Flag{}) == "[]");
         EXPECT_TRUE( string::compose( Flag::a) == "[ a]");
         EXPECT_TRUE( string::compose( Flag::a | Flag::b | Flag::d ) == "[ a, b, d]");
      }

      TEST( common_flag, operator_or)
      {
         unittest::Trace trace;
         using Flag = detail::Flag;

         auto flags = Flag::a | Flag::b;

         EXPECT_TRUE( flag::exists( flags, Flag::a));
         EXPECT_TRUE( flag::exists( flags, Flag::b));
         EXPECT_TRUE( ! flag::exists( flags, Flag::c));
      }

      TEST( common_flag, operator_reference_or)
      {
         unittest::Trace trace;
         using Flag = detail::Flag;

         auto flags = Flag::a;

         EXPECT_TRUE( ( flags |= Flag::b) == ( Flag::a | Flag::b));
         EXPECT_TRUE( ( flags |= Flag::b) == ( Flag::a | Flag::b));
         EXPECT_TRUE( ( flags |= Flag::c) == ( Flag::a | Flag::b | Flag::c));
         EXPECT_TRUE( ( flags |= Flag::d) == ( Flag::a | Flag::b | Flag::c | Flag::d));
      }

      TEST( common_flag, operator_reference_subtract)
      {
         unittest::Trace trace;
         using Flag = detail::Flag;

         auto flags = Flag::a | Flag::b | Flag::c;
         
         // subtract non existent -> expect same
         EXPECT_TRUE( ( flags -= Flag::d) == ( Flag::a | Flag::b | Flag::c));
         EXPECT_TRUE( ( flags -= Flag::b) == ( Flag::a | Flag::c));
         EXPECT_TRUE( ( flags -= Flag::a) == Flag::c);
         EXPECT_TRUE( flag::empty( flags -= Flag::c));
      }

      TEST( common_flag, operator_and)
      {
         unittest::Trace trace;
         using Flag = detail::Flag;

         const auto flags = Flag::a | Flag::b | Flag::c | Flag::e;

         {
            auto result = flags & Flag::a_c_d;
            EXPECT_TRUE( flag::exists( result, Flag::a));
            EXPECT_TRUE( ! flag::exists( result, Flag::b));
            EXPECT_TRUE( flag::exists( result, Flag::c));
            EXPECT_TRUE( ! flag::exists( result, Flag::d));
            EXPECT_TRUE( ! flag::exists( result, Flag::e));
         }
      }

      TEST( common_flags, valid)
      {
         unittest::Trace trace;
         using Flag = detail::Flag;

         constexpr auto expected = Flag::a | Flag::c;

         EXPECT_TRUE( flag::valid( expected, Flag::a));
         EXPECT_TRUE( flag::valid( expected, Flag::a | Flag::c));
         EXPECT_TRUE( ! flag::valid( expected, Flag::b));
         EXPECT_TRUE( ! flag::valid( expected, Flag::a | Flag::b | Flag::c));
      }

      TEST( common_flags, convert)
      {
         unittest::Trace trace;
         using Flag = detail::Flag;

         constexpr auto filter = Flag::a | Flag::c;

         EXPECT_TRUE( flag::convert( filter, 0b11111) == filter);
         EXPECT_TRUE( flag::convert( filter, 0b01) == Flag::a);
         EXPECT_TRUE( flag::convert( filter, 0) == Flag{});
      }

      namespace detail::subset
      {
         enum struct Flag : std::uint16_t
         {
            b = std::to_underlying( detail::Flag::b),
            d = std::to_underlying( detail::Flag::d),
            e = std::to_underlying( detail::Flag::e),
         };

         consteval void casual_enum_as_flag( Flag);

         // use detail::Flag as a superset -> use it's description
         consteval detail::Flag casual_enum_as_flag_superset( Flag);
         
      } // detail::subset

      TEST( common_flags, superset_print)
      {
         unittest::Trace trace;
         using Flag = detail::subset::Flag;

         EXPECT_TRUE( string::compose( Flag{}) == "[]");
         EXPECT_TRUE( string::compose( Flag::b) == "[ b]");
         EXPECT_TRUE( string::compose( Flag::b | Flag::e) == "[ b, e]");
      }


   } // common
} // casual

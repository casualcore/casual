//!
//! test_conformance.cpp
//!
//! Created on: Dec 21, 2014
//!     Author: Lazan
//!


#include <gtest/gtest.h>


#include "common/algorithm.h"
#include "common/traits.h"

#include <type_traits>


//
// Just a place to test C++-conformance, or rather, casual's view on conformance
//



namespace casual
{
   namespace common
   {
      TEST( casual_common_conformance, struct_with_pod_attributes__is_pod)
      {
         struct POD
         {
            int int_value;
         };

         EXPECT_TRUE( std::is_pod< POD>::value);
      }

      TEST( casual_common_conformance, struct_with_member_function__is_pod)
      {
         struct POD
         {
            int func1() { return 42;}
         };

         EXPECT_TRUE( std::is_pod< POD>::value);
      }


      TEST( casual_common_conformance, is_floating_point__is_signed)
      {

         EXPECT_TRUE( std::is_signed< float>::value);
         EXPECT_TRUE( std::is_floating_point< float>::value);

         EXPECT_TRUE( std::is_signed< double>::value);
         EXPECT_TRUE( std::is_floating_point< double>::value);

      }





      struct some_functor
      {
         void operator () ( const double& value)  {}

         int test;
      };


      TEST( casual_common_conformance, get_functor_argument_type)
      {

         EXPECT_TRUE( traits::function< some_functor>::arguments() == 1);

         using argument_type = typename traits::function< some_functor>::argument< 0>::type;

         auto is_same = std::is_same< const double&, argument_type>::value;
         EXPECT_TRUE( is_same);

      }


      TEST( casual_common_conformance, get_function_argument_type)
      {
         using function_1 = std::function< void( double&)>;

         EXPECT_TRUE( traits::function< function_1>::arguments() == 1);

         using argument_type = typename traits::function< function_1>::argument< 0>::type;

         auto is_same = std::is_same< double&, argument_type>::value;
         EXPECT_TRUE( is_same);

      }



      long some_function( const std::string& value) { return 1;}

      TEST( casual_common_conformance, get_free_function_argument_type)
      {
         using traits_type = traits::function< decltype( some_function)>;

         EXPECT_TRUE( traits_type::arguments() == 1);

         {
            using argument_type = typename traits_type::argument< 0>::type;

            auto is_same = std::is_same< const std::string&, argument_type>::value;
            EXPECT_TRUE( is_same);
         }

         {
            using result_type = typename traits_type::result_type;

            auto is_same = std::is_same< long, result_type>::value;
            EXPECT_TRUE( is_same);
         }
      }




      TEST( casual_common_conformance, search)
      {
         std::string source{ "some string to search in"};
         std::string to_find{ "some"};


         EXPECT_TRUE( std::search( std::begin( source), std::end( source), std::begin( to_find), std::end( to_find)) != std::end( source));
      }

      /*
       * To bad std::function does not support move-only functors...
       *
      struct move_only_functor
      {
         move_only_functor( move_only_functor&&) = default;
         move_only_functor& operator = ( move_only_functor&&) = default;

         bool operator () ( long value)
         {
            return value == 42;
         }
      };


      TEST( casual_common_conformance, std_function__move_only_functor)
      {
         std::function< bool(long)> f1{ move_only_functor{}};

         EXPECT_TRUE( f1( 42));
      }
      */


   } // common
} // casual

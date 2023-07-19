//! 
//! Copyright (c) 2015, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!


#include <common/unittest.h>

#include "common/traits.h"
#include "casual/concepts.h"

#include <array>
#include <stack>

namespace casual
{

   namespace common
   {
      static_assert( concepts::container::array< std::array< int, 10>>);

      TEST( casual_common_traits, is_container_array_like)
      {
         common::unittest::Trace trace;

         EXPECT_TRUE( ( concepts::container::array< std::array< int, 10>>));
      }

      TEST( casual_common_traits, is_container_sequence_like)
      {
         common::unittest::Trace trace;

         EXPECT_TRUE(  concepts::container::sequence< std::vector< int>>);
         EXPECT_TRUE(  concepts::container::sequence< std::deque< int>>);
         EXPECT_TRUE(  concepts::container::sequence< std::list< int>>);
         EXPECT_FALSE(  concepts::container::sequence< std::set< int>>);

         //EXPECT_TRUE( ( traits::is::container::sequence::like< std::array< int, 10>>::value));
      }

      TEST( casual_common_traits, is_container_associative_like)
      {
         common::unittest::Trace trace;

         EXPECT_TRUE( ( concepts::container::associative< std::map< int, int>>));
         EXPECT_TRUE( ( concepts::container::associative< std::multimap< int, int>>));
         EXPECT_TRUE( ( concepts::container::associative< std::unordered_map< int, int>>));
         EXPECT_TRUE( ( concepts::container::associative< std::unordered_multimap< int, int>>));

         EXPECT_TRUE(  concepts::container::associative< std::set< int>>);
         EXPECT_TRUE(  concepts::container::associative< std::multiset< int>>);
         EXPECT_TRUE(  concepts::container::associative< std::unordered_set< int>>);
         EXPECT_TRUE(  concepts::container::associative< std::unordered_multiset< int>>);

         EXPECT_FALSE(  concepts::container::associative< std::vector< int>>);
      }


      TEST( casual_common_traits, is_container_like)
      {
         common::unittest::Trace trace;

         EXPECT_TRUE( ( concepts::container::like< std::map< int, int>>));
         EXPECT_TRUE( ( concepts::container::like< std::multimap< int, int>>));
         EXPECT_TRUE( ( concepts::container::like< std::unordered_map< int, int>>));
         EXPECT_TRUE( ( concepts::container::like< std::unordered_multimap< int, int>>));

         EXPECT_TRUE(  concepts::container::like< std::set< int>>);
         EXPECT_TRUE(  concepts::container::like< std::multiset< int>>);
         EXPECT_TRUE(  concepts::container::like< std::unordered_set< int>>);
         EXPECT_TRUE(  concepts::container::like< std::unordered_multiset< int>>);

         EXPECT_TRUE(  concepts::container::like< std::vector< int>>);
         EXPECT_TRUE(  concepts::container::like< std::deque< int>>);
         EXPECT_TRUE(  concepts::container::like< std::list< int>>);

         EXPECT_FALSE(  concepts::container::like< long>);
      }

      TEST( casual_common_traits_function, functor_void)
      {
         common::unittest::Trace trace;

         struct Functor
         {
            void operator () () {};
         };

         EXPECT_TRUE( traits::function< Functor>::arguments() == 0);
         EXPECT_TRUE( ( std::is_same< typename traits::function< Functor>::result_type, void>::value));
      }

      TEST( casual_common_traits_function, functor_long__short_double)
      {
         common::unittest::Trace trace;

         struct Functor
         {
            long operator () ( short, double) { return 42;};
         };

         EXPECT_TRUE( traits::function< Functor>::arguments() == 2);
         EXPECT_TRUE( ( std::is_same< typename traits::function< Functor>::result_type, long>::value));
         EXPECT_TRUE( ( std::is_same< typename traits::function< Functor>::argument< 0>::type, short>::value));
         EXPECT_TRUE( ( std::is_same< typename traits::function< Functor>::argument< 1>::type, double>::value));

      }

      TEST( casual_common_traits_function, lambda_bool__int)
      {
         common::unittest::Trace trace;

         auto lamdba = [](int){ return true;};
         EXPECT_TRUE( traits::function< decltype( lamdba)>::arguments() == 1);
         EXPECT_TRUE( ( std::is_same< typename traits::function< decltype( lamdba)>::result_type, bool>::value));
         EXPECT_TRUE( ( std::is_same< typename traits::function< decltype( lamdba)>::argument< 0>::type, int>::value));
      }

      TEST( casual_common_traits_function, member_function_bool__long)
      {
         common::unittest::Trace trace;

         struct Functor
         {
            bool function( long) { return true;};
         };

         EXPECT_TRUE( traits::function< decltype( &Functor::function)>::arguments() == 1);
         EXPECT_TRUE( ( std::is_same< typename traits::function< decltype( &Functor::function)>::argument< 0>::type, long>::value));
         EXPECT_TRUE( ( std::is_same< typename traits::function< decltype( &Functor::function)>::result_type, bool>::value));
      }

      namespace local
      {
         namespace
         {
            bool free_function_void_long( long value)
            {
               return true;
            }
         } // <unnamed>
      } // local

      TEST( casual_common_traits_function, free_function_void__long)
      {
         common::unittest::Trace trace;


         EXPECT_TRUE( traits::function< decltype( &local::free_function_void_long)>::arguments() == 1);
         EXPECT_TRUE( ( std::is_same< typename traits::function< decltype( &local::free_function_void_long)>::argument< 0>::type, long>::value));
         EXPECT_TRUE( ( std::is_same< typename traits::function< decltype( &local::free_function_void_long)>::result_type, bool>::value));
         EXPECT_TRUE( local::free_function_void_long( 42));
      }


   }

} // casual

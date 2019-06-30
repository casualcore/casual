//! 
//! Copyright (c) 2015, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!


#include <common/unittest.h>

#include "common/traits.h"

#include <array>
#include <stack>

namespace casual
{

   namespace common
   {
      TEST( casual_common_traits, is_container_array_like)
      {
         common::unittest::Trace trace;

         EXPECT_TRUE( ( traits::is::container::array::like< std::array< int, 10>>::value));
      }

      TEST( casual_common_traits, is_container_sequence_like)
      {
         common::unittest::Trace trace;

         EXPECT_TRUE(  traits::is::container::sequence::like< std::vector< int>>::value);
         EXPECT_TRUE(  traits::is::container::sequence::like< std::deque< int>>::value);
         EXPECT_TRUE(  traits::is::container::sequence::like< std::list< int>>::value);
         EXPECT_FALSE(  traits::is::container::sequence::like< std::set< int>>::value);

         //EXPECT_TRUE( ( traits::is::container::sequence::like< std::array< int, 10>>::value));
      }

      TEST( casual_common_traits, is_container_associative_like)
      {
         common::unittest::Trace trace;

         EXPECT_TRUE( ( traits::is::container::associative::like< std::map< int, int>>::value));
         EXPECT_TRUE( ( traits::is::container::associative::like< std::multimap< int, int>>::value));
         EXPECT_TRUE( ( traits::is::container::associative::like< std::unordered_map< int, int>>::value));
         EXPECT_TRUE( ( traits::is::container::associative::like< std::unordered_multimap< int, int>>::value));

         EXPECT_TRUE(  traits::is::container::associative::like< std::set< int>>::value);
         EXPECT_TRUE(  traits::is::container::associative::like< std::multiset< int>>::value);
         EXPECT_TRUE(  traits::is::container::associative::like< std::unordered_set< int>>::value);
         EXPECT_TRUE(  traits::is::container::associative::like< std::unordered_multiset< int>>::value);

         EXPECT_FALSE(  traits::is::container::associative::like< std::vector< int>>::value);
      }


      TEST( casual_common_traits, is_container_like)
      {
         common::unittest::Trace trace;

         EXPECT_TRUE( ( traits::is::container::like< std::map< int, int>>::value));
         EXPECT_TRUE( ( traits::is::container::like< std::multimap< int, int>>::value));
         EXPECT_TRUE( ( traits::is::container::like< std::unordered_map< int, int>>::value));
         EXPECT_TRUE( ( traits::is::container::like< std::unordered_multimap< int, int>>::value));

         EXPECT_TRUE(  traits::is::container::like< std::set< int>>::value);
         EXPECT_TRUE(  traits::is::container::like< std::multiset< int>>::value);
         EXPECT_TRUE(  traits::is::container::like< std::unordered_set< int>>::value);
         EXPECT_TRUE(  traits::is::container::like< std::unordered_multiset< int>>::value);

         EXPECT_TRUE(  traits::is::container::like< std::vector< int>>::value);
         EXPECT_TRUE(  traits::is::container::like< std::deque< int>>::value);
         EXPECT_TRUE(  traits::is::container::like< std::list< int>>::value);

         EXPECT_FALSE(  traits::is::container::like< long>::value);
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

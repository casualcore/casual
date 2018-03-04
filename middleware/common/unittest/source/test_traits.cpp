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
      TEST( casual_common_traits, is_array_container)
      {
         common::unittest::Trace trace;


         EXPECT_TRUE( ( traits::container::is_array< std::array< int, 10>>::value));
      }

      TEST( casual_common_traits, is_sequence_container)
      {
         common::unittest::Trace trace;

         EXPECT_TRUE(  traits::container::is_sequence< std::vector< int>>::value);
         EXPECT_TRUE(  traits::container::is_sequence< std::deque< int>>::value);
         EXPECT_TRUE(  traits::container::is_sequence< std::list< int>>::value);
         EXPECT_TRUE( ( traits::container::is_sequence< std::array< int, 10>>::value));

         EXPECT_TRUE( ( traits::container::is_sequence< std::array< int, 10>>::value));


         EXPECT_FALSE(  traits::container::is_sequence< std::set< int>>::value);
      }


      TEST( casual_common_traits, is_associative_container)
      {
         common::unittest::Trace trace;

         EXPECT_TRUE( ( traits::container::is_associative< std::map< int, int>>::value));
         EXPECT_TRUE( ( traits::container::is_associative< std::multimap< int, int>>::value));
         EXPECT_TRUE( ( traits::container::is_associative< std::unordered_map< int, int>>::value));
         EXPECT_TRUE( ( traits::container::is_associative< std::unordered_multimap< int, int>>::value));

         EXPECT_TRUE(  traits::container::is_associative< std::set< int>>::value);
         EXPECT_TRUE(  traits::container::is_associative< std::multiset< int>>::value);
         EXPECT_TRUE(  traits::container::is_associative< std::unordered_set< int>>::value);
         EXPECT_TRUE(  traits::container::is_associative< std::unordered_multiset< int>>::value);

         EXPECT_FALSE(  traits::container::is_associative< std::vector< int>>::value);
      }

      TEST( casual_common_traits, container_is_unordered)
      {
         common::unittest::Trace trace;

         EXPECT_TRUE( ( traits::container::is_unordered< std::unordered_map< int, int>>::value));
         EXPECT_TRUE( ( traits::container::is_unordered< std::unordered_multimap< int, int>>::value));
         EXPECT_TRUE(  traits::container::is_unordered< std::unordered_set< int>>::value);
         EXPECT_TRUE(  traits::container::is_unordered< std::unordered_multiset< int>>::value);

         EXPECT_FALSE( ( traits::container::is_unordered< std::map< int, int>>::value));
      }

      TEST( casual_common_traits, is_container_adaptor)
      {
         common::unittest::Trace trace;

         EXPECT_TRUE(  traits::container::is_adaptor< std::stack< int>>::value);
         EXPECT_TRUE(  traits::container::is_adaptor< std::queue< int>>::value);
         EXPECT_TRUE(  traits::container::is_adaptor< std::priority_queue< int>>::value);

         EXPECT_FALSE(  traits::container::is_adaptor< std::vector< int>>::value);
      }

      TEST( casual_common_traits, is_container)
      {
         common::unittest::Trace trace;

         EXPECT_TRUE( ( traits::container::is_container< std::map< int, int>>::value));
         EXPECT_TRUE( ( traits::container::is_container< std::multimap< int, int>>::value));
         EXPECT_TRUE( ( traits::container::is_container< std::unordered_map< int, int>>::value));
         EXPECT_TRUE( ( traits::container::is_container< std::unordered_multimap< int, int>>::value));

         EXPECT_TRUE(  traits::container::is_container< std::set< int>>::value);
         EXPECT_TRUE(  traits::container::is_container< std::multiset< int>>::value);
         EXPECT_TRUE(  traits::container::is_container< std::unordered_set< int>>::value);
         EXPECT_TRUE(  traits::container::is_container< std::unordered_multiset< int>>::value);

         EXPECT_TRUE(  traits::container::is_container< std::vector< int>>::value);
         EXPECT_TRUE(  traits::container::is_container< std::deque< int>>::value);
         EXPECT_TRUE(  traits::container::is_container< std::list< int>>::value);


         EXPECT_TRUE(  traits::container::is_container< std::stack< int>>::value);
         EXPECT_TRUE(  traits::container::is_container< std::queue< int>>::value);
         EXPECT_TRUE(  traits::container::is_container< std::priority_queue< int>>::value);

         EXPECT_FALSE(  traits::container::is_container< long>::value);
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


      TEST( casual_common_traits_function, has_serialize__false)
      {
         common::unittest::Trace trace;

         struct A
         {
         };

         EXPECT_FALSE( ( traits::has::serialize< A, long>::value));
         //EXPECT_FALSE( test_1< A>::value);
      }

      namespace local
      {
         namespace
         {
            struct A
            {
               template< typename A>
               void serialize( A& archive) {}
            };
         } // <unnamed>
      } // local

      TEST( casual_common_traits_function, has_serialize__true)
      {
         common::unittest::Trace trace;

         EXPECT_TRUE( ( traits::has::serialize< local::A, long>::value));
      }




   }

} // casual

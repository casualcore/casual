//!
//! test_traits.cpp
//!
//! Created on: Jun 30, 2014
//!     Author: Lazan
//!

#include "common/traits.h"


#include <gtest/gtest.h>


#include <array>
#include <stack>

namespace casual
{

   namespace common
   {
      TEST( casual_common_traits, is_sequence_container)
      {

         EXPECT_TRUE(  traits::container::is_sequence< std::vector< int>>::value);
         EXPECT_TRUE(  traits::container::is_sequence< std::deque< int>>::value);
         EXPECT_TRUE(  traits::container::is_sequence< std::list< int>>::value);
         EXPECT_TRUE( ( traits::container::is_sequence< std::array< int, 10>>::value));

         EXPECT_FALSE(  traits::container::is_sequence< std::set< int>>::value);
      }


      TEST( casual_common_traits, is_associative_container)
      {

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

         EXPECT_TRUE( ( traits::container::is_unordered< std::unordered_map< int, int>>::value));
         EXPECT_TRUE( ( traits::container::is_unordered< std::unordered_multimap< int, int>>::value));
         EXPECT_TRUE(  traits::container::is_unordered< std::unordered_set< int>>::value);
         EXPECT_TRUE(  traits::container::is_unordered< std::unordered_multiset< int>>::value);

         EXPECT_FALSE( ( traits::container::is_unordered< std::map< int, int>>::value));
      }

      TEST( casual_common_traits, is_container_adaptor)
      {

         EXPECT_TRUE(  traits::container::is_adaptor< std::stack< int>>::value);
         EXPECT_TRUE(  traits::container::is_adaptor< std::queue< int>>::value);
         EXPECT_TRUE(  traits::container::is_adaptor< std::priority_queue< int>>::value);

         EXPECT_FALSE(  traits::container::is_adaptor< std::vector< int>>::value);
      }

      TEST( casual_common_traits, is_container)
      {

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
         struct Functor
         {
            void operator () () {};
         };

         EXPECT_TRUE( traits::function< Functor>::arguments() == 0);
         EXPECT_TRUE( ( std::is_same< typename traits::function< Functor>::result_type, void>::value));
      }

      TEST( casual_common_traits_function, functor_long__short_double)
      {
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
         auto lamdba = [](int){ return true;};
         EXPECT_TRUE( traits::function< decltype( lamdba)>::arguments() == 1);
         EXPECT_TRUE( ( std::is_same< typename traits::function< decltype( lamdba)>::result_type, bool>::value));
         EXPECT_TRUE( ( std::is_same< typename traits::function< decltype( lamdba)>::argument< 0>::type, int>::value));
      }

      TEST( casual_common_traits_function, member_function_bool__long)
      {
         struct Functor
         {
            bool function( long) { return true;};
         };

         EXPECT_TRUE( traits::function< decltype( &Functor::function)>::arguments() == 1);
         EXPECT_TRUE( ( std::is_same< typename traits::function< decltype( &Functor::function)>::argument< 0>::type, long>::value));
         EXPECT_TRUE( ( std::is_same< typename traits::function< decltype( &Functor::function)>::result_type, bool>::value));
      }


      template< typename T, typename... Args>
      struct has_serializible
      {
      private:
         using one = char;
         struct two { char m[ 2];};

         template< typename C, typename... A> static auto test( C&& c, A&&... a) -> decltype( (void)( c.serialize( a...)), one());
         static two test(...);
      public:
         enum { value = sizeof( test( std::declval< T>(), std::declval< Args>()...)) == sizeof(one) };
      };


      template< typename T>
      struct test_1 : std::integral_constant< bool, std::is_same< decltype( std::declval< T>().serialize( long())), void>::value > {};




      TEST( casual_common_traits_function, member_has_serializible__false)
      {
         struct A
         {
         };

         EXPECT_FALSE( ( has_serializible< A, long>::value));
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

               void serialize( long) {}
            };
         } // <unnamed>
      } // local

      TEST( casual_common_traits_function, member_has_serializible__true)
      {


         EXPECT_TRUE( ( has_serializible< local::A, long>::value));
      }







   }

} // casual

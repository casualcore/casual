//!
//! test_traits.cpp
//!
//! Created on: Jun 30, 2014
//!     Author: Lazan
//!

#include "common/traits.h"


#include <gtest/gtest.h>



namespace casual
{

   namespace common
   {
      TEST( casual_common_traits, is_sequence_container)
      {

         EXPECT_TRUE(  traits::is_sequence_container< std::vector< int>>::value);
         EXPECT_TRUE(  traits::is_sequence_container< std::deque< int>>::value);
         EXPECT_TRUE(  traits::is_sequence_container< std::list< int>>::value);

         EXPECT_FALSE(  traits::is_sequence_container< std::set< int>>::value);
      }


      TEST( casual_common_traits, is_associative_set_container)
      {

         EXPECT_TRUE(  traits::is_associative_set_container< std::set< int>>::value);
         EXPECT_TRUE(  traits::is_associative_set_container< std::multiset< int>>::value);
         EXPECT_TRUE(  traits::is_associative_set_container< std::unordered_set< int>>::value);
         EXPECT_TRUE(  traits::is_associative_set_container< std::unordered_multiset< int>>::value);

         EXPECT_FALSE(  traits::is_associative_set_container< std::vector< int>>::value);
      }

      TEST( casual_common_traits, is_associative_map_container)
      {

         EXPECT_TRUE( ( traits::is_associative_map_container< std::map< int, int>>::value));
         EXPECT_TRUE( ( traits::is_associative_map_container< std::multimap< int, int>>::value));
         EXPECT_TRUE( ( traits::is_associative_map_container< std::unordered_map< int, int>>::value));
         EXPECT_TRUE( ( traits::is_associative_map_container< std::unordered_multimap< int, int>>::value));

         EXPECT_FALSE(  traits::is_associative_map_container< std::set< int>>::value);
      }

      TEST( casual_common_traits, is_associative_map_container_lvalue)
      {

         EXPECT_TRUE( ( traits::is_associative_map_container< std::map< int, int>>::value));
         EXPECT_TRUE( ( traits::is_associative_map_container< std::multimap< int, int>>::value));
         EXPECT_TRUE( ( traits::is_associative_map_container< std::unordered_map< int, int>>::value));
         EXPECT_TRUE( ( traits::is_associative_map_container< std::unordered_multimap< int, int>>::value));

         EXPECT_FALSE(  traits::is_associative_map_container< std::set< int>>::value);
      }

      TEST( casual_common_traits, is_associative_container)
      {

         EXPECT_TRUE( ( traits::is_associative_container< std::map< int, int>>::value));
         EXPECT_TRUE( ( traits::is_associative_container< std::multimap< int, int>>::value));
         EXPECT_TRUE( ( traits::is_associative_container< std::unordered_map< int, int>>::value));
         EXPECT_TRUE( ( traits::is_associative_container< std::unordered_multimap< int, int>>::value));

         EXPECT_TRUE(  traits::is_associative_container< std::set< int>>::value);
         EXPECT_TRUE(  traits::is_associative_container< std::multiset< int>>::value);
         EXPECT_TRUE(  traits::is_associative_container< std::unordered_set< int>>::value);
         EXPECT_TRUE(  traits::is_associative_container< std::unordered_multiset< int>>::value);

         EXPECT_FALSE(  traits::is_associative_container< std::vector< int>>::value);
      }

      TEST( casual_common_traits, is_container_adaptor)
      {

         EXPECT_TRUE(  traits::is_container_adaptor< std::stack< int>>::value);
         EXPECT_TRUE(  traits::is_container_adaptor< std::queue< int>>::value);
         EXPECT_TRUE(  traits::is_container_adaptor< std::priority_queue< int>>::value);

         EXPECT_FALSE(  traits::is_container_adaptor< std::vector< int>>::value);
      }

      TEST( casual_common_traits, is_container)
      {

         EXPECT_TRUE( ( traits::is_container< std::map< int, int>>::value));
         EXPECT_TRUE( ( traits::is_container< std::multimap< int, int>>::value));
         EXPECT_TRUE( ( traits::is_container< std::unordered_map< int, int>>::value));
         EXPECT_TRUE( ( traits::is_container< std::unordered_multimap< int, int>>::value));

         EXPECT_TRUE(  traits::is_container< std::set< int>>::value);
         EXPECT_TRUE(  traits::is_container< std::multiset< int>>::value);
         EXPECT_TRUE(  traits::is_container< std::unordered_set< int>>::value);
         EXPECT_TRUE(  traits::is_container< std::unordered_multiset< int>>::value);

         EXPECT_TRUE(  traits::is_container< std::vector< int>>::value);
         EXPECT_TRUE(  traits::is_container< std::deque< int>>::value);
         EXPECT_TRUE(  traits::is_container< std::list< int>>::value);


         EXPECT_FALSE(  traits::is_container< long>::value);
      }

   }

} // casual

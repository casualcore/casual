//!
//! test_algorithm.cpp
//!
//! Created on: Dec 22, 2013
//!     Author: Lazan
//!

#include <gtest/gtest.h>


#include "common/algorithm.h"

namespace casual
{
   namespace common
   {

      namespace local
      {
         std::vector< int> unsorted() { return std::vector< int>{ 5,6,3,4,1,8,9,4,3,2};}

         std::vector< int> sorted() {   return std::vector< int>{ 1,2,3,3,4,4,5,6,8,9};}

      } // local



      TEST( casual_common_algorithm, sort)
      {
         auto us = local::unsorted();
         auto s = local::sorted();

         auto sorted = sort( make_range( us));

         EXPECT_TRUE( ! sorted.empty());
         EXPECT_TRUE( equal( sorted, make_range( s)));
      }


      TEST( casual_common_algorithm, sort_predi)
      {
         auto us = local::unsorted();
         auto s = local::sorted();

         auto sorted = sort( make_range( us), std::less< int>());

         EXPECT_TRUE( ! sorted.empty());
         EXPECT_TRUE( equal( sorted, make_range( s)));

      }

      TEST( casual_common_algorithm, sort_reverse)
      {
         auto us = local::unsorted();
         auto s = local::sorted();

         auto sorted = sort( make_reverse_range( us));

         EXPECT_TRUE( ! sorted.empty());
         EXPECT_TRUE( equal( sorted, make_range( s)));
      }


      TEST( casual_common_algorithm, sort_predicate_reverse)
      {
         auto us = local::unsorted();
         auto s = local::sorted();

         auto sorted = sort( make_reverse_range( us), std::less< int>());

         EXPECT_TRUE( ! sorted.empty());
         EXPECT_TRUE( equal( sorted, make_range( s))) << "sorted : " << sorted << "\nunsortd: " << make_range( s);

      }

      TEST( casual_common_algorithm, partition)
      {
         auto us = local::unsorted();

         auto part = partition( make_range( us), []( int value) { return value == 3;});

         ASSERT_TRUE( part.size() == 2);
         EXPECT_TRUE( *part.first == 3);
      }

      TEST( casual_common_algorithm, partition_reverse)
      {
         auto us = local::unsorted();

         auto part = partition( make_reverse_range( us), []( int value) { return value == 3;});

         ASSERT_TRUE( part.size() == 2);
         EXPECT_TRUE( *part.first == 3);
      }

      TEST( casual_common_algorithm, find_value)
      {
         auto us = local::unsorted();

         auto found =  common::find( make_range( us), 3);

         ASSERT_TRUE( ! found.empty());
         EXPECT_TRUE( *found.first == 3);

      }

      TEST( casual_common_algorithm, find_value_reverse)
      {
         auto us = local::unsorted();

         auto found =  common::find( make_reverse_range( us), 3);

         ASSERT_TRUE( ! found.empty());
         EXPECT_TRUE( *found.first == 3);

      }

      TEST( casual_common_algorithm, sort_patition_find_value)
      {
         auto us = local::unsorted();


         auto found =  common::find(
               common::partition(
                     common::sort( make_range( us)),
               []( int value) { return value == 3;}), 3);

         ASSERT_TRUE( ! found.empty());
         EXPECT_TRUE( *found.first == 3);

      }


   } // algorithm

} // casual

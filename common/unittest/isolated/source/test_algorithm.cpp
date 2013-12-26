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

         auto sorted = range::sort( range::make( us));

         EXPECT_TRUE( ! sorted.empty());
         EXPECT_TRUE( range::equal( sorted, range::make( s)));
      }


      TEST( casual_common_algorithm, sort_predi)
      {
         auto us = local::unsorted();
         auto s = local::sorted();

         auto sorted = range::sort( range::make( us), std::less< int>());

         EXPECT_TRUE( ! sorted.empty());
         EXPECT_TRUE( range::equal( sorted, range::make( s)));

      }

      TEST( casual_common_algorithm, sort_reverse)
      {
         auto us = local::unsorted();
         auto s = local::sorted();

         auto sorted = range::sort( range::make_reverse( us));

         EXPECT_TRUE( ! sorted.empty());
         EXPECT_TRUE( range::equal( sorted, range::make( s)));
      }


      TEST( casual_common_algorithm, sort_predicate_reverse)
      {
         auto us = local::unsorted();
         auto s = local::sorted();

         auto sorted = range::sort( range::make_reverse( us), std::less< int>());

         EXPECT_TRUE( ! sorted.empty());
         EXPECT_TRUE( range::equal( sorted, range::make( s))) << "sorted : " << sorted << "\nunsortd: " << range::make( s);

      }

      TEST( casual_common_algorithm, partition)
      {
         auto us = local::unsorted();

         auto part = range::partition( range::make( us), []( int value) { return value == 3;});

         ASSERT_TRUE( part.size() == 2);
         EXPECT_TRUE( *part.first == 3);
      }

      TEST( casual_common_algorithm, partition_reverse)
      {
         auto us = local::unsorted();

         auto part = range::partition( range::make_reverse( us), []( int value) { return value == 3;});

         ASSERT_TRUE( part.size() == 2);
         EXPECT_TRUE( *part.first == 3);
      }

      TEST( casual_common_algorithm, find_value)
      {
         auto us = local::unsorted();

         auto found =  range::find( range::make( us), 3);

         ASSERT_TRUE( ! found.empty());
         EXPECT_TRUE( *found.first == 3);

      }

      TEST( casual_common_algorithm, find_value_reverse)
      {
         auto us = local::unsorted();

         auto found =  range::find( range::make_reverse( us), 3);

         ASSERT_TRUE( ! found.empty());
         EXPECT_TRUE( *found.first == 3);

      }

      TEST( casual_common_algorithm, sort_patition_find_value)
      {
         auto us = local::unsorted();


         auto found =  range::find(
               range::partition(
                     range::sort( range::make( us)),
               []( int value) { return value == 3;}), 3);

         ASSERT_TRUE( ! found.empty());
         EXPECT_TRUE( *found.first == 3);

      }


   } // algorithm

} // casual

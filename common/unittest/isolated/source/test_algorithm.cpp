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
         std::vector< int> unsorted() { return { 5,6,3,4,1,8,9,4,3,2};}

         std::vector< int> sorted() {   return { 1,2,3,3,4,4,5,6,8,9};}

      } // local



      TEST( casual_common_algorithm, sort)
      {
         auto us = local::unsorted();
         auto s = local::sorted();

         // shall not compile
         //auto sorted = range::sort( range::make( local::unsorted()));

         auto sorted = range::sort( us);

         //EXPECT_TRUE( sorted);
         EXPECT_TRUE( ! sorted.empty());
         EXPECT_TRUE( range::equal( sorted, s));
      }


      TEST( casual_common_algorithm, sort_predi)
      {
         auto us = local::unsorted();
         auto s = local::sorted();

         auto sorted = range::sort( us, std::less< int>());

         //EXPECT_TRUE( sorted);
         EXPECT_TRUE( ! sorted.empty());
         EXPECT_TRUE( range::equal( sorted, s));

      }

      TEST( casual_common_algorithm, sort_reverse)
      {
         auto us = local::unsorted();
         auto s = local::sorted();

         auto sorted = range::sort( range::make_reverse( us));

         //EXPECT_TRUE( sorted);
         EXPECT_TRUE( ! sorted.empty());
         EXPECT_TRUE( range::equal( sorted, s));
      }


      TEST( casual_common_algorithm, sort_predicate_reverse)
      {
         auto us = local::unsorted();
         auto s = local::sorted();

         auto sorted = range::sort( range::make_reverse( us), std::less< int>());

         //EXPECT_TRUE( sorted);
         EXPECT_TRUE( ! sorted.empty());
         EXPECT_TRUE( range::equal( sorted, s)) << "sorted : " << sorted << "\nunsortd: " << range::make( s);

      }

      TEST( casual_common_algorithm, partition)
      {
         auto us = local::unsorted();

         auto part = range::partition( us, []( int value) { return value == 3;});

         ASSERT_TRUE( part.size() == 2);
         EXPECT_TRUE( *part == 3);
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

         auto found =  range::find( us, 3);

         ASSERT_TRUE( ! found.empty());
         EXPECT_TRUE( *found == 3);
      }

      TEST( casual_common_algorithm, find_map_value)
      {
         std::map< int, std::string> container{ {1, "one"}, {2, "two"}, {3, "three"}, {4, "four"}};

         auto found =  range::find( container, 3);

         ASSERT_TRUE( ! found.empty());
         EXPECT_TRUE( found->second == "three");
      }

      TEST( casual_common_algorithm, find_value_reverse)
      {
         auto us = local::unsorted();

         auto found =  range::find( range::make_reverse( us), 3);

         ASSERT_TRUE( ! found.empty());
         EXPECT_TRUE( *found == 3);

      }

      TEST( casual_common_algorithm, uniform__true)
      {
         std::vector< int> uniform{ 1, 1, 1, 1, 1, 1, 1, 1, 1};

         EXPECT_TRUE( range::uniform (uniform));

      }

      TEST( casual_common_algorithm, uniform__false)
      {
         std::vector< int> uniform{ 1, 1, 1, 1, 1, 1, 1, 2, 1};

         EXPECT_FALSE( range::uniform (uniform));
      }


      TEST( casual_common_algorithm, sort_patition_find_value)
      {
         auto us = local::unsorted();


         auto found =  range::find(
               range::partition(
                     range::sort( us),
               []( int value) { return value == 3;}), 3);

         ASSERT_TRUE( ! found.empty());
         EXPECT_TRUE( *found == 3);
      }

      TEST( casual_common_algorithm, trim_container)
      {
         auto s = local::sorted();

         auto found =  range::find( s, 3);
         auto result = range::trim( s, found);

         EXPECT_TRUE( range::equal( result, s));

      }

      enum class Enum
      {
         zero,
         one,
         two,
      };

      TEST( casual_common_algorithm, enum_as_integer)
      {
         auto e = Enum::two;

         EXPECT_TRUE( common::as_integer( e) == 2);
      }

      TEST( casual_common_algorithm, enum_ostream)
      {
         auto e = Enum::two;

         std::ostringstream out;

         out << e;

         EXPECT_TRUE( out.str() == "2");
      }


      template< typename T>
      struct basic_equal
      {
         basic_equal( T value) : m_value( std::move( value)) {}

         template< typename V>
         bool operator () ( V&& value) const
         {
            return value == m_value;
         }

         T m_value;
      };

      using Equal = basic_equal< int>;

      TEST( casual_common_algorithm, chain_basic)
      {

         std::vector< int> range{ 1, 2, 4, 5, 6, 7};

         auto part = range::partition( range, chain::Or::link( Equal{ 2}, Equal{ 4}));

         EXPECT_TRUE( part.size() == 2);

      }


      TEST( casual_common_algorithm, chain_basic_lvalue)
      {
         auto functor = chain::Or::link( Equal{ 2}, Equal{ 4});

         std::vector< int> range{ 1, 2, 4, 5, 6, 7};

         auto part = range::partition( range, functor);

         EXPECT_TRUE( part.size() == 2);
      }

      TEST( casual_common_algorithm, chain_basic_lvalue1)
      {
         Equal pred1{ 2};
         Equal pred2{ 4};
         auto functor = chain::Or::link( pred1, pred2);

         std::vector< int> range{ 1, 2, 4, 5, 6, 7};

         auto part = range::partition( range, functor);

         EXPECT_TRUE( part.size() == 2);
      }

      struct Value
      {
         std::string name;
         long age;
         double height;
      };

      namespace order
      {
         namespace name
         {
            struct Ascending
            {
               bool operator () ( const Value& lhs, const Value& rhs) const { return lhs.name < rhs.name;}
            };

            using Descending = compare::Inverse< Ascending>;
         } // name

         namespace age
         {
            struct Ascending
            {
               bool operator () ( const Value& lhs, const Value& rhs) const { return lhs.age < rhs.age;}
            };

            using Descending = compare::Inverse< Ascending>;
         } // age

         namespace height
         {
            struct Ascending
            {
               bool operator () ( const Value& lhs, const Value& rhs) const { return lhs.height < rhs.height;}
            };

            using Descending = compare::Inverse< Ascending>;
         } // hight

      } // order

      namespace local
      {
         std::vector< Value> values()
         {
            return { { "Tom", 29, 1.75}, { "Charlie", 23, 1.63 }, { "Charlie", 29, 1.90}, { "Tom", 30, 1.63} };
         }
      } // local

      TEST( casual_common_algorithm, chain_order_name_asc)
      {
         auto values = local::values();

         auto sorted = range::sort( values, chain::Order::link( order::name::Ascending()));

         EXPECT_TRUE( sorted->name == "Charlie");
      }

      TEST( casual_common_algorithm, chain_order_name_desc__age_asc)
      {
         auto values = local::values();

         auto sorted = range::sort( values, chain::Order::link(
               order::name::Descending(),
               order::age::Ascending()));

         EXPECT_TRUE( sorted->name == "Tom");
         EXPECT_TRUE( sorted->age == 29);
         EXPECT_TRUE( ( sorted.first + 3)->name == "Charlie");
         EXPECT_TRUE( ( sorted.first + 3)->age == 29);

      }

      TEST( casual_common_algorithm, chain_order_age_desc__height_asc)
      {
         auto values = local::values();

         auto sorted = range::sort( values, chain::Order::link(
               order::age::Descending(),
               order::height::Ascending()));

         EXPECT_TRUE( ( sorted.first + 1)->age == 29);
         EXPECT_TRUE( ( sorted.first + 1)->height == 1.75);
         EXPECT_TRUE( ( sorted.first + 2)->age == 29);
         EXPECT_TRUE( ( sorted.first + 2)->height == 1.90);

      }

      TEST( casual_common_algorithm, copy_max__source_shorter_than_target)
      {
         const std::string source = "1234";
         std::string target;

         range::copy_max( source, 50, std::back_inserter( target));

         EXPECT_TRUE( target == "1234") << target;

      }

      TEST( casual_common_algorithm, copy_max__source_equal_to_target)
      {
         const std::string source = "1234567";
         std::string target;

         range::copy_max( source, source.size(), std::back_inserter( target));

         EXPECT_TRUE( source == target) << target;

      }

      TEST( casual_common_algorithm, copy_max__source_longer_than_target)
      {
         const std::string source = "1234567";
         std::string target;

         range::copy_max( source, 4, std::back_inserter( target));

         EXPECT_TRUE( target == "1234") << target;

      }



   } // algorithm

} // casual

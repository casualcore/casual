//! 
//! Copyright (c) 2015, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!


#include "common/unittest.h"
#include "common/unittest/log.h"
#include "common/algorithm.h"
#include "common/algorithm/compare.h"
#include "common/algorithm/coalesce.h"
#include "common/algorithm/is.h"

namespace casual
{
   namespace common
   {

      namespace local
      {
         namespace
         {
            std::vector< int> unsorted() { return { 5,6,3,4,1,8,9,4,3,2};}

            std::vector< int> sorted() {   return { 1,2,3,3,4,4,5,6,8,9};}
         }

      } // local


      TEST( common_algorithm_range, default_constructor__expect_empty)
      {
         common::unittest::Trace trace;

         using range_type = range::type_t< std::vector< int>>;
         range_type empty;

         EXPECT_TRUE( empty.empty());
         EXPECT_TRUE( empty.size() == 0);
      }

      TEST( common_algorithm_range, default_constructor_int_pointer__expect_empty)
      {
         common::unittest::Trace trace;

         Range< int*> empty;

         EXPECT_TRUE( empty.empty());
         EXPECT_TRUE( empty.size() == 0);
      }

      TEST( common_algorithm_equal, list_container)
      {
         common::unittest::Trace trace;

         std::list< int> container{ 1, 2, 3, 4, 5};

         auto range = range::make( container);
         EXPECT_TRUE( algorithm::equal( range, container));
      }

      TEST( common_algorithm_equal, deque_container)
      {
         common::unittest::Trace trace;

         std::deque< int> container{ 1, 2, 3, 4, 5};

         auto range = range::make( container);
         EXPECT_TRUE( algorithm::equal( range, container));
      }

      TEST( common_algorithm_equal, array_container)
      {
         common::unittest::Trace trace;

         std::array< int, 5> container = {{ 1, 2, 3, 4, 5}};

         auto range = range::make( container);
         EXPECT_TRUE( algorithm::equal( range, container));
      }

      TEST( common_algorithm_equal, c_array_container)
      {
         common::unittest::Trace trace;

         int container[] = { 1, 2, 3, 4, 5};

         auto range = range::make( container);
         EXPECT_TRUE( algorithm::equal( range, container));
      }

      TEST( common_algorithm_position, overlap)
      {
         common::unittest::Trace trace;

         std::vector< int> container( 100);

         EXPECT_TRUE( range::position::overlap( container, container));
         EXPECT_TRUE( range::position::overlap( range::make( std::begin( container), 60), container));
         EXPECT_TRUE( range::position::overlap( container, range::make( std::begin( container) + 40, 60)));
         EXPECT_TRUE( range::position::overlap( range::make( std::begin( container), 60), range::make( std::begin( container) + 40, 60)));

         EXPECT_FALSE( range::position::overlap( range::make( std::begin( container), 40), range::make( std::begin( container) + 60, 30)));
      }

      TEST( common_algorithm_position, subtract_two_equal_ranges__expect_empty_result)
      {
         common::unittest::Trace trace;

         std::vector< int> container( 100);

         auto result = range::position::subtract( container, container);

         EXPECT_TRUE( std::get< 0>( result).empty());
         EXPECT_TRUE( std::get< 1>( result).empty());
      }

      TEST( common_algorithm_position, subtract_bigger_from_smaller__expect_empty_result)
      {
         common::unittest::Trace trace;

         std::vector< int> container( 100);

         auto result = range::position::subtract( range::make( std::begin( container) + 10, 40), container);

         EXPECT_TRUE( std::get< 0>( result).empty());
         EXPECT_TRUE( std::get< 1>( result).empty());
      }

      TEST( common_algorithm_position, subtract_right_overlapping__expect_first)
      {
         common::unittest::Trace trace;

         std::vector< int> container( 100);

         auto result = range::position::subtract( container, range::make( std::begin( container) + 40, std::end( container)));

         ASSERT_TRUE( ! std::get< 0>( result).empty());
         ASSERT_TRUE( std::get< 0>( result).size() == 40);
         ASSERT_TRUE( std::get< 0>( result).end() == std::begin( container) + 40);
         EXPECT_TRUE( std::get< 1>( result).empty());
      }

      TEST( common_algorithm_position, subtract_left_overlapping__expect_first)
      {
         common::unittest::Trace trace;

         std::vector< int> container( 100);

         auto result = range::position::subtract( container, range::make( std::begin( container), 40));

         ASSERT_TRUE( ! std::get< 0>( result).empty());
         ASSERT_TRUE( std::get< 0>( result).size() == 60) << "std::get< 0>( result).size(): " << std::get< 0>( result).size();
         ASSERT_TRUE( std::get< 0>( result).begin() == std::begin( container) + 40);
         EXPECT_TRUE( std::get< 1>( result).empty());
      }

      TEST( common_algorithm_position, subtract_smaller_from_larger_overlapping__expect_splitted_into_two_ranges)
      {
         common::unittest::Trace trace;

         std::vector< int> container( 100);

         auto result = range::position::subtract( container, range::make( std::begin( container) + 20, 40));

         ASSERT_TRUE( ! std::get< 0>( result).empty());
         EXPECT_TRUE( std::get< 0>( result).size() == 20);
         EXPECT_TRUE( std::get< 0>( result).begin() == std::begin( container));
         EXPECT_TRUE( std::get< 0>( result).end() == std::begin( container) + 20);

         ASSERT_TRUE( ! std::get< 1>( result).empty());
         EXPECT_TRUE( std::get< 1>( result).size() == 40) << "std::get< 1>( result).size(): " << std::get< 1>( result).size();
         EXPECT_TRUE( std::get< 1>( result).begin() == std::begin( container) + 60) << "std::distance( std::get< 1>( result).first, std::begin( container) + 60): " << std::distance( std::get< 1>( result).begin(), std::begin( container) + 60);
         EXPECT_TRUE( std::get< 1>( result).end() == std::end( container)) << "std::distance( std::get< 1>( result).last, std::end( container)): " << std::distance( std::get< 1>( result).end(), std::end( container));
      }




      TEST( common_algorithm, sort)
      {
         common::unittest::Trace trace;

         auto us = local::unsorted();
         auto s = local::sorted();

         // shall not compile
         //auto sorted = algorithm::sort( range::make( local::unsorted()));

         auto sorted = algorithm::sort( us);

         //EXPECT_TRUE( sorted);
         EXPECT_TRUE( ! sorted.empty());
         EXPECT_TRUE( algorithm::equal( sorted, s));
      }


      TEST( common_algorithm, sort_predicate)
      {
         common::unittest::Trace trace;

         auto us = local::unsorted();
         auto s = local::sorted();

         auto sorted = algorithm::sort( us, std::less<>());

         //EXPECT_TRUE( sorted);
         EXPECT_TRUE( ! sorted.empty());
         EXPECT_TRUE( algorithm::equal( sorted, s));
      }

      TEST( common_algorithm, sort_reverse)
      {
         common::unittest::Trace trace;

         auto us = local::unsorted();
         auto s = local::sorted();

         auto sorted = algorithm::sort( range::reverse( us));

         //EXPECT_TRUE( sorted);
         EXPECT_TRUE( ! sorted.empty());
         EXPECT_TRUE( algorithm::equal( sorted, s));
      }


      TEST( common_algorithm, sort_predicate_reverse)
      {
         common::unittest::Trace trace;

         auto us = local::unsorted();
         auto s = local::sorted();

         auto sorted = algorithm::sort( range::reverse( us), std::less<>());

         //EXPECT_TRUE( sorted);
         EXPECT_TRUE( ! sorted.empty());
         EXPECT_TRUE( algorithm::equal( sorted, s)) << trace.compose( "sorted : ", sorted, "\nunsortd: ", range::make( s));

      }

      TEST( common_algorithm, rotate)
      {
         common::unittest::Trace trace;

         auto container = std::vector< int>{ 1, 2, 3, 4, 5, 6, 7, 8};

         auto result = algorithm::rotate( container, std::begin( container) + 1);

         EXPECT_TRUE(( std::get< 0>( result) == std::vector< int>{ 2, 3, 4, 5, 6, 7, 8})) << trace.compose( std::get< 0>( result));
         EXPECT_TRUE(( std::get< 1>( result) == std::vector< int>{ 1})) << trace.compose( std::get< 1>( result));
      }

      TEST( common_algorithm, partition)
      {
         common::unittest::Trace trace;

         auto us = local::unsorted();

         auto part = algorithm::partition( us, []( int value) { return value == 3;});

         ASSERT_TRUE( std::get< 0>( part).size() == 2);
         EXPECT_TRUE( *std::get< 0>( part) == 3);
         EXPECT_TRUE( std::get< 0>( part).size() + std::get< 1>( part).size() == range::size( us));
      }

      TEST( common_algorithm, partition_reverse)
      {
         common::unittest::Trace trace;

         auto us = local::unsorted();

         auto part = algorithm::partition( range::reverse( us), []( int value) { return value == 3;});

         ASSERT_TRUE( std::get< 0>( part).size() == 2);
         EXPECT_TRUE( *std::get< 0>( part) == 3);
         EXPECT_TRUE( std::get< 0>( part).size() + std::get< 1>( part).size() == range::size( us));
      }

      TEST( common_algorithm, find_value)
      {
         common::unittest::Trace trace;

         auto us = local::unsorted();

         auto found =  algorithm::find( us, 3);

         ASSERT_TRUE( ! found.empty());
         EXPECT_TRUE( *found == 3);
      }

      TEST( common_algorithm, find_map_value)
      {
         common::unittest::Trace trace;

         std::map< int, std::string> container{ {1, "one"}, {2, "two"}, {3, "three"}, {4, "four"}};

         auto found =  algorithm::find( container, 3);

         ASSERT_TRUE( ! found.empty());
         EXPECT_TRUE( found->second == "three");
      }

      TEST( common_algorithm, find_value_reverse)
      {
         common::unittest::Trace trace;

         auto us = local::unsorted();

         auto found =  algorithm::find( range::reverse( us), 3);

         ASSERT_TRUE( ! found.empty());
         EXPECT_TRUE( *found == 3);
      }

      TEST( common_algorithm, contains_vector)
      {
         common::unittest::Trace trace;

         auto values = std::vector< int>{ 0, 1, 2, 3, 5, 6, 7, 8, 9};

         EXPECT_TRUE( algorithm::contains( values, 0));
         EXPECT_TRUE( algorithm::contains( values, 9));
         EXPECT_TRUE( ! algorithm::contains( values, 42));
      }

      TEST( common_algorithm, contains_map)
      {
         common::unittest::Trace trace;

         auto values = std::map< int, int>{ { 0, 42}, { 1, 42}, { 2, 42}, { 3, 42}, { 5, 42}, { 6, 42}, { 7, 42}, { 8, 42}};

         EXPECT_TRUE( algorithm::contains( values, 0));
         EXPECT_TRUE( algorithm::contains( values, 8));
         EXPECT_TRUE( ! algorithm::contains( values, 42));
      }

      TEST( common_algorithm, search)
      {
         common::unittest::Trace trace;

         auto found = algorithm::search( "some text to search", std::string{ "some"});
         EXPECT_TRUE( ! found.empty());

      }


      TEST( common_algorithm, uniform__true)
      {
         common::unittest::Trace trace;

         std::vector< int> uniform{ 1, 1, 1, 1, 1, 1, 1, 1, 1};

         EXPECT_TRUE( algorithm::uniform (uniform));

      }

      TEST( common_algorithm, uniform__false)
      {
         common::unittest::Trace trace;

         std::vector< int> uniform{ 1, 1, 1, 1, 1, 1, 1, 2, 1};

         EXPECT_FALSE( algorithm::uniform (uniform));
      }


      TEST( common_algorithm, sort_partition_find_value)
      {
         common::unittest::Trace trace;

         auto us = local::unsorted();


         auto found =  algorithm::find(
               std::get< 0>( algorithm::partition(
                     algorithm::sort( us),
               []( int value) { return value == 3;})), 3);

         ASSERT_TRUE( ! found.empty());
         EXPECT_TRUE( *found == 3);
      }

      TEST( common_algorithm, trim_container)
      {
         common::unittest::Trace trace;

         auto s = local::sorted();

         auto found =  algorithm::find( s, 3);
         auto result = algorithm::trim( s, found);

         EXPECT_TRUE( algorithm::equal( result, s));

      }


      TEST( common_algorithm, sort_unique_trim_container)
      {
         common::unittest::Trace trace;

         std::vector< int> set{ 3, 1, 3, 2, 1, 3, 1, 2, 3};

         algorithm::trim( set, algorithm::unique( algorithm::sort( set)));

         ASSERT_TRUE( set.size() == 3);
         EXPECT_TRUE( set.at( 0) == 1);
         EXPECT_TRUE( set.at( 1) == 2);
         EXPECT_TRUE( set.at( 2) == 3);
      }


      TEST( common_algorithm, duplicates)
      {
         common::unittest::Trace trace;

         std::vector< int> set{ 1, 2, 3, 4, 5, 6, 7, 1, 3};

         auto duplicates = algorithm::duplicates( algorithm::sort( set));

         ASSERT_TRUE( duplicates.size() == 2) << trace.compose( "duplicates: ", duplicates);
         EXPECT_TRUE( set.at( 0) == 1);
         EXPECT_TRUE( set.at( 1) == 3);
      }


      TEST( common_algorithm, copy_empty)
      {
         common::unittest::Trace trace;

         const std::string source;
         std::string target;

         algorithm::copy( source, target);

         EXPECT_TRUE( target.empty()) << target;
      }

      TEST( common_algorithm, copy_string_to_existing_string__expect_overwrite)
      {
         common::unittest::Trace trace;

         const std::string source = "foo-bar";
         std::string target = "xxx";

         algorithm::copy( source, target);

         EXPECT_TRUE( target == source) << target;
      }

      TEST( common_algorithm, copy_max__source_shorter_than_target)
      {
         common::unittest::Trace trace;

         const std::string source = "1234";
         std::string target;

         algorithm::copy_max( source, 50, std::back_inserter( target));

         EXPECT_TRUE( target == "1234") << target;

      }

      TEST( common_algorithm, copy_max__source_equal_to_target)
      {
         common::unittest::Trace trace;

         const std::string source = "1234567";
         std::string target;

         algorithm::copy_max( source, source.size(), std::back_inserter( target));

         EXPECT_TRUE( source == target) << target;
      }

      TEST( common_algorithm, copy_max_range__source_equal_to_target)
      {
         common::unittest::Trace trace;

         const std::string source = "1234567";
         std::string target;
         target.resize( source.size());

         algorithm::copy_max( source, target);

         EXPECT_TRUE( source == target) << "\nsource: '" << source << "'\ntarget: '" << target << "'";

      }

      TEST( common_algorithm, copy_max__source_longer_than_target)
      {
         common::unittest::Trace trace;

         const std::string source = "1234567";
         std::string target;

         algorithm::copy_max( source, 4, std::back_inserter( target));

         EXPECT_TRUE( target == "1234") << target;
      }

      TEST( common_algorithm, copy_max__range__source_longer_than_target)
      {
         common::unittest::Trace trace;

         const std::string source = "1234567";
         std::string target;
         target.resize( 4);

         algorithm::copy_max( source, target);

         EXPECT_TRUE( target == "1234") << "\nsource: '" << source << "'\ntarget: '" << target << "'";
      }

      TEST( common_algorithm, intersection)
      {
         common::unittest::Trace trace;

         std::vector< int> range{ 9, 3, 1, 7, 4, 2, 5, 8, 6};
         std::vector< int> lookup{ 4, 1, 3, 5, 2};

         auto split = algorithm::intersection( range, lookup);

         EXPECT_TRUE( algorithm::equal( algorithm::sort( std::get< 0>( split)), algorithm::sort( lookup))) << trace.compose( std::get< 0>( split));
         EXPECT_TRUE( algorithm::equal( std::get< 1>( split), ( std::vector< int>{ 9, 7, 8, 6})));
      }

      namespace local
      {
         namespace
         {
            namespace intersection
            {
               struct common
               {
                  common( int v) : value{ v} {}
                  int value = 0;
                  friend bool operator == ( const common& lhs, const common& rhs) { return lhs.value == rhs.value;}
                  friend bool operator < ( const common& lhs, const common& rhs) { return lhs.value < rhs.value;}
               };

               struct A : common
               {
                  using common::common;
               };

               struct B : common
               {
                  using common::common;
               };


            } // intersection

         } // <unnamed>
      } // local

      TEST( common_algorithm, intersection_predicate)
      {
         common::unittest::Trace trace;

         using namespace local::intersection;

         std::vector< A> range{ 9, 3, 1, 7, 4, 2, 5, 8, 6};
         std::vector< B> lookup{ 4, 1, 3, 5, 2};

         auto split = algorithm::intersection( range, lookup, []( const A& a, const B& b){ return a.value == b.value;});

         EXPECT_TRUE( algorithm::equal( algorithm::sort( std::get< 0>( split)), algorithm::sort( lookup)));// << std::get< 0>( split);
         EXPECT_TRUE( algorithm::equal( std::get< 1>( split), ( std::vector< int>{ 9, 7, 8, 6})));
      }


      TEST( common_algorithm_coalesce, lvalue_string__expect_lvalue)
      {
         common::unittest::Trace trace;

         std::string first;
         std::string second;

         EXPECT_TRUE( std::is_lvalue_reference< decltype( algorithm::coalesce( first, second))>::value);
      }

      TEST( common_algorithm_coalesce, rvalue_string_first__expect_rvalue)
      {
         common::unittest::Trace trace;

         std::string first;
         std::string second;

         EXPECT_TRUE( ! std::is_lvalue_reference< decltype( algorithm::coalesce( std::move( first), second))>::value);
      }

      TEST( common_algorithm_coalesce, lvalue_string_first__literal_string_second__expect_string_rvalue)
      {
         common::unittest::Trace trace;

         std::string first;

         EXPECT_TRUE( ! std::is_lvalue_reference< decltype( algorithm::coalesce( first, "0"))>::value);
         EXPECT_TRUE( ( std::is_same< decltype( algorithm::coalesce( first, "0")), std::string>::value));
         EXPECT_TRUE( algorithm::coalesce( first, "0") == "0");
      }

      TEST( common_algorithm_coalesce, int_nullptr_first__int_pointer_second__expect_second)
      {
         common::unittest::Trace trace;

         int* first = nullptr;
         int temp = 42;
         int* second = &temp;
         
         EXPECT_TRUE( algorithm::coalesce( first, second) == second);
         EXPECT_TRUE( *algorithm::coalesce( first, second) == 42);
      }


      TEST( common_algorithm_coalesce, lvalue_string_10__last_literal___expect_string_rvalue)
      {
         common::unittest::Trace trace;

         auto coalesce_strings = []() {
            std::vector< std::string> strings( 10);

            return algorithm::coalesce(
                  strings.at( 0),
                  strings.at( 1),
                  strings.at( 2),
                  strings.at( 3),
                  strings.at( 4),
                  strings.at( 5),
                  strings.at( 6),
                  strings.at( 7),
                  strings.at( 8),
                  strings.at( 9),
                  "0");

         };

         EXPECT_TRUE( ! std::is_lvalue_reference< decltype( coalesce_strings())>::value);
         EXPECT_TRUE( ( std::is_same< decltype( coalesce_strings()), std::string>::value));
         EXPECT_TRUE(coalesce_strings() == "0");
      }



      TEST( common_algorithm_compare_any, value_1__to_1__expect_true)
      {
         common::unittest::Trace trace;

         EXPECT_TRUE( algorithm::compare::any( 1, 1));
      }

      TEST( common_algorithm_compare_any, value_1__to_2__expect_false)
      {
         common::unittest::Trace trace;

         EXPECT_FALSE( algorithm::compare::any( 1, 2));
      }

      TEST( common_algorithm_compare_any, value_1__to_5_3_2_1__expect_true)
      {
         common::unittest::Trace trace;

         EXPECT_TRUE( algorithm::compare::any( 1, 5, 4, 3, 2, 1));
      }


      TEST( common_algorithm_remove, empty_empty__expect_empty)
      {
         common::unittest::Trace trace;

         std::vector< int> container;

         EXPECT_TRUE( algorithm::remove( range::make( container), range::make( container)).empty());
      }

      TEST( common_algorithm_remove, source_4__unwanted_empty__expect_4)
      {
         common::unittest::Trace trace;

         std::vector< int> container{ 1, 2, 3, 4};

         EXPECT_TRUE( algorithm::remove( range::make( container), range::make( std::begin( container), 0)) == container);
      }

      TEST( common_algorithm_remove, source_4__unwanted_first__expect_3)
      {
         common::unittest::Trace trace;

         std::vector< int> container{ 1, 2, 3, 4};

         EXPECT_TRUE(( algorithm::remove( range::make( container), range::make( std::begin( container), 1)) == std::vector< int>{ 2, 3, 4}));
      }


      TEST( common_algorithm_remove, source_4__unwanted_last__expect_3)
      {
         common::unittest::Trace trace;

         std::vector< int> container{ 1, 2, 3, 4};

         EXPECT_TRUE(( algorithm::remove( range::make( container), range::make( std::end( container) - 1, 1)) == std::vector< int>{ 1, 2, 3}));
      }

      TEST( common_algorithm_remove, source_4__unwanted_middle_2__expect_2)
      {
         common::unittest::Trace trace;

         std::vector< int> container{ 1, 2, 3, 4};

         EXPECT_TRUE(( algorithm::remove( range::make( container), range::make( std::begin( container) + 1, 2)) == std::vector< int>{ 1, 4}));
      }

      TEST( common_algorithm_append_unique, empty__exepct_empty)
      {
         common::unittest::Trace trace;

         const std::vector< int> source;
         std::vector< int> target;

         algorithm::append_unique( source, target);

         EXPECT_TRUE( target.empty());
      }

      TEST( common_algorithm_append_unique, unique_values__exepct_all)
      {
         common::unittest::Trace trace;

         const std::vector< int> source{ 1, 2, 3, 4};
         std::vector< int> target;

         algorithm::append_unique( source, target);

         EXPECT_TRUE( target == source);
      }

      TEST( common_algorithm_append_unique, not_unique_values__exepct_some)
      {
         common::unittest::Trace trace;

         const std::vector< int> source{ 1, 2, 4, 3, 1, 1, 3, 3, 4, 4, 4, 4, 4, 2, 2, 2, 2, 2, 2};
         std::vector< int> target;

         algorithm::append_unique( source, target);

         EXPECT_TRUE(( target == std::vector< int>{ 1, 2, 4, 3}));
      }

      TEST( common_algorithm_transform_if, rvalue)
      {
         common::unittest::Trace trace;

         const std::vector< int> source{ 1, 2, 4, 3, 1, 1, 3, 3, 4, 4, 4, 4, 4, 2, 2, 2, 2, 2, 2};
         auto result = algorithm::transform_if( source, std::vector< std::string>{}, []( auto v){ return std::to_string( v);}, []( auto v){ return v == 3;});

         EXPECT_TRUE(( result == std::vector< std::string>{ "3", "3", "3"}));
      }

      TEST( common_algorithm_transform_if, back_inserter)
      {
         common::unittest::Trace trace;

         const std::vector< int> source{ 1, 2, 4, 3, 1, 1, 3, 3, 4, 4, 4, 4, 4, 2, 2, 2, 2, 2, 2};
         std::vector< std::string> result;
         algorithm::transform_if( source, std::back_inserter( result), []( auto v){ return std::to_string( v);}, []( auto v){ return v == 3;});

         EXPECT_TRUE(( result == std::vector< std::string>{ "3", "3", "3"}));
      }

      TEST( common_algorithm_is_unique, empty__expect_true)
      {
         common::unittest::Trace trace;
         std::vector< int> range;

         EXPECT_TRUE( algorithm::is::unique( range));
      }

      TEST( common_algorithm_is_unique, some_unique_values__expect_true)
      {
         common::unittest::Trace trace;
         std::vector< int> range{ 5, 6, 7, 1, 2, 3, 4};

         EXPECT_TRUE( algorithm::is::unique( range));
      }

      TEST( common_algorithm_is_unique, some_not_unique_values__expect_false)
      {
         common::unittest::Trace trace;
         std::vector< int> range{ 5, 6, 7, 1, 2, 3, 4, 2};

         EXPECT_TRUE( ! algorithm::is::unique( range));
      }

      TEST( common_algorithm_is_unique, last_two_not_unique__expect_false)
      {
         common::unittest::Trace trace;
         std::vector< int> range{ 5, 6, 7, 1, 2, 3, 4, 42, 42};

         EXPECT_TRUE( ! algorithm::is::unique( range));
      }
   } // common

} // casual

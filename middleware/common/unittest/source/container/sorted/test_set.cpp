//!
//! Copyright (c) 2022, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!


#include "common/unittest.h"

#include "casual/container/sorted/set.h"

#include "common/algorithm/is.h"
#include "common/array.h"
#include "common/compare.h"

#include <string>

namespace casual
{
   using namespace common;

   TEST( container_sorted_set, immutability_non_const)
   {
      container::sorted::Set< long> set( array::make( 5, 6, 1, 3, 34));

      static_assert( traits::is::same_v< const long&, decltype( *std::begin( set))>);
      static_assert( traits::is::same_v< const long&, decltype( *std::end( set))>);  
   }

   TEST( container_sorted_set, immutability_const)
   {
      const container::sorted::Set< long> set( array::make( 5, 6, 1, 3, 34));

      static_assert( traits::is::same_v< const long&, decltype( *std::begin( set))>);
      static_assert( traits::is::same_v< const long&, decltype( *std::end( set))>);  
   }

   TEST( container_sorted_set, default_ctor)
   {
      container::sorted::Set< long> set;
      EXPECT_TRUE( set.empty());
   }

   TEST( container_sorted_set, range_ctor)
   {
      auto origin = array::make( 5, 6, 1, 3, 34, 32425, 2342, 23, 1231);

      container::sorted::Set< long> set( std::begin( origin), std::end( origin));
      EXPECT_TRUE( set.size() == origin.size());
      EXPECT_TRUE( algorithm::is::sorted( set));
      EXPECT_TRUE( algorithm::equal( set, algorithm::sort( origin)));
   }

   TEST( container_sorted_set, move_sematics)
   {
      auto origin = array::make( 5, 6, 1, 3, 34, 32425, 2342, 23, 1231);

      container::sorted::Set< long> a( std::begin( origin), std::end( origin));

      auto b = std::move( a);
      EXPECT_TRUE( a.empty());

      EXPECT_TRUE( b.size() == origin.size());
      EXPECT_TRUE( algorithm::is::sorted( b));
      EXPECT_TRUE( algorithm::equal( b, algorithm::sort( origin)));
   }

   TEST( container_sorted_set, insert_long)
   {
      container::sorted::Set< long> set;
      set.insert( 5);
      set.insert( 9);
      set.insert( 1);
      set.insert( 3);
      
      EXPECT_TRUE( set.size() == 4);
      EXPECT_TRUE( algorithm::is::sorted( set));
   }

   TEST( container_sorted_set, erase_iterator)
   {
      container::sorted::Set< long> set{ array::make( 1, 2, 3, 4, 5)};

      EXPECT_TRUE( *std::next( std::begin( set)) == 2);
      set.erase( std::next( std::begin( set)));

      EXPECT_TRUE( algorithm::equal( set, array::make( 1, 3, 4, 5))) << CASUAL_NAMED_VALUE( set);
   }

   TEST( container_sorted_set, erase_range)
   {
      container::sorted::Set< long> set{ array::make( 1, 2, 3, 4, 5)};

      // remove [2, 3]
      set.erase( range::make( std::next( std::begin( set)), 2));

      EXPECT_TRUE( algorithm::equal( set, array::make( 1,  4, 5))) << CASUAL_NAMED_VALUE( set);
   }

   TEST( container_sorted_set, equal)
   {
      using Set = container::sorted::Set< long>;

      // [ 1, 3, 4, 5]
      EXPECT_TRUE( Set( array::make( 1, 1, 3, 4, 5)) == Set( array::make( 1, 3, 3, 4, 5)));
      EXPECT_TRUE( ! ( Set( array::make( 1, 1, 3, 4, 5)) < Set( array::make( 1, 3, 3, 4, 5))));
      EXPECT_TRUE( ! ( Set( array::make( 1, 1, 3, 4, 5)) > Set( array::make( 1, 3, 3, 4, 5))));
   }

   TEST( container_sorted_set, less)
   {
      using Set = container::sorted::Set< long>;
   
      EXPECT_TRUE( Set( array::make( 1, 2, 3, 4)) < Set( array::make( 2, 3, 4, 5)));
      EXPECT_TRUE( Set( array::make( 1, 2, 3, 4)) != Set( array::make( 2, 3, 4, 5)));
   }

   TEST( container_sorted_set, extract)
   {
      container::sorted::Set< long> set{ array::make( 1, 2, 3, 4, 5)};

      // extract 2
      auto value = set.extract( std::next( std::begin( set)));

      EXPECT_TRUE( value == 2);
      EXPECT_TRUE( algorithm::equal( set, array::make( 1, 3, 4, 5))) << CASUAL_NAMED_VALUE( set);
   }

   TEST( container_sorted_set, container_sorted_set_insert_long_range)
   {
      auto origin = array::make( 5, 6, 1, 3);

      container::sorted::Set< long> set{ origin};
      
      EXPECT_TRUE( set.size() == 4);
      EXPECT_TRUE( algorithm::is::sorted( set));
      EXPECT_TRUE( algorithm::equal( set, algorithm::sort( origin)));
   }

   namespace local
   {
      namespace
      {
         struct Value : Compare< Value>
         {
            Value( std::string s, long l) 
               : s{ std::move( s)}, l{ l} {}
            
            std::string s;
            long l{};

            auto tie() const noexcept { return std::tie( s, l);}
         };
      } // <unnamed>
   } // local

   TEST( container_sorted_set, emplace)
   {
      container::sorted::Set< local::Value> set;

      EXPECT_TRUE( std::get< 1>( set.emplace( "b", 4)));
      EXPECT_TRUE( *std::get< 0>( set.emplace( "a", 42)) == local::Value( "a", 42));
      set.emplace( "b", 2);
      EXPECT_TRUE( ! std::get< 1>( set.emplace( "b", 4)));

      EXPECT_TRUE( algorithm::equal( set, array::make( local::Value( "a", 42), local::Value( "b", 2), local::Value( "b", 4))));
   }


   TEST( container_sorted_set, deduction_guide)
   {
      auto origin = array::make( 5, 6, 1, 3);

      container::sorted::Set set{ origin};
      
      EXPECT_TRUE( set.size() == 4);
      EXPECT_TRUE( algorithm::is::sorted( set));
      EXPECT_TRUE( algorithm::equal( set, algorithm::sort( origin)));
   }

   TEST( container_sorted_set, move_iterator__insert)
   {
      using namespace std::string_literals;

      auto origin = array::make( "some long string without SSO - B"s, "some long string without SSO - C"s, "some long string without SSO - A"s, "some long string without SSO - D"s);

      // the "move range" should trigger move for each element from origin
      container::sorted::Set< std::string> a( common::range::move( origin));

      EXPECT_TRUE( algorithm::all_of( origin, []( auto& element){ return element.empty();}));

      auto b = std::move( a);
      EXPECT_TRUE( a.empty());

      EXPECT_TRUE( b.size() == origin.size()) << CASUAL_NAMED_VALUE( b);
      EXPECT_TRUE( algorithm::is::sorted( b));
   }

} // casual
//!
//! Copyright (c) 2024, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!

#include "common/unittest.h"

#include "casual/container.h"

namespace casual
{
   TEST( container_index, empty)
   {
      common::unittest::Trace trace;

      container::Index< int> data;

      EXPECT_TRUE( data.empty());
      EXPECT_TRUE( data.size() == 0);
   }

   TEST( container_index, insert)
   {
      common::unittest::Trace trace;

      container::Index< char> data;
      auto a = data.insert( 'a');
      auto b = data.insert( 'b');
      auto c = data.insert( 'c');

      EXPECT_TRUE( a != b);

      EXPECT_TRUE( a.value() == 0);
      EXPECT_TRUE( b.value() == 1);
      EXPECT_TRUE( c.value() == 2);

      EXPECT_TRUE( data[ a] == 'a');
      EXPECT_TRUE( data[ b] == 'b');
      EXPECT_TRUE( data[ c] == 'c');
   }

   TEST( container_index, update)
   {
      common::unittest::Trace trace;

      container::Index< char> data;
      auto a = data.insert( 'a');
      auto b = data.insert( 'b');
      auto c = data.insert( 'c');

      data[ a] = 'x';
      data[ b] = 'y';
      data[ c] = 'z';

      EXPECT_TRUE( data[ a] == 'x');
      EXPECT_TRUE( data[ b] == 'y');
      EXPECT_TRUE( data[ c] == 'z');
   }

   TEST( container_index, erase)
   {
      common::unittest::Trace trace;

      auto count_values = []( auto& index)
      {
         return std::ranges::count_if( index, []( auto& value){ return value.has_value();});
      };

      container::Index< char> data;
      auto a = data.insert( 'a');
      auto b = data.insert( 'b');
      auto c = data.insert( 'c');


      EXPECT_TRUE( b.value() == 1);

      data.erase( a);
      EXPECT_TRUE( count_values( data) == 2);

      data.erase( c);
      EXPECT_TRUE( count_values( data) == 1);

      auto x = data.insert( 'x');
      EXPECT_TRUE( x.value() == 0);

      auto y = data.insert( 'y');
      EXPECT_TRUE( y.value() == 2);

      EXPECT_TRUE( count_values( data) == 3);
   }

   
} // casual
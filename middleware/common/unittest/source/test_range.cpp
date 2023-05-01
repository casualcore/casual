//!
//! Copyright (c) 2018, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!


#include <common/unittest.h>


#include "common/range.h"
#include "common/log/stream.h"

namespace casual
{
   namespace common
   {
      TEST( common_range_empty, empty)
      {
         common::unittest::Trace trace;

         std::string owner;

         EXPECT_TRUE( range::empty( owner));
         EXPECT_TRUE( range::empty( range::make( owner)));
      }

      TEST( common_range_empty, not_empty)
      {
         common::unittest::Trace trace;

         std::string owner = "foo-bar";

         EXPECT_TRUE( ! range::empty( owner));
         EXPECT_TRUE( ! range::empty( range::make( owner)));
      }

      TEST( common_range_size, empty)
      {
         common::unittest::Trace trace;

         std::string owner;

         EXPECT_TRUE( range::size( owner) == 0);
         EXPECT_TRUE( range::size( range::make( owner)) == 0);
      }

      TEST( common_range_size, not_empty)
      {
         common::unittest::Trace trace;

         std::string owner = "foo-bar";
         platform::size::type size = owner.size();

         EXPECT_TRUE( range::size( owner) == size);
         EXPECT_TRUE( range::size( range::make( owner)) == size);
      }

      TEST( common_range, reverse)
      {
         common::unittest::Trace trace;

         const std::vector< int> owner{ 1, 2, 3, 4, 5, 6};

         auto forward = range::make( owner);
         auto reverse = range::reverse( forward);

         EXPECT_TRUE(( algorithm::equal( reverse, std::vector< int>{ 6, 5, 4, 3, 2, 1})));

         EXPECT_TRUE( algorithm::equal( range::reverse( forward), reverse));
         EXPECT_TRUE( algorithm::equal( range::reverse( reverse), forward));
         EXPECT_TRUE( algorithm::equal( range::reverse( range::reverse( forward)), forward));

         static_assert( std::is_same< decltype( forward), decltype( range::reverse( range::reverse( forward)))>::value, ""); 
      }

   } // common
} // casual
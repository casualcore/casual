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
      TEST( casual_common_range_empty, empty)
      {
         common::unittest::Trace trace;

         std::string owner;

         EXPECT_TRUE( range::empty( owner));
         EXPECT_TRUE( range::empty( range::make( owner)));
      }

      TEST( casual_common_range_empty, not_empty)
      {
         common::unittest::Trace trace;

         std::string owner = "foo-bar";

         EXPECT_TRUE( ! range::empty( owner));
         EXPECT_TRUE( ! range::empty( range::make( owner)));
      }

      TEST( casual_common_range_size, empty)
      {
         common::unittest::Trace trace;

         std::string owner;

         EXPECT_TRUE( range::size( owner) == 0);
         EXPECT_TRUE( range::size( range::make( owner)) == 0);
      }

      TEST( casual_common_range_size, not_empty)
      {
         common::unittest::Trace trace;

         std::string owner = "foo-bar";
         platform::size::type size = owner.size();

         EXPECT_TRUE( range::size( owner) == size);
         EXPECT_TRUE( range::size( range::make( owner)) == size);
      }
   } // common
} // casual
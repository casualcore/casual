//! 
//! Copyright (c) 2015, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!


#include <gtest/gtest.h>


#include "serviceframework/archive/traits.h"

#include <vector>

namespace casual
{

   namespace local
   {
      struct Test
      {

      };
   }

   TEST( serviceframework_traits, is_container)
   {
      EXPECT_TRUE( serviceframework::traits::container::is_container< std::vector< int>>::value);
      EXPECT_FALSE( serviceframework::traits::container::is_container< Test>::value);
   }
}



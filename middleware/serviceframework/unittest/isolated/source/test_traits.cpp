//!
//! casual_isolatedunittest_traits.cpp
//!
//! Created on: Oct 21, 2012
//!     Author: Lazan
//!

#include <gtest/gtest.h>


#include "sf/archive/traits.h"

#include <vector>

namespace casual
{

   namespace local
   {
      struct Test
      {

      };
   }

   TEST( casual_sf_traits, is_container)
   {
      EXPECT_TRUE( sf::traits::container::is_container< std::vector< int>>::value);
      EXPECT_FALSE( sf::traits::container::is_container< Test>::value);
   }
}



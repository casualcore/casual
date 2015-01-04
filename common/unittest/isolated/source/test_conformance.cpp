//!
//! test_conformance.cpp
//!
//! Created on: Dec 21, 2014
//!     Author: Lazan
//!


#include <gtest/gtest.h>


#include "common/algorithm.h"

#include <type_traits>

namespace casual
{
   namespace common
   {
      TEST( casual_common_conformance, struct_with_pod_attributes__is_pod)
      {
         struct POD
         {
            int int_value;
         };

         EXPECT_TRUE( std::is_pod< POD>::value);
      }

      TEST( casual_common_conformance, struct_with_member_function__is_pod)
      {
         struct POD
         {
            int func1() { return 42;}
         };

         EXPECT_TRUE( std::is_pod< POD>::value);
      }



      TEST( casual_common_conformance, bind)
      {
         struct POD
         {
            int test = 42;
         };

         auto bind = std::bind( equal_to{}, std::bind( &POD::test, std::placeholders::_1), std::placeholders::_2);

         POD pod;

         EXPECT_TRUE( bind( pod, 42) == true);
      }

   } // common
} // casual

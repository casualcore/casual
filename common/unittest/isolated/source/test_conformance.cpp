//!
//! test_conformance.cpp
//!
//! Created on: Dec 21, 2014
//!     Author: Lazan
//!


#include <gtest/gtest.h>



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

   } // common
} // casual

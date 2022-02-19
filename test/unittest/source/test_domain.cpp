//! 
//! Copyright (c) 2015, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!


#include "common/unittest.h"

#include "domain/unittest/manager.h"

#include "common/communication/instance.h"

namespace casual
{
   using namespace common;

   namespace test
   {
      
      TEST( test_domain, no_configuration)
      {
         common::unittest::Trace trace;

         auto manager = casual::domain::unittest::manager();

         EXPECT_TRUE( communication::instance::ping( manager.handle().ipc) == manager.handle());
      }


      TEST( test_domain, two_domain_empty_configuration)
      {
         common::unittest::Trace trace;

         constexpr auto A = R"(
domain:
   name: A
)";

         constexpr auto B = R"(
domain:
   name: B
)";

         auto a = casual::domain::unittest::manager( A);
         auto b = casual::domain::unittest::manager( B);

         EXPECT_TRUE( communication::instance::ping( a.handle().ipc) == a.handle());
         EXPECT_TRUE( communication::instance::ping( b.handle().ipc) == b.handle());
      }


   } // domain

} // casual

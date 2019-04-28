//! 
//! Copyright (c) 2015, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!


#include "common/unittest.h"

#include "casual/test/domain.h"

#include "common/communication/instance.h"

namespace casual
{
   using namespace common;

   namespace test
   {
      namespace domain
      {
        
         TEST( test_domain_basic, empty_configuration)
         {
            common::unittest::Trace trace;

            constexpr auto configuration = R"(
domain:
  name: empty_configuration
)";

            domain::Manager manager{ { configuration}};

            EXPECT_TRUE( communication::instance::ping( manager.handle().ipc) == manager.handle());
         }


         TEST( test_domain_basic, two_domain_empty_configuration)
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

            domain::Manager a{ {A}};
            domain::Manager b{ {B}};

            EXPECT_TRUE( communication::instance::ping( a.handle().ipc) == a.handle());
            EXPECT_TRUE( communication::instance::ping( b.handle().ipc) == b.handle());
         }

      } // domain

   } // test
} // casual

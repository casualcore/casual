//!
//! casual
//!

#include "common/unittest.h"

#include "common/server/service.h"
#include "xatmi.h"

namespace casual
{
   namespace common
   {

      namespace local
      {
         namespace
         {
            void service1( TPSVCINFO *) {}
            void service2( TPSVCINFO *) {}
         } // <unnamed>
      } // local



      TEST( common_server_service, equality)
      {
         common::unittest::Trace trace;

         auto s1 = server::xatmi::service( ".1", &local::service1);
         auto s2 = server::xatmi::service( ".2", &local::service1);

         EXPECT_TRUE( s1 == s2);
      }

      TEST( common_server_service, in_equality)
      {
         common::unittest::Trace trace;

         auto s1 = server::xatmi::service( ".1", &local::service1);
         auto s2 = server::xatmi::service( ".2", &local::service2);

         EXPECT_TRUE( s1 != s2) << "s1: " << s1 << " - s2: " << s2;
      }




   } // common

} // casual

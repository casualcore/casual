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
            void service3( TPSVCINFO *, int) {}
         } // <unnamed>
      } // local



      TEST( common_server_service, equality)
      {
         common::unittest::Trace trace;

         server::Service s1{ ".1", &local::service1};
         server::Service s2{ ".2", &local::service1};

         EXPECT_TRUE( s1 == s2);
      }

      TEST( common_server_service, in_equality)
      {
         common::unittest::Trace trace;

         server::Service s1{ ".1", &local::service1};
         server::Service s2{ ".2", &local::service2};

         EXPECT_TRUE( s1 != s2);
      }


      TEST( common_server_service, bind_argument)
      {
         common::unittest::Trace trace;

         server::Service s1{ ".1", std::bind( &local::service3, std::placeholders::_1, 10)};
         server::Service s2{ ".2", std::bind( &local::service3, std::placeholders::_1, 10)};

         EXPECT_TRUE( s1 == s2);
      }




   } // common

} // casual

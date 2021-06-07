//!
//! Copyright (c) 2021, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!


// to be able to use 'raw' flags and codes
// since we undefine 'all' of them in common
#define CASUAL_NO_XATMI_UNDEFINE

#include "common/unittest.h"

#include "domain/manager/unittest/process.h"

#include "service/manager/admin/server.h"
#include "service/manager/admin/model.h"

#include "casual/xatmi.h"

namespace casual
{
   using namespace common;

   namespace test::domain::assassinate
   {
      namespace local
      {
         namespace
         {
            using Manager = casual::domain::manager::unittest::Process;
         } // <unnamed>
      } // local

      TEST( test_domain_assassinate, timeout_1ms__call_)
      {
         unittest::Trace trace;

         constexpr auto configuration = R"(
domain: 
   name: A

   groups: 
      -  name: base
      -  name: user
         dependencies: [ base]
   
   service:
      execution:
         timeout:
            duration: 2ms
            contract: kill
   servers:
      -  path: "${CASUAL_REPOSITORY_ROOT}/middleware/service/bin/casual-service-manager"
         memberships: [ base]
      -  path: "${CASUAL_REPOSITORY_ROOT}/middleware/transaction/bin/casual-transaction-manager"
         memberships: [ base]
         
      -  path: "${CASUAL_REPOSITORY_ROOT}/middleware/example/server/bin/casual-example-server"
         memberships: [ user]
         arguments: [ --sleep, 1s]

)";


         local::Manager domain{ { configuration}};

         auto start = platform::time::clock::type::now();

         auto buffer = tpalloc( X_OCTET, nullptr, 128);
         auto len = tptypes( buffer, nullptr, nullptr);

         EXPECT_TRUE( tpcall( "casual/example/sleep", buffer, 128, &buffer, &len, 0) == -1);
         EXPECT_TRUE( tperrno == TPETIME) << "tperrno: " << tperrnostring( tperrno);
         EXPECT_TRUE( platform::time::clock::type::now() - start > std::chrono::milliseconds{ 2});

         tpfree( buffer);

      }



   } //test::domain::assassinate

} // casual
//!
//! Copyright (c) 2021, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!


// to be able to use 'raw' flags and codes
// since we undefine 'all' of them in common
#define CASUAL_NO_XATMI_UNDEFINE

#include "common/unittest.h"

#include "domain/unittest/manager.h"

#include "service/manager/admin/server.h"
#include "service/manager/admin/model.h"

#include "casual/xatmi.h"

namespace casual
{
   using namespace common;

   namespace test
   {
      TEST( test_assassinate, timeout_1ms__call_)
      {
         unittest::Trace trace;

         constexpr auto configuration = R"(
domain: 
   name: A

   groups: 
      - name: base
      - name: user
        dependencies: [ base]
   
   service:
      execution:
         timeout:
            duration: 2ms
            contract: kill
   servers:
      - path: ${CASUAL_MAKE_SOURCE_ROOT}/middleware/service/bin/casual-service-manager
        memberships: [ base]
      - path: ${CASUAL_MAKE_SOURCE_ROOT}/middleware/transaction/bin/casual-transaction-manager
        memberships: [ base]
      - path: ${CASUAL_MAKE_SOURCE_ROOT}/middleware/example/server/bin/casual-example-server
        memberships: [ user]
        arguments: [ --sleep, 1s]
)";

         auto domain = casual::domain::unittest::manager( configuration);

         auto start = platform::time::clock::type::now();

         auto buffer = tpalloc( X_OCTET, nullptr, 128);
         auto len = tptypes( buffer, nullptr, nullptr);

         EXPECT_TRUE( tpcall( "casual/example/sleep", buffer, 128, &buffer, &len, 0) == -1);
         EXPECT_TRUE( tperrno == TPETIME) << "tperrno: " << tperrnostring( tperrno);
         EXPECT_TRUE( platform::time::clock::type::now() - start > std::chrono::milliseconds{ 2});

         tpfree( buffer);

      }

      TEST( test_assassinate, timeout_1ms__call__global_config)
      {
         unittest::Trace trace;

         constexpr auto configuration = R"(
domain: 
   name: A

   global:
      service:
         execution:
            timeout:
               duration: 2ms
               contract: kill

   groups: 
      - name: base
      - name: user
        dependencies: [ base]

   servers:
      - path: ${CASUAL_MAKE_SOURCE_ROOT}/middleware/service/bin/casual-service-manager
        memberships: [ base]
      - path: ${CASUAL_MAKE_SOURCE_ROOT}/middleware/transaction/bin/casual-transaction-manager
        memberships: [ base]
      - path: ${CASUAL_MAKE_SOURCE_ROOT}/middleware/example/server/bin/casual-example-server
        memberships: [ user]
        arguments: [ --sleep, 1s]
)";

         auto domain = casual::domain::unittest::manager( configuration);

         auto start = platform::time::clock::type::now();

         auto buffer = tpalloc( X_OCTET, nullptr, 128);
         auto len = tptypes( buffer, nullptr, nullptr);

         EXPECT_TRUE( tpcall( "casual/example/sleep", buffer, 128, &buffer, &len, 0) == -1);
         EXPECT_TRUE( tperrno == TPETIME) << "tperrno: " << tperrnostring( tperrno);
         EXPECT_TRUE( platform::time::clock::type::now() - start > std::chrono::milliseconds{ 2});

         tpfree( buffer);

      }



   } //test

} // casual
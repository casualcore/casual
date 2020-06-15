//!
//! Copyright (c) 2015, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!

#include "common/unittest.h"

#include "http/outbound/state.h"

#include "common/environment.h"
#include "common/unittest/file.h"

#include "domain/manager/unittest/process.h"


#include "casual/xatmi.h"

namespace casual
{

   namespace http
   {
      namespace outbound
      {

         namespace local
         {
            namespace
            {

               struct Domain 
               {
                  Domain( std::string configuration) 
                     : domain{ { std::move( configuration)}} {}

                  Domain() : Domain{ Domain::configuration} {}

                  common::file::scoped::Path http_configuration = []()
                  {
                     auto result = common::unittest::file::temporary::content( ".yaml", R"(
http:
  services:
    - name: do/not/discard/transaction
      url: a/b.se
      discard_transaction: false

)");
                     common::environment::variable::set( "CASUAL_UNITTEST_HTTP_CONFIGURATION", result.path());
                     return result;
                  }();

                  domain::manager::unittest::Process domain;


                  static constexpr auto configuration = R"(
domain: 
   name: queue-domain

   groups: 
      - name: base
      - name: http
        dependencies: [ base]
   
   servers:
      - path: "${CASUAL_HOME}/bin/casual-service-manager"
        memberships: [ base]
      - path: "${CASUAL_HOME}/bin/casual-transaction-manager"
        memberships: [ base]
      - path: "./bin/casual-http-outbound"
        arguments: [ --configuration, "${CASUAL_UNITTEST_HTTP_CONFIGURATION}"]
        memberships: [ http]


)";
               };

            } // <unnamed>

         } // local

         TEST( http_outbound, boot)
         {
            common::unittest::Trace trace;

            EXPECT_NO_THROW({
               local::Domain domain;
            });
         }

         TEST( http_outbound, in_transaction_call_no_discard__expect_rollback)
         {
            common::unittest::Trace trace;
            local::Domain domain;

            ASSERT_TRUE( tx_begin() == TX_OK);

            auto buffer = tpalloc( "X_OCTET", nullptr, 128);
            long size = 128;

            EXPECT_TRUE( tpcall( "do/not/discard/transaction", buffer, size, &buffer, &size, 0) == -1);

            EXPECT_TRUE( tx_commit() == TX_PROTOCOL_ERROR);
            EXPECT_TRUE( tx_rollback() == TX_OK);

            tpfree( buffer);
         }

      } // outbound
   } // http
} // casual

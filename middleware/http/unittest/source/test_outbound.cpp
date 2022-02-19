//!
//! Copyright (c) 2015, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!

#include "common/unittest.h"

#include "http/outbound/state.h"

#include "common/environment.h"
#include "common/communication/device.h"
#include "common/communication/ipc.h"
#include "common/unittest/file.h"

#include "domain/unittest/manager.h"
#include "domain/discovery/api.h"


#include "casual/xatmi.h"

namespace casual
{
   namespace http::outbound
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
      -  name: do/not/discard/transaction
         url: a.example/do/not/discard/transaction
         discard_transaction: false
      -  name: discard/transaction
         url: a.example/discard/transaction
         discard_transaction: true

)");
                  common::environment::variable::set( "CASUAL_UNITTEST_HTTP_CONFIGURATION", result.string());
                  return result;
               }();

               domain::unittest::Manager domain;


               static constexpr auto configuration = R"(
domain: 
   name: queue-domain

   groups: 
      -  name: base
      -  name: http
         dependencies: [ base]

   servers:
      -  path: ${CASUAL_MAKE_SOURCE_ROOT}/middleware/service/bin/casual-service-manager
         memberships: [ base]
      -  path: ${CASUAL_MAKE_SOURCE_ROOT}/middleware/transaction/bin/casual-transaction-manager
         memberships: [ base]
      -  path: bin/casual-http-outbound
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

      TEST( http_outbound, in_transaction_call_discard__expect_rollback)
      {
         common::unittest::Trace trace;
         local::Domain domain;

         ASSERT_TRUE( tx_begin() == TX_OK);

         auto buffer = tpalloc( "X_OCTET", nullptr, 128);
         long size = 128;

         EXPECT_TRUE( tpcall( "discard/transaction", buffer, size, &buffer, &size, 0) == -1);

         EXPECT_TRUE( tx_commit() == TX_PROTOCOL_ERROR);
         EXPECT_TRUE( tx_rollback() == TX_OK);

         tpfree( buffer);
      }

      TEST( http_outbound, discover)
      {
         common::unittest::Trace trace;
         local::Domain domain;

         casual::domain::discovery::Request request{ common::process::handle()};
         request.domain = common::domain::identity();
         request.content.services = { "do/not/discard/transaction", "discard/transaction"};
         request.directive = decltype( request.directive)::forward;
         auto correlation = casual::domain::discovery::request( request);

         casual::domain::discovery::Reply reply;
         EXPECT_TRUE( common::communication::device::blocking::receive( common::communication::ipc::inbound::device(), reply, correlation));

         EXPECT_TRUE( reply.content.services.size() == 2);
         EXPECT_TRUE( common::algorithm::find( reply.content.services, "do/not/discard/transaction")) << CASUAL_NAMED_VALUE( reply);
         EXPECT_TRUE( common::algorithm::find( reply.content.services, "discard/transaction")) << CASUAL_NAMED_VALUE( reply);
      }

   } // http::outbound
} // casual

//! 
//! Copyright (c) 2015, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!


//
// to be able to use 'raw' flags and codes
// since we undefine 'all' of them in common
//
#define CASUAL_NO_XATMI_UNDEFINE



#include "common/unittest.h"

#include "domain/manager/unittest/process.h"

#include "xatmi.h"


namespace casual
{
   using namespace common;

   namespace xatmi
   {
      namespace local
      {
         namespace
         {

            auto domain()
            {
               return casual::domain::manager::unittest::Process{{
R"(
domain:
   name: test-default-domain

   groups: 
      - name: base
      - name: transaction
        dependencies: [ base]
      - name: queue
        dependencies: [ transaction]
      - name: example
        dependencies: [ queue]

   servers:
      - path: ${CASUAL_HOME}/bin/casual-service-manager
        memberships: [ base]
      - path: ${CASUAL_HOME}/bin/casual-transaction-manager
        memberships: [ transaction]
      - path: ${CASUAL_HOME}/bin/casual-queue-manager
        memberships: [ queue]
      - path: ${CASUAL_HOME}/bin/casual-example-error-server
        memberships: [ example]
      - path: ${CASUAL_HOME}/bin/casual-example-server
        memberships: [ example]
)"
               }};
            }

         } // <unnamed>
      } // local

      TEST( casual_xatmi_conversation, disconnect__invalid_descriptor__expect_TPEBADDESC)
      {
         unittest::Trace trace;

         EXPECT_TRUE( tpdiscon( 42) == -1);
         EXPECT_TRUE( tperrno == TPEBADDESC);
      }

      TEST( casual_xatmi_conversation, send__invalid_descriptor__expect_TPEBADDESC)
      {
         unittest::Trace trace;

         long event = 0;

         EXPECT_TRUE( tpsend( 42, nullptr, 0, 0, &event) == -1);
         EXPECT_TRUE( tperrno == TPEBADDESC);
         EXPECT_TRUE( event == 0);
      }

      TEST( casual_xatmi_conversation, receive__invalid_descriptor__expect_TPEBADDESC)
      {
         unittest::Trace trace;

         long event = 0;

         auto buffer = tpalloc( X_OCTET, nullptr, 128);
         auto len = tptypes( buffer, nullptr, nullptr);

         EXPECT_TRUE( tprecv( 42, &buffer, &len, 0, &event) == -1);
         EXPECT_TRUE( tperrno == TPEBADDESC);
         EXPECT_TRUE( event == 0);

         tpfree( buffer);
      }

      TEST( casual_xatmi_conversation, connect__no_flag__expect_TPEINVAL)
      {
         unittest::Trace trace;

         auto domain = local::domain();

         EXPECT_TRUE( tpconnect( "casual/example/conversation", nullptr, 0, 0) == -1);
         EXPECT_TRUE( tperrno == TPEINVAL);
      }

      TEST( casual_xatmi_conversation, connect__TPSENDONLY_and_TPRECVONLY___expect_TPEINVAL)
      {
         unittest::Trace trace;

         auto domain = local::domain();

         EXPECT_TRUE( tpconnect( "casual/example/conversation", nullptr, 0, TPSENDONLY | TPRECVONLY) == -1);
         EXPECT_TRUE( tperrno == TPEINVAL);
      }

      TEST( casual_xatmi_conversation, connect__TPSENDONLY__absent_service___expect_TPENOENT)
      {
         unittest::Trace trace;

         auto domain = local::domain();

         EXPECT_TRUE( tpconnect( "absent", nullptr, 0, TPSENDONLY) == -1);
         EXPECT_TRUE( tperrno == TPENOENT);
      }

      TEST( casual_xatmi_conversation, connect__TPSENDONLY__echo_service)
      {
         unittest::Trace trace;

         auto domain = local::domain();

         auto descriptor = tpconnect( "casual/example/conversation", nullptr, 0, TPSENDONLY);
         EXPECT_TRUE( descriptor != -1);
         EXPECT_TRUE( tperrno == 0);

         EXPECT_TRUE( tpdiscon( descriptor) != -1);
      }

/*
      TEST( casual_xatmi_conversation, connect__TPSENDONLY__service_rollback__tpsend___expect_TPEV_DISCONIMM_TPEV_SVCFAIL_events)
      {
         unittest::Trace trace;

         auto domain = local::domain();

         auto descriptor = tpconnect( "casual/example/rollback", nullptr, 0, TPSENDONLY);
         EXPECT_TRUE( descriptor != -1);
         EXPECT_TRUE( tperrno == 0);

         {
            long event = 0;

            EXPECT_TRUE( tpsend( descriptor, nullptr, 0, 0, &event) == -1);
            EXPECT_TRUE( tperrno == TPEEVENT);
            EXPECT_TRUE( event == ( TPEV_SVCFAIL | TPEV_DISCONIMM));
         }

         // descriptor should be unreserved
         EXPECT_TRUE( tpdiscon( descriptor) == -1);
      }

      TEST( casual_xatmi_conversation, connect__TPSENDONLY__service_error_system__tpsend___expect_TPEV_DISCONIMM__TPEV_SVCERR_events)
      {
         unittest::Trace trace;

         auto domain = local::domain();

         auto descriptor = tpconnect( "casual/example/error/system", nullptr, 0, TPSENDONLY);
         EXPECT_TRUE( descriptor != -1);
         EXPECT_TRUE( tperrno == 0);

         {
            long event = 0;

            EXPECT_TRUE( tpsend( descriptor, nullptr, 0, 0, &event) == -1);
            EXPECT_TRUE( tperrno == TPEEVENT);
            EXPECT_TRUE( event == ( TPEV_SVCERR | TPEV_DISCONIMM));
         }

         // descriptor should be unreserved
         EXPECT_TRUE( tpdiscon( descriptor) == -1);
      }

      TEST( casual_xatmi_conversation, connect__TPSENDONLY__service_SUCCESS__tpsend___expect_TPEV_DISCONIMM___events)
      {
         unittest::Trace trace;

         auto domain = local::domain();

         auto descriptor = tpconnect( "casual/example/echo", nullptr, 0, TPSENDONLY);
         EXPECT_TRUE( descriptor != -1);
         EXPECT_TRUE( tperrno == 0);

         {
            long event = 0;

            EXPECT_TRUE( tpsend( descriptor, nullptr, 0, 0, &event) == -1);
            EXPECT_TRUE( tperrno == TPEEVENT);
            EXPECT_TRUE( event & TPEV_DISCONIMM) << "event: " << event;
         }

         // descriptor should be unreserved
         EXPECT_TRUE( tpdiscon( descriptor) == -1);
      }


      TEST( casual_xatmi_conversation, connect__TPRECVONLY__echo_service)
      {
         unittest::Trace trace;

         auto domain = local::domain();

         auto buffer = tpalloc( X_OCTET, nullptr, 128);
         auto len = tptypes( buffer, nullptr, nullptr);

         unittest::random::range( range::make( buffer, buffer + len));

         auto descriptor = tpconnect( "casual/example/echo", buffer, len, TPRECVONLY);
         EXPECT_TRUE( descriptor != -1);
         EXPECT_TRUE( tperrno == 0);

         // receive
         {
            auto reply = tpalloc( X_OCTET, nullptr, 128);

            long event = 0;
            EXPECT_TRUE( tprecv( descriptor, &reply, &len, 0, &event) == -1);
            EXPECT_TRUE( tperrno == TPEEVENT);
            EXPECT_TRUE( event & TPEV_SENDONLY);

            EXPECT_TRUE( algorithm::equal( range::make( reply, 128), range::make( buffer, 128)));

            tpfree( reply);

         }
         tpfree( buffer);

         EXPECT_TRUE( tpdiscon( descriptor) != -1);
      }

*/

   } // xatmi
} // casual

//!
//! casual 
//!

#include "common/unittest.h"


#include "common/mockup/domain.h"

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

            //
            // Represent a domain
            //
            struct Domain
            {
               Domain()
                  : server{ {
                     mockup::domain::echo::create::service( "echo"),

                     /*
                      * Will echo with error set to the following
                        #define TPEOS 7
                        #define TPEPROTO 9
                        #define TPESVCERR 10
                        #define TPESVCFAIL 11
                        #define TPESYSTEM 12
                      */
                     mockup::domain::echo::create::service( "service_TPESVCERR"),
                     mockup::domain::echo::create::service( "service_TPESVCFAIL"),
                     mockup::domain::echo::create::service( "service_SUCCESS"),
                  }}
               {

               }

               mockup::domain::Manager manager;
               mockup::domain::Broker broker;
               mockup::domain::transaction::Manager tm;
               mockup::domain::echo::Server server;
            };

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

         local::Domain domain;

         EXPECT_TRUE( tpconnect( "echo", nullptr, 0, 0) == -1);
         EXPECT_TRUE( tperrno == TPEINVAL);
      }

      TEST( casual_xatmi_conversation, connect__TPSENDONLY_and_TPRECVONLY___expect_TPEINVAL)
      {
         unittest::Trace trace;

         local::Domain domain;

         EXPECT_TRUE( tpconnect( "echo", nullptr, 0, TPSENDONLY | TPRECVONLY) == -1);
         EXPECT_TRUE( tperrno == TPEINVAL);
      }

      TEST( casual_xatmi_conversation, connect__TPSENDONLY__absent_service___expect_TPENOENT)
      {
         unittest::Trace trace;

         local::Domain domain;

         EXPECT_TRUE( tpconnect( "absent", nullptr, 0, TPSENDONLY) == -1);
         EXPECT_TRUE( tperrno == TPENOENT);
      }

      TEST( casual_xatmi_conversation, connect__TPSENDONLY__echo_service)
      {
         unittest::Trace trace;

         local::Domain domain;

         auto descriptor = tpconnect( "echo", nullptr, 0, TPSENDONLY);
         EXPECT_TRUE( descriptor != -1);
         EXPECT_TRUE( tperrno == 0);

         EXPECT_TRUE( tpdiscon( descriptor) != -1);
      }

      TEST( casual_xatmi_conversation, connect__TPSENDONLY__service_TPESVCFAIL__tpsend___expect_TPEV_DISCONIMM_TPEV_SVCFAIL_events)
      {
         unittest::Trace trace;

         local::Domain domain;

         auto descriptor = tpconnect( "service_TPESVCFAIL", nullptr, 0, TPSENDONLY);
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

      TEST( casual_xatmi_conversation, connect__TPSENDONLY__service_TPESVCERR__tpsend___expect_TPEV_DISCONIMM__TPEV_SVCERR_events)
      {
         unittest::Trace trace;

         local::Domain domain;

         auto descriptor = tpconnect( "service_TPESVCERR", nullptr, 0, TPSENDONLY);
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

         local::Domain domain;

         auto descriptor = tpconnect( "service_SUCCESS", nullptr, 0, TPSENDONLY);
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


   } // xatmi
} // casual

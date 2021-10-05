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
#include "tx.h"

// debugging/workaround for hang on domain shutdown
#include "common/process.h"
#include "common/chronology.h"
// include code to sleep a short time at end of testcase 
// before shutting down domain
# define HANG_WORKAROUND 1
// debugging end

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
#if 1
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
      - path: ${CASUAL_MAKE_SOURCE_ROOT}/middleware/service/bin/casual-service-manager
        memberships: [ base]
      - path: ${CASUAL_MAKE_SOURCE_ROOT}/middleware/transaction/bin/casual-transaction-manager
        memberships: [ transaction]
      - path: ${CASUAL_MAKE_SOURCE_ROOT}/middleware/example/server/bin/casual-example-error-server
        memberships: [ example]
      - path: ${CASUAL_MAKE_SOURCE_ROOT}/middleware/example/server/bin/casual-example-server
        memberships: [ example]
)"
#else
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
      - path: ${CASUAL_MAKE_SOURCE_ROOT}/middleware/service/bin/casual-service-manager
        memberships: [ base]
      - path: ${CASUAL_MAKE_SOURCE_ROOT}/middleware/transaction/bin/casual-transaction-manager
        memberships: [ transaction]
      - path: ${CASUAL_MAKE_SOURCE_ROOT}/middleware/queue/bin/casual-queue-manager
        memberships: [ queue]
      - path: ${CASUAL_MAKE_SOURCE_ROOT}/middleware/example/server/bin/casual-example-error-server
        memberships: [ example]
      - path: ${CASUAL_MAKE_SOURCE_ROOT}/middleware/example/server/bin/casual-example-server
        memberships: [ example]
)"
#endif
               }};
            }
            // Helper for doing a tpsend when expected to succeed
            // string for data to send, allocates and frees X_OCTET buffer
            // tperrno from tpsend returned in tpsend_tperrno in case it is needed
            // by caller.
            int th_tpsend(int cd, const std::string send_data, long flags, long* revent, int* tpsend_tperrno )
            {
               auto send_buffer = tpalloc( X_OCTET, nullptr, send_data.length());
               algorithm::copy(range::make( send_data.begin(), send_data.end()), send_buffer);
               auto send_status=tpsend(cd, send_buffer, send_data.length(),
                                       flags, revent);
               EXPECT_TRUE(send_status != -1) << "send_status: " << send_status
                                             << " tperrno: " << tperrno
                                             << " event: " << *revent;
               EXPECT_TRUE(tperrno == 0);
               // We do not check send_event. Value is only defined 
               // when tpsend failed return value -1 and tperrno=TPEEVENT
               // and that is not expected.

               // we can't save and restore tperrno, so we return the tpsend
               // tperrno in an out argument before destroying it...
               if (tpsend_tperrno) {
                  *tpsend_tperrno=tperrno;
               }
               tpfree(send_buffer);
               return send_status;
            }

            int th_tprecv_event(int cd,
                                std::string& reply_string, long* len,
                                long flags,
                                long* event, int* tprecv_tperrno )
            {
               auto reply = tpalloc( X_OCTET, nullptr, *len);
               *len = tptypes(reply, nullptr, nullptr);
               int tprecv_status = tprecv( cd, &reply, len, 0, event);
               EXPECT_TRUE( tprecv_status == -1);
               EXPECT_TRUE( tperrno == TPEEVENT);
               if (tprecv_status != -1 ||
                   (tperrno == TPEEVENT &&
                     (*event == TPEV_SVCFAIL ||
                      *event == TPEV_SVCSUCC ||
                      *event == TPEV_SENDONLY
                     )
                   )
                  )
               {
                  reply_string.assign(reply, static_cast<std::string::size_type>(*len));
               }
               if (tprecv_tperrno) {
                  *tprecv_tperrno = tperrno;
               }
               tpfree(reply);
               return tprecv_status;
            }

            // A helper for the common case that we expect a tprecv to get
            // the end of service response generated by tpreturn.
            // 
            void th_tprecv_expect_TPEV_SVCSUCC(int cd,
                                long flags,
                                const std::string& expected_reply,
                                long expected_tpurcode)
            {
               long event = 0;
               std::string replystring;
               long len=128;
               int tprecv_tperrno;
               local::th_tprecv_event(cd, replystring, &len, flags, &event, &tprecv_tperrno);
               EXPECT_TRUE( event == TPEV_SVCSUCC) << "event: " << event;
               EXPECT_TRUE( tpurcode == expected_tpurcode)
                  << "got tpurcode: " << tpurcode << " expected: " << expected_tpurcode;
               EXPECT_TRUE( replystring == expected_reply)
                  << "got: " << replystring << " expected: " << expected_reply;
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

      TEST( casual_xatmi_conversation, DISABLED_connect__invalid_flag_TPTRAN__expect_TPEINVAL)
      {
         unittest::Trace trace;

         auto domain = local::domain();

         //std::cout << "attach debugger" << std::flush;
         //casual::platform::time::unit sleep_debug{60s};
         //common::process::sleep(sleep_debug);

         // TPTRAN is a flag that is input(!) to a called service
         // Not allowed in tpconnect.
         EXPECT_TRUE( tpconnect( "casual/example/conversation", nullptr, 0, TPTRAN) == -1);
         EXPECT_TRUE( tperrno == TPEINVAL) << "tperrno: " << tperrnostring(tperrno);
      }

      TEST( casual_xatmi_conversation, DISABLED_connect__invalid_flag_TPCONV__expect_TPEINVAL)
      {
         unittest::Trace trace;

         auto domain = local::domain();

//         std::cout << "attach debugger" << std::flush;
//         casual::platform::time::unit sleep_debug{60s};
//         common::process::sleep(sleep_debug);

         // TPCONV is a flag that is input(!) to a called service, set for conversational.
         // Not alloed in tpconnect.
         EXPECT_TRUE( tpconnect( "casual/example/conversation", nullptr, 0, TPCONV) == -1);
         EXPECT_TRUE( tperrno == TPEINVAL) << "tperrno: " << tperrnostring(tperrno);
      }

      TEST( casual_xatmi_conversation, connect__TPSENDONLY__absent_service___expect_TPENOENT)
      {
         unittest::Trace trace;

         auto domain = local::domain();

         EXPECT_TRUE( tpconnect( "absent", nullptr, 0, TPSENDONLY) == -1);
         EXPECT_TRUE( tperrno == TPENOENT);
      }

      TEST( casual_xatmi_conversation, connect__TPSENDONLY__conversation_service)
      {
         unittest::Trace trace;

         auto domain = local::domain();

         auto descriptor = tpconnect( "casual/example/conversation", nullptr, 0, TPSENDONLY);
         EXPECT_TRUE( descriptor != -1);
         EXPECT_TRUE( tperrno == 0);

         EXPECT_TRUE( tpdiscon( descriptor) != -1);
#if HANG_WORKAROUND
         casual::platform::time::unit sleep_before_shutdown{1s};
         common::process::sleep(sleep_before_shutdown);
#endif
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
*/

      TEST( casual_xatmi_conversation, connect__TPRECVONLY__echo_service)
      {
         unittest::Trace trace;

         auto domain = local::domain();

         auto buffer = tpalloc( X_OCTET, nullptr, 128);
         auto len = tptypes( buffer, nullptr, nullptr);

         unittest::random::range( range::make( buffer, buffer + len));

         // This calls the echo service that in reality is a request/response
         // type service. It echoes the input data with a tpreturn. 
         // It "works" to call this type of service with tpconnect, but
         // the tpreturn will cause the conversation to "terminate". This
         // will result in a event TPEV_SVCSUCC on the receive (if the service
         // terminates with success, otherwise TPEV_SVCFAIL)
         // After this event the descriptor returned by connect is no longer
         // valid, so a tpdiscon() is expected to fail.
         auto descriptor = tpconnect( "casual/example/echo", buffer, len, TPRECVONLY);
         EXPECT_TRUE( descriptor != -1);
         EXPECT_TRUE( tperrno == 0)  << "tperrno: " <<  tperrnostring(tperrno);

         // receive
         {
            auto reply = tpalloc( X_OCTET, nullptr, 128);

            long event = 0;
            EXPECT_TRUE( tprecv( descriptor, &reply, &len, 0, &event) == -1);
            EXPECT_TRUE( tperrno == TPEEVENT)  << "tperrno: " <<  tperrnostring(tperrno);
            EXPECT_TRUE( event == TPEV_SVCSUCC) << "event:" << event;
            EXPECT_TRUE( tpurcode == 0) << "tpurcode: " << tpurcode;

            EXPECT_TRUE( algorithm::equal( range::make( reply, 128), range::make( buffer, 128)));

            tpfree( reply);

         }
         tpfree( buffer);

         // Conversation terminated so tpdiscon(descriptor) is expected to fail
         EXPECT_TRUE( tpdiscon( descriptor) == -1);
#if HANG_WORKAROUND
         // Workaround:
         casual::platform::time::unit sleep_before_shutdown{1s};
         common::process::sleep(sleep_before_shutdown);
#endif
      }

      TEST( casual_xatmi_conversation, connect__TPRECVONLY__conversation2_auto_service)
      {
         unittest::Trace trace;

         auto domain = local::domain();

         // This calls the conversation2_auto service. This test:
         // * has no started transaction
         // * send no data in the connect
         // * connnects with TPRECVONLY (i.e. hands over control to service)
         // * issues a tprecv
         // * expects this tprecv to return with an event
         //   indicating service completion with success
         // * expects the data to contain the flags set when the service was invoked.
         //   In this test TPCONV|TPSENDONLY|TPTRAN (=0x00000c10).
         //
         // "conversation2_auto" has the same code as "conversation2" but is
         // defined with "transaction: auto", so service should have an active
         // transaction without tx_begin() here. 
         auto descriptor = tpconnect( "casual/example/conversation2_auto", nullptr, 0, TPRECVONLY);
         EXPECT_TRUE( descriptor != -1);
         EXPECT_TRUE( tperrno == 0);

         // This receive is expected to get the result of a tpreturn
         local::th_tprecv_expect_TPEV_SVCSUCC(descriptor, TPSIGRSTRT, "Flags: 0x00000c10", 0);

         // conversation terminated so tpdiscon(descriptor) is expected to fail
         EXPECT_TRUE( tpdiscon( descriptor) == -1);
         // workaround for hang on shutdown:
#if HANG_WORKAROUND
         casual::platform::time::unit sleep_before_shutdown{1s};
         common::process::sleep(sleep_before_shutdown);
#endif
      }

      TEST( casual_xatmi_conversation, connect__TPRECVONLY__conversation2_service)
      {
         unittest::Trace trace;

         auto domain = local::domain();

         // This calls the conversation2 service. This test:
         // * has no started transaction
         // * send no data in the connect
         // * connnects with TPRECVONLY (i.e. hands over control to service)
         // * issues a tprecv
         // * expects this tprecv to return with an event
         //   indicating service completion with success
         // * expects the data to contain the flags set when the service was invoked.
         //   In this test TPCONV|TPSENDONLY (=0x00000c00).
         //
         // "conversation2_auto" has the same code as "conversation2" but is
         // defined with "transaction: join", so service will NOT have an active
         // transaction as we do not start one before connecting.
         auto descriptor = tpconnect( "casual/example/conversation2", nullptr, 0, TPRECVONLY);
         EXPECT_TRUE( descriptor != -1);
         EXPECT_TRUE( tperrno == 0);

         // receive
         // This receive is expected to get the result of a tpreturn
         local::th_tprecv_expect_TPEV_SVCSUCC(descriptor, TPSIGRSTRT, "Flags: 0x00000c00", 0);

         // conversation terminated so tpdiscon(descriptor) is expected to fail
         EXPECT_TRUE( tpdiscon( descriptor) == -1);
         // workaround for hang on shutdown:
#if HANG_WORKAROUND
         casual::platform::time::unit sleep_before_shutdown{1s};
         common::process::sleep(sleep_before_shutdown);
#endif
      }

      TEST( casual_xatmi_conversation, connect__TX_TPRECVONLY__conversational2_service)
      {
         unittest::Trace trace;

         auto domain = local::domain();

         // This calls the conversational2 service. This test:
         // * has a started transaction
         // * send no data in the connect
         // * connnects with TPRECVONLY (i.e. hands over control to service)
         // * issues a tprecv
         // * expects this tprecv to return with an event
         //   indicating service completion with success
         // * expects the data to contain the flags set when the service was
         //   invoked. In this case TPVONV|TPSENDONLY|TPTRAN (=0x00000c10)
         //   The service has "transaction: join" 
         //
         // "conversation2" terminates the service with tpreturn and a response
         // with the invocationa flags when it gets control of the conversation.
         // (the service will also echo all data it receives before
         //  getting control with a tpsend before the tpreturn. In this test there
         // is no data in the connect and therefore no tpsend is done by the service)
         EXPECT_TRUE(tx_begin() == TX_OK);
         auto descriptor = tpconnect( "casual/example/conversation2", nullptr, 0, TPRECVONLY);
         EXPECT_TRUE( descriptor != -1);
         EXPECT_TRUE( tperrno == 0) << "tperrno:" << tperrnostring(tperrno);

         // This receive is expected to get the result of a tpreturn
         local::th_tprecv_expect_TPEV_SVCSUCC(descriptor, TPSIGRSTRT, "Flags: 0x00000c10", 0);

         EXPECT_TRUE(tx_rollback() == TX_OK);

         EXPECT_TRUE( tpdiscon( descriptor) == -1);
#if HANG_WORKAROUND
         casual::platform::time::unit sleep_before_shutdown{1s};
         common::process::sleep(sleep_before_shutdown);
#endif
      }

      TEST( casual_xatmi_conversation, connect__TX_TPRECVONLY_TPNOTRAN__conversational2_service)
      {
         unittest::Trace trace;

         auto domain = local::domain();

         // This calls the conversational2 service. This test:
         // * has a started transaction
         // * send no data in the connect
         // * connnects with TPRECVONLY (i.e. hands over control to service)
         //   and TPNOTRAN. So transaction is not forwarded to service.
         //   service has "transaction: join" so should be invoked without
         //   TPTRAN flag.
         // * issues a tprecv
         // * expects this tprecv to return with an event
         //   indicating service completion with success
         // * expects the data to contain the flags set when the service was invoked
         //   In thuis case TPCONV|TPSENDONLY (TPTRAN not set)
         EXPECT_TRUE(tx_begin() == TX_OK);
         auto descriptor = tpconnect( "casual/example/conversation2", nullptr, 0, TPRECVONLY|TPNOTRAN);
         EXPECT_TRUE( descriptor != -1);
         EXPECT_TRUE( tperrno == 0);

         // This receive is expected to get the result of a tpreturn
         local::th_tprecv_expect_TPEV_SVCSUCC(descriptor, TPSIGRSTRT, "Flags: 0x00000c00", 0);

         EXPECT_TRUE(tx_rollback() == TX_OK);

         EXPECT_TRUE( tpdiscon( descriptor) == -1);
#if HANG_WORKAROUND
         casual::platform::time::unit sleep_before_shutdown{1s};
         common::process::sleep(sleep_before_shutdown);
#endif
      }

      TEST( casual_xatmi_conversation, connect_with_data__TX_TPRECVONLY__conversational2_service)
      {
         unittest::Trace trace;

         auto domain = local::domain();

         // This calls the conversational2 service. This test:
         // * has a started transaction
         // * send data in the connect
         // * connnects with TPRECVONLY (i.e. transfers control of conversation).
         //   service has "transaction: join" so should be invoked with
         //   TPTRAN flag.
         // * issues a tprecv, expects data in connect back.
         // * issues a tprecv.
         //   Expects this tprecv to return with an event
         //   indicating service completion with success
         // * expects the data to contain the flags passed when the service
         //   was invoked. In this case TPCONV|TPSENDONLY|TPTRAN (==0xc10).
         //
         std::string connect_string {"data in connect"};
         auto buffer = tpalloc( X_OCTET, nullptr, connect_string.length());
         auto len = tptypes( buffer, nullptr, nullptr);
         algorithm::copy(range::make(connect_string.begin(), connect_string.end()), buffer);

         EXPECT_TRUE(tx_begin() == TX_OK);

         auto descriptor = tpconnect( "casual/example/conversation2", buffer, len, TPRECVONLY);
         EXPECT_TRUE( descriptor != -1) << "tpconnect error, tperrno: "
                                        << tperrnostring(tperrno);
         EXPECT_TRUE( tperrno == 0) << "tperrno: " <<  tperrnostring(tperrno);
         // is tperrno value defined if tpconnect returned != -1? 

         // As we connected with data and TPRECVONLY the service is expected
         // to send the data back with a tpsend() (followed by a tpreturn()).
         // Service calls tpsend with TPSENDONLY so it keeps control.
         {
            auto reply = tpalloc( X_OCTET, nullptr, 128);
            long event = 0;
            auto len = tptypes(reply, nullptr, nullptr);
            EXPECT_TRUE( tprecv( descriptor, &reply, &len, 0, &event) != -1);
            // it is not really defined what tperrno is set to when tprecv
            // returns != -1, but it is most likely set to 0 by casual.
            EXPECT_TRUE( tperrno == 0) << "tperrno: " <<  tperrnostring(tperrno);
            EXPECT_TRUE( reply != nullptr) << "A reply was expected";
          
            std::string reply_string(reply, static_cast<std::string::size_type>(len));
            EXPECT_TRUE( reply_string == connect_string) << "Got: [" << reply_string 
                                                    << "] expected: [" << connect_string << "]"
                                                    << std::endl;
            tpfree( reply);
         }
         // This receive is expected to get the result from the tpreturn()
         local::th_tprecv_expect_TPEV_SVCSUCC(descriptor, TPSIGRSTRT, "Flags: 0x00000c10", 0);

         EXPECT_TRUE(tx_rollback() == TX_OK);

         EXPECT_TRUE( tpdiscon( descriptor) == -1);
#if HANG_WORKAROUND
         casual::platform::time::unit sleep_before_shutdown{1s};
         common::process::sleep(sleep_before_shutdown);
#endif
      }

      TEST( casual_xatmi_conversation, connect_tpsend__TX_TPRECVONLY__conversational2_service)
      {
         unittest::Trace trace;

         auto domain = local::domain();

         // This calls the conversational2 service. This test:
         // * has a started transaction
         // * send data in the connect
         // * connnects with TPSENDONLY (i.e. keeps control of conversation).
         //   service has "transaction: join" so should be invoked with
         //   TPTRAN flag.
         // * sends data with tpsend and with TPRECVONLY flag (transfers
         //   control of conversation)). 
         // * issues a tprecv, expects data sent in connect and tpsend back.
         // * issues a tprecv.
         //   Expects this tprecv to return with an event
         //   indicating service completion with success
         // * expects the data to contain the flags passed when the service
         //   was invoked. In this case TPCONV|TPRECVONLY|TPTRAN (==0x1410).
         //
         std::string connect_string {"data in connect. "};
         auto buffer = tpalloc( X_OCTET, nullptr, connect_string.length());
         auto len = tptypes( buffer, nullptr, nullptr);

         algorithm::copy(range::make(connect_string.begin(), connect_string.end()), buffer); // no terminating 0

         EXPECT_TRUE(tx_begin() == TX_OK);
         auto descriptor = tpconnect( "casual/example/conversation2", buffer, len, TPSENDONLY);
         EXPECT_TRUE( descriptor != -1) << "tpconnect error, tperrno: " << tperrno;
         EXPECT_TRUE( tperrno == 0); // defined if tpconnect returned != -1?

         std::string sent_data = connect_string;

         // send
         {
            long send_event=0;
            int send_tperrno;
            std::string send_data {"Data with tpsend"};

            local::th_tpsend(descriptor, send_data, TPRECVONLY, &send_event, &send_tperrno);

            sent_data += send_data;
         }
         // As we have sent data with connect and tpsend, service is expected
         // to send the data back with a tpsend() (followed by a tpreturn()).
         {
            auto reply = tpalloc( X_OCTET, nullptr, 128);
            long event = 0;
            auto len = tptypes(reply, nullptr, nullptr);
            EXPECT_TRUE( tprecv( descriptor, &reply, &len, 0, &event) != -1);
            // it is not really defined what tperrno is set to when tprecv
            // returns != -1, but is most likely set to 0.
            EXPECT_TRUE( tperrno == 0);
            EXPECT_TRUE( reply != nullptr) << "A reply was expected";
          
            std::string reply_string(reply, static_cast<std::string::size_type>(len));
            EXPECT_TRUE( reply_string == sent_data) << "Got: [" << reply_string 
                                                    << "] expected: [" << sent_data << "]"
                                                    << std::endl;
            tpfree( reply);
         }
         // This receive is expected to get the result from the tpreturn()
         local::th_tprecv_expect_TPEV_SVCSUCC(descriptor, TPSIGRSTRT, "Flags: 0x00001410", 0);

         EXPECT_TRUE(tx_rollback() == TX_OK);

         EXPECT_TRUE( tpdiscon( descriptor) == -1);
#if HANG_WORKAROUND
         casual::platform::time::unit sleep_before_shutdown{1s};
         common::process::sleep(sleep_before_shutdown);
#endif
      }

      TEST( casual_xatmi_conversation, connect_tpsend_tpsend__TX_TPRECVONLY__conversational2_service)
      {
         unittest::Trace trace;

         auto domain = local::domain();

         // This calls the conversational2 service. This test:
         // * has a started transaction
         // * send data in the connect
         // * connnects with TPSENDONLY (i.e. keeps control of conversation).
         //   service has "transaction: join" so should be invoked with
         //   TPTRAN flag.
         // * sends data with tpsend and TPSENDONLY
         // * sends data with tpsend and TPRECVONLY flag (transfers
         //   control of conversation)). 
         // * issues a tprecv, expects data sent in connect and tpsend back.
         // * issues a tprecv.
         //   Expects this tprecv to return with an event
         //   indicating service completion with success
         // * expects the data to contain the flags passed when the service
         //   was invoked. In this case TPCONV|TPRECVONLY|TPTRAN (==0x1410).
         //
         std::string connect_string {"data in connect. "};
         auto buffer = tpalloc( X_OCTET, nullptr, connect_string.length());
         auto len = tptypes( buffer, nullptr, nullptr);

         algorithm::copy(range::make(connect_string.begin(), connect_string.end()), buffer); // no terminating 0

         EXPECT_TRUE(tx_begin() == TX_OK);
         auto descriptor = tpconnect( "casual/example/conversation2", buffer, len, TPSENDONLY);
         EXPECT_TRUE( descriptor != -1) << "tpconnect error, tperrno: " << tperrno;
         EXPECT_TRUE( tperrno == 0); // defined if tpconnect returned != -1?

         std::string sent_data = connect_string;

         // first send
         {
            long send_event=0;
            int send_tperrno;
            std::string send_data {"Data with tpsend"};

            local::th_tpsend(descriptor, send_data, TPSIGRSTRT, &send_event, &send_tperrno);

            sent_data += send_data;
         }
         // second send, transfer control
         {
            long send_event=0;
            int send_tperrno;
            std::string send_data {"Data2 with tpsend"};

            local::th_tpsend(descriptor, send_data, TPSIGRSTRT|TPRECVONLY, &send_event, &send_tperrno);

            sent_data += send_data;
         }
         // As we have sent data with connect and tpsend, service is expected
         // to send the data back with a tpsend() (followed by a tpreturn()).
         {
            auto reply = tpalloc( X_OCTET, nullptr, 128);
            long event = 0;
            auto len = tptypes(reply, nullptr, nullptr);
            EXPECT_TRUE( tprecv( descriptor, &reply, &len, 0, &event) != -1);
            // it is not really defined what tperrno is set to when tprecv
            // returns != -1, but is most likely set to 0 by casual.
            EXPECT_TRUE( tperrno == 0);
            EXPECT_TRUE( reply != nullptr && len > 0) << "A reply was expected, len: " << len;
          
            std::string reply_string(reply, static_cast<std::string::size_type>(len));
            EXPECT_TRUE( reply_string == sent_data) << "Got: [" << reply_string 
                                                    << "] expected: [" << sent_data << "]"
                                                    << std::endl;
            tpfree( reply);
         }
         // This receive is expected to get the result from the tpreturn()
         local::th_tprecv_expect_TPEV_SVCSUCC(descriptor, TPSIGRSTRT, "Flags: 0x00001410", 0);

         EXPECT_TRUE(tx_rollback() == TX_OK);

         EXPECT_TRUE( tpdiscon( descriptor) == -1);
#if HANG_WORKAROUND
         casual::platform::time::unit sleep_before_shutdown{1s};
         common::process::sleep(sleep_before_shutdown);
#endif
      }

      namespace local 
      {
         namespace
         {
            void one_connect_tpsend__TX_TPRECVONLY__conversational2()
            {
               // helper that executes a conversational session that:
               // calls the conversational2 service. 
               // * starts a transaction
               // * send data in the connect
               // * connnects with TPSENDONLY (i.e. keeps control of conversation).
               //   service has "transaction: join" so should be invoked with
               //   TPTRAN flag.
               // * sends data with tpsend and with TPRECVONLY flag (transfers
               //   control of conversation)). 
               // * issues a tprecv, expects data sent in connect and tpsend back.
               // * issues a tprecv.
               //   Expects this tprecv to return with an event
               //   indicating service completion with success
               // * expects the data to contain the flags passed when the service
               //   was invoked. In this case TPCONV|TPRECVONLY|TPTRAN (==0x1410).
               //
               // Used in test cases that do this once or multiple times against
               // the same domain  
               std::string connect_string {"data in connect. "};
               auto buffer = tpalloc( X_OCTET, nullptr, connect_string.length());
               auto len = tptypes( buffer, nullptr, nullptr);

               algorithm::copy(range::make(connect_string.begin(), connect_string.end()), buffer); // no terminating 0

               EXPECT_TRUE(tx_begin() == TX_OK);
               auto descriptor = tpconnect( "casual/example/conversation2", buffer, len, TPSENDONLY);
               EXPECT_TRUE( descriptor != -1) << "tpconnect error, tperrno: " << tperrno;
               EXPECT_TRUE( tperrno == 0); // defined if tpconnect returned != -1?

               std::string sent_data = connect_string;

               // send
               {
                  long send_event=0;
                  int send_tperrno;
                  std::string send_data {"Data with tpsend"};
                  local::th_tpsend(descriptor, send_data, TPRECVONLY, &send_event, &send_tperrno);
                  sent_data += send_data;
               }
               // As we have sent data with connect and tpsend, service is expected
               // to send the data back with a tpsend() (followed by a tpreturn()).
               {
                  auto reply = tpalloc( X_OCTET, nullptr, 128);
                  long event = 0;
                  auto len = tptypes(reply, nullptr, nullptr);
                  EXPECT_TRUE( tprecv( descriptor, &reply, &len, 0, &event) != -1);
                  // it is not really defined what tperrno is set to when tprecv
                  // returns != -1, but is most likely set to 0.
                  EXPECT_TRUE( tperrno == 0);
                  EXPECT_TRUE( reply != nullptr) << "A reply was expected";
               
                  std::string reply_string(reply, static_cast<std::string::size_type>(len));
                  EXPECT_TRUE( reply_string == sent_data) << "Got: [" << reply_string 
                                                         << "] expected: [" << sent_data << "]"
                                                         << std::endl;
                  tpfree( reply);
               }
               // This receive is expected to get the result from the tpreturn()
               // This receive is expected to get the result of a tpreturn
               local::th_tprecv_expect_TPEV_SVCSUCC(descriptor, TPSIGRSTRT, "Flags: 0x00001410", 0);

               EXPECT_TRUE(tx_rollback() == TX_OK);

               EXPECT_TRUE( tpdiscon( descriptor) == -1);

            }
         }
      }

      TEST( casual_xatmi_conversation, do_10_connect_tpsend__TX_TPRECVONLY__conversational2_service)
      {
         unittest::Trace trace;

         auto domain = local::domain();
         for (int i=0; i<10; i++)
         {
            local::one_connect_tpsend__TX_TPRECVONLY__conversational2();
         }
#if HANG_WORKAROUND
         casual::platform::time::unit sleep_before_shutdown{1s};
         common::process::sleep(sleep_before_shutdown);
#endif
      }
/* */

   } // xatmi
} // casual

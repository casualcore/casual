//! 
//! Copyright ( c) 2015, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!

// to be able to use 'raw' flags and codes
// since we undefine 'all' of them in common
#define CASUAL_NO_XATMI_UNDEFINE


#include "common/unittest.h"

#include "domain/manager/unittest/process.h"
#include "test/unittest/xatmi/buffer.h"
#include "test/unittest/gateway.h"

#include "common/string/view.h"
#include "common/algorithm/compare.h"

#include "xatmi.h"
#include "tx.h"

// debugging/workaround for hang on domain shutdown
#include "common/process.h"
// define to include code to sleep a short time at end of testcase 
// before shutting down domain. Problem does not occur in all
// environments! Something depends on timing.
//# define HANG_WORKAROUND 1
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
            namespace configuration
            {
               constexpr auto base = R"(
domain: 
   name: base

   groups: 
      -  name: base
      -  name: user
         dependencies: [ base]
      -  name: gateway
         dependencies: [ user]
   
   servers:
      -  path: "${CASUAL_MAKE_SOURCE_ROOT}/middleware/service/bin/casual-service-manager"
         memberships: [ base]
      -  path: "${CASUAL_MAKE_SOURCE_ROOT}/middleware/transaction/bin/casual-transaction-manager"
         memberships: [ base]
      -  path: "${CASUAL_MAKE_SOURCE_ROOT}/middleware/gateway/bin/casual-gateway-manager"
         memberships: [ gateway]
)";

               constexpr auto example =  R"(
domain:
   name: conversation
   servers:
      -  path: ${CASUAL_MAKE_SOURCE_ROOT}/middleware/example/server/bin/casual-example-error-server
         memberships: [ user]
      -  path: ${CASUAL_MAKE_SOURCE_ROOT}/middleware/example/server/bin/casual-example-server
         memberships: [ user]
)";

            } // configuration
            
            template< typename... Ts>
            auto domain( Ts&&... ts)
            {
               return casual::domain::manager::unittest::process( configuration::base, std::forward< Ts>( ts)...);
            }

            auto domain()
            {
               return domain( configuration::example);
            }


            namespace send
            {
               struct Result
               {
                  long event{};
                  int retval{};
                  int error{};
                  long user{};

                  explicit operator bool() const noexcept { return retval != -1 && event == 0 && error == 0;}

                  CASUAL_LOG_SERIALIZE(
                     CASUAL_SERIALIZE( event);
                     CASUAL_SERIALIZE( retval);
                     CASUAL_SERIALIZE( error);
                     CASUAL_SERIALIZE( user);
                  )
               };

               //! Helper for doing a tpsend when expected to succeed
               //! string for data to send.
               //! @returns `send::Result` with event state and errno state
               template< typename T>
               Result invoke( int descriptor, T&& data, long flags)
               {
                  Result result;

                  auto buffer = test::unittest::xatmi::buffer::x_octet{ data.size()};
                  algorithm::copy( data, buffer);

                  // We do not check send_event. Value is only defined 
                  // when tpsend failed return value -1 and tperrno=TPEEVENT
                  // and that is not expected.
                  result.retval = tpsend( descriptor, buffer.data, buffer.size, flags, &result.event);
                  result.error = tperrno;
                  result.user = tpurcode;

                  return result;
               }
               
            } // send

            namespace receive
            {
               struct Result : send::Result 
               {
                  std::string payload;

                  CASUAL_LOG_SERIALIZE(
                     send::Result::serialize( archive);
                     CASUAL_SERIALIZE( payload);
                  )
               };

               //! Helper for  a tprecv 
               //! @returns `send::Result` with event state and errno state
               Result invoke( int descriptor, long flags)
               {
                  Result result;

                  auto buffer = test::unittest::xatmi::buffer::x_octet{};

                  result.retval = tprecv( descriptor, &buffer.data, &buffer.size, flags, &result.event);
                  result.error = tperrno;
                  result.user = tpurcode;
                  
                  if ( result.retval != -1 || // =no error or event from tprecv or
                      (result.retval == -1 && result.error == TPEEVENT && // "error" and event with possible data
                       algorithm::compare::any( result.event, TPEV_SVCFAIL, TPEV_SVCSUCC, TPEV_SENDONLY)))
                  {
                     algorithm::copy( buffer, result.payload);
                  }

                  return result;
               }
               
            } // receive

            namespace connect
            {
               struct Result
               {
                  int descriptor = -1; // return value from tpconnect, -1 on error
                  int error{};
                  long user{};

                  explicit operator bool() const noexcept { return descriptor >= 0;}

                  CASUAL_LOG_SERIALIZE(
                     CASUAL_SERIALIZE( descriptor);
                     CASUAL_SERIALIZE( error);
                     CASUAL_SERIALIZE( user);
                  )
               };
               
               template< typename T>
               Result invoke( std::string_view service, T&& payload, long flags)
               {
                  Result result;

                  auto buffer = test::unittest::xatmi::buffer::x_octet{ payload.size()};
                  algorithm::copy( payload, buffer);

                  result.descriptor = tpconnect( service.data(), buffer.data, buffer.size, flags);
                  result.error = tperrno;
                  result.user = tpurcode;

                  return result;
               }

            } // connect


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

         auto buffer = test::unittest::xatmi::buffer::x_octet{};
         
         long event = 0;
         EXPECT_TRUE( tprecv( 42, &buffer.data, &buffer.size, 0, &event) == -1);
         EXPECT_TRUE( tperrno == TPEBADDESC);
         EXPECT_TRUE( event == 0);
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

      TEST( casual_xatmi_conversation, connect__invalid_flag_TPTRAN__expect_TPEINVAL)
      {
         unittest::Trace trace;

         // TPTRAN is a flag that is input( !) to a called service
         // Not allowed in tpconnect.
         EXPECT_TRUE( tpconnect( "casual/example/conversation", nullptr, 0, TPTRAN) == -1);
         EXPECT_TRUE( tperrno == TPEINVAL) << "tperrno: " << tperrnostring( tperrno);
      }

      TEST( casual_xatmi_conversation, connect__invalid_flag_TPCONV__expect_TPEINVAL)
      {
         unittest::Trace trace;

         // TPCONV is a flag that is input( !) to a called service, set for conversational.
         // Not allowed in tpconnect.
         EXPECT_TRUE( tpconnect( "casual/example/conversation", nullptr, 0, TPCONV) == -1);
         EXPECT_TRUE( tperrno == TPEINVAL) << "tperrno: " << tperrnostring( tperrno);
      }

      TEST( casual_xatmi_conversation, connect__TPSENDONLY__absent_service___expect_TPENOENT)
      {
         unittest::Trace trace;

         auto domain = local::domain();

         EXPECT_TRUE( tpconnect( "absent", nullptr, 0, TPSENDONLY) == -1);
         EXPECT_TRUE( tperrno == TPENOENT);
      }

      TEST( casual_xatmi_conversation, connect__TPSENDONLY_discon__conversation_service)
      {
         unittest::Trace trace;

         auto domain = local::domain();
         // What happens in this test?
         // The connect goes to casual/example/conversation. This is a copy of
         // the echo service. It is wriiten as a "non-conversational service"
         // and does an immediate tpreturn with the input data and service success.
         // It ignores the TPRECVONLY flag it gets when invoked. It is
         // not normal to do a tpreturn when not in control of the session.
         // The tpreturn in the service is "unconditional" and
         // should result in a TPEV_SVCERR event to the caller and
         // failed transaction if one was active. In this case the service
         // has "join" so no transaction is active.
         //
         // This code issues a tpdiscon. This results in a message in the
         // casual log from the example-server with the following at the end
         // (line split):
         //   ...|casual-example-server||||error|[casual:internal-unexpected-value]
         //   message type: conversation_disconnect not recognized - action: discard
         // Why?
         // The caller side still believes that the conversation is active
         // so the tpdiscon() is successful and sends a disconnect message.
         // When this is received in the server/callee there is no active
         // conversation and the message is discarded causing the log message.
         // The mesage sent to caller by the tpreturn is probably also
         // discarded, but this is probably not logged by default logging
         // "(error|warning|information)".
         //
         // This "test" is repeated below but with
         // casual/example/conversation_recv_send instead.

         auto descriptor = tpconnect( "casual/example/conversation", nullptr, 0, TPSENDONLY);
         EXPECT_TRUE( descriptor != -1);
         EXPECT_TRUE( tperrno == 0);

         EXPECT_TRUE( tpdiscon( descriptor) != -1);

#ifdef HANG_WORKAROUND
         common::process::sleep( 200ms);
#endif
      }

      TEST( casual_xatmi_conversation, connect__TPSENDONLY_discon__conversation_recv_send_service)
      {
         unittest::Trace trace;

         auto domain = local::domain();
         // What happens in this test?
         // The connect goes to casual/example/conversation_recv_send. This
         // service is conversational and will call tprecv() to wait for
         // more input. This tprecv() should get an event because of the
         // tpdiscon() called in the test. What happens in the server can not
         // easily be verified by the test case!

         const std::string_view payload {"disconnect follows"};
         auto connection = local::connect::invoke( "casual/example/conversation_recv_send", payload, TPSENDONLY);
         EXPECT_TRUE( connection) << CASUAL_NAMED_VALUE( connection);
         EXPECT_TRUE( connection.error == 0) << CASUAL_NAMED_VALUE( connection);

         // Sleep for a short time before disconnect. This in practice
         // guarantees that the tprecv() in the service is called before
         // the disconnect message arrives. 
         common::process::sleep( 1s);

         EXPECT_TRUE( tpdiscon( connection.descriptor) != -1);

#ifdef HANG_WORKAROUND
         common::process::sleep( 200ms);
#endif
      }

      TEST( casual_xatmi_conversation, connect__TPSENDONLY_service_sleep_before_recv_discon__conversation_recv_send_service)
      {
         unittest::Trace trace;

         auto domain = local::domain();
         // What happens in this test?
         // This is the same as the previous test, with the small twist
         // that the service is instructed to sleep for a short time
         // before calling tprecv(). This is to cause the disconnect message to
         // arrive before the tprecv() is called. This causes different
         // timing and can cause a different code path to detect the disconnect.
         // It should still work. 

         const std::string_view payload {"disconnect follows sleep before tprecv"};
         auto connection = local::connect::invoke( "casual/example/conversation_recv_send", payload, TPSENDONLY);
         EXPECT_TRUE( connection) << CASUAL_NAMED_VALUE( connection);
         EXPECT_TRUE( connection.error == 0) << CASUAL_NAMED_VALUE( connection);

         EXPECT_TRUE( tpdiscon( connection.descriptor) != -1);

#ifdef HANG_WORKAROUND
         common::process::sleep( 200ms);
#endif
      }

      TEST( casual_xatmi_conversation, connect__TPRECVONLY__echo_service)
      {
         unittest::Trace trace;

         auto domain = local::domain();

         auto payload = unittest::random::string( 128);

         // This calls the echo service that in reality is a request/response
         // type service. It echoes the input data with a tpreturn. 
         // It "works" to call this type of service with tpconnect, but
         // the tpreturn will cause the conversation to "terminate". This
         // will result in a event TPEV_SVCSUCC on the receive ( if the service
         // terminates with success, otherwise TPEV_SVCFAIL)
         // After this event the descriptor returned by connect is no longer
         // valid, so a tpdiscon() is expected to fail.
         auto connection = local::connect::invoke( "casual/example/echo", payload, TPRECVONLY);
         EXPECT_TRUE( connection) << CASUAL_NAMED_VALUE( connection);

         // receive
         {

            auto result = local::receive::invoke( connection.descriptor, 0);

            EXPECT_TRUE( result.error == TPEEVENT)  << CASUAL_NAMED_VALUE( result);
            EXPECT_TRUE( result.event == TPEV_SVCSUCC) << CASUAL_NAMED_VALUE( result);
            EXPECT_TRUE( result.user == 0) << CASUAL_NAMED_VALUE( result);

            EXPECT_TRUE( result.payload == payload) << CASUAL_NAMED_VALUE( result);
         }

         // Conversation terminated so tpdiscon( descriptor) is expected to fail
         EXPECT_TRUE( tpdiscon( connection.descriptor) == -1);

#ifdef HANG_WORKAROUND
         // Workaround:
         common::process::sleep( 200ms);
#endif
      }

      TEST( casual_xatmi_conversation, connect__TPRECVONLY__conversation_recv_send_auto_service)
      {
         unittest::Trace trace;

         auto domain = local::domain();

         // This calls the conversation_recv_send_auto service. This test:
         // * has no started transaction
         // * send no data in the connect
         // * connnects with TPRECVONLY ( i.e. hands over control to service)
         // * issues a tprecv
         // * expects this tprecv to return with an event
         //   indicating service completion with success
         // * expects the data to contain the flags set when the service was invoked.
         //   In this test TPCONV|TPSENDONLY|TPTRAN ( =0x00000c10).
         //
         // "conversation_recv_send_auto" has the same code as "conversation_recv_send" but is
         // defined with "transaction: auto", so service should have an active
         // transaction without tx_begin() here. 
         auto descriptor = tpconnect( "casual/example/conversation_recv_send_auto", nullptr, 0, TPRECVONLY);
         ASSERT_TRUE( descriptor != -1);
         ASSERT_TRUE( tperrno == 0);

         // This receive is expected to get the result of a tpreturn
         auto result = local::receive::invoke( descriptor, TPSIGRSTRT);
         EXPECT_TRUE( result.payload == "Flags: 0x00000c10") << CASUAL_NAMED_VALUE( result);

         // conversation terminated so tpdiscon( descriptor) is expected to fail
         EXPECT_TRUE( tpdiscon( descriptor) == -1);
         // workaround for hang on shutdown:

#ifdef HANG_WORKAROUND
         common::process::sleep( 200ms);
#endif
      }

      TEST( casual_xatmi_conversation, connect__TPRECVONLY__conversation_recv_send_service)
      {
         unittest::Trace trace;

         auto domain = local::domain();

         // This calls the conversation_recv_send service. This test:
         // * has no started transaction
         // * send no data in the connect
         // * connnects with TPRECVONLY ( i.e. hands over control to service)
         // * issues a tprecv
         // * expects this tprecv to return with an event
         //   indicating service completion with success
         // * expects the data to contain the flags set when the service was invoked.
         //   In this test TPCONV|TPSENDONLY ( =0x00000c00).
         //
         // "conversation_recv_send_auto" has the same code as "conversation_recv_send" but is
         // defined with "transaction: join", so service will NOT have an active
         // transaction as we do not start one before connecting.
         auto descriptor = tpconnect( "casual/example/conversation_recv_send", nullptr, 0, TPRECVONLY);
         EXPECT_TRUE( descriptor != -1);
         EXPECT_TRUE( tperrno == 0);

         // receive
         // This receive is expected to get the result of a tpreturn
         auto result = local::receive::invoke( descriptor, TPSIGRSTRT);
         EXPECT_TRUE( result.payload == "Flags: 0x00000c00") << CASUAL_NAMED_VALUE( result);

         // conversation terminated so tpdiscon( descriptor) is expected to fail
         EXPECT_TRUE( tpdiscon( descriptor) == -1);
         // workaround for hang on shutdown:

#ifdef HANG_WORKAROUND
         common::process::sleep( 200ms);
#endif

      }

      TEST( casual_xatmi_conversation, connect__TX_TPRECVONLY__conversation_recv_send_service)
      {
         unittest::Trace trace;

         auto domain = local::domain();

         // This calls the conversational2 service. This test:
         // * has a started transaction
         // * send no data in the connect
         // * connnects with TPRECVONLY ( i.e. hands over control to service)
         // * issues a tprecv
         // * expects this tprecv to return with an event
         //   indicating service completion with success
         // * expects the data to contain the flags set when the service was
         //   invoked. In this case TPVONV|TPSENDONLY|TPTRAN ( =0x00000c10)
         //   The service has "transaction: join" 
         //
         // "conversation_recv_send" terminates the service with tpreturn and a response
         // with the invocationa flags when it gets control of the conversation.
         // ( the service will also echo all data it receives before
         //  getting control with a tpsend before the tpreturn. In this test there
         // is no data in the connect and therefore no tpsend is done by the service)
         EXPECT_TRUE( tx_begin() == TX_OK);
         auto descriptor = tpconnect( "casual/example/conversation_recv_send", nullptr, 0, TPRECVONLY);
         EXPECT_TRUE( descriptor != -1);
         EXPECT_TRUE( tperrno == 0) << "tperrno:" << tperrnostring( tperrno);

         auto result = local::receive::invoke( descriptor, TPSIGRSTRT);
         EXPECT_TRUE( result.payload == "Flags: 0x00000c10") << CASUAL_NAMED_VALUE( result);

         EXPECT_TRUE( tx_rollback() == TX_OK);

         EXPECT_TRUE( tpdiscon( descriptor) == -1);

#ifdef HANG_WORKAROUND
         common::process::sleep( 200ms);
#endif
      }

      TEST( casual_xatmi_conversation, connect__TX_TPRECVONLY_TPNOTRAN__conversation_recv_send_service)
      {
         unittest::Trace trace;

         auto domain = local::domain();

         // This calls the conversational2 service. This test:
         // * has a started transaction
         // * send no data in the connect
         // * connnects with TPRECVONLY ( i.e. hands over control to service)
         //   and TPNOTRAN. So transaction is not forwarded to service.
         //   service has "transaction: join" so should be invoked without
         //   TPTRAN flag.
         // * issues a tprecv
         // * expects this tprecv to return with an event
         //   indicating service completion with success
         // * expects the data to contain the flags set when the service was invoked
         //   In thuis case TPCONV|TPSENDONLY ( TPTRAN not set)
         EXPECT_TRUE( tx_begin() == TX_OK);
         auto descriptor = tpconnect( "casual/example/conversation_recv_send", nullptr, 0, TPRECVONLY|TPNOTRAN);
         EXPECT_TRUE( descriptor != -1);
         EXPECT_TRUE( tperrno == 0);

         // This receive is expected to get the result of a tpreturn
         auto result = local::receive::invoke( descriptor, TPSIGRSTRT);
         EXPECT_TRUE( result.payload == "Flags: 0x00000c00") << CASUAL_NAMED_VALUE( result);

         EXPECT_TRUE( tx_rollback() == TX_OK);

         EXPECT_TRUE( tpdiscon( descriptor) == -1);

#ifdef HANG_WORKAROUND
         common::process::sleep( 200ms);
#endif
      }

      TEST( casual_xatmi_conversation, connect_with_data__TX_TPRECVONLY__conversation_recv_send_service)
      {
         unittest::Trace trace;

         auto domain = local::domain();

         // This calls the conversational2 service. This test:
         // * has a started transaction
         // * send data in the connect
         // * connnects with TPRECVONLY ( i.e. transfers control of conversation).
         //   service has "transaction: join" so should be invoked with
         //   TPTRAN flag.
         // * issues a tprecv, expects data in connect back.
         // * issues a tprecv.
         //   Expects this tprecv to return with an event
         //   indicating service completion with success
         // * expects the data to contain the flags passed when the service
         //   was invoked. In this case TPCONV|TPSENDONLY|TPTRAN ( ==0xc10).
         //
         
         const std::string_view connect_string{ "data in connect"};
         EXPECT_TRUE( tx_begin() == TX_OK);

         auto connection = local::connect::invoke( "casual/example/conversation_recv_send", connect_string, TPRECVONLY);

         EXPECT_TRUE( connection) << CASUAL_NAMED_VALUE( connection);
         EXPECT_TRUE( connection.error == 0) << CASUAL_NAMED_VALUE( connection);
         // is tperrno value defined if tpconnect returned != -1? 

         // As we connected with data and TPRECVONLY the service is expected
         // to send the data back with a tpsend() ( followed by a tpreturn()).
         // Service calls tpsend with TPSENDONLY so it keeps control.
         {
            auto result = local::receive::invoke( connection.descriptor, TPSIGRSTRT);

            // it is not really defined what tperrno is set to when tprecv
            // returns != -1, but it is most likely set to 0 by casual.
            EXPECT_TRUE( result.error == 0) << "tperrno: " <<  tperrnostring( result.error);
            EXPECT_TRUE( result.payload == connect_string) << CASUAL_NAMED_VALUE( result) << ", connect_string: " << connect_string;

         }
         // This receive is expected to get the result from the tpreturn()
         auto result = local::receive::invoke( connection.descriptor, TPSIGRSTRT);
         EXPECT_TRUE( result.retval == -1) << CASUAL_NAMED_VALUE(result);
         EXPECT_TRUE( result.error == TPEEVENT) << CASUAL_NAMED_VALUE(result);
         EXPECT_TRUE( result.event == TPEV_SVCSUCC) << CASUAL_NAMED_VALUE(result);
         EXPECT_TRUE( result.payload == "Flags: 0x00000c10") << CASUAL_NAMED_VALUE( result);


         EXPECT_TRUE( tx_rollback() == TX_OK);

         EXPECT_TRUE( tpdiscon( connection.descriptor) == -1);

#ifdef HANG_WORKAROUND
         common::process::sleep( 200ms);
#endif
      }

      TEST( casual_xatmi_conversation, connect_tpsend__TX_TPRECVONLY__conversation_recv_send_service)
      {
         unittest::Trace trace;

         auto domain = local::domain();

         // This calls the conversational2 service. This test:
         // * has a started transaction
         // * send data in the connect
         // * connnects with TPSENDONLY ( i.e. keeps control of conversation).
         //   service has "transaction: join" so should be invoked with
         //   TPTRAN flag.
         // * sends data with tpsend and with TPRECVONLY flag ( transfers
         //   control of conversation)). 
         // * issues a tprecv, expects data sent in connect and tpsend back.
         // * issues a tprecv.
         //   Expects this tprecv to return with an event
         //   indicating service completion with success
         // * expects the data to contain the flags passed when the service
         //   was invoked. In this case TPCONV|TPRECVONLY|TPTRAN ( ==0x1410).
         

         std::string connect_string {"data in connect. "};

         EXPECT_TRUE( tx_begin() == TX_OK);

         auto connection = local::connect::invoke( "casual/example/conversation_recv_send", connect_string, TPSENDONLY);
         EXPECT_TRUE( connection) << CASUAL_NAMED_VALUE( connection);
         EXPECT_TRUE( connection.error == 0) << CASUAL_NAMED_VALUE( connection);

         std::string sent_data = connect_string;

         // send
         {
            std::string send_data {"Data with tpsend"};

            auto result = local::send::invoke( connection.descriptor, send_data, TPRECVONLY);
            EXPECT_TRUE( result) << CASUAL_NAMED_VALUE( result);

            sent_data += send_data;
         }
         // As we have sent data with connect and tpsend, service is expected
         // to send the data back with a tpsend() ( followed by a tpreturn()).
         {
            auto result = local::receive::invoke( connection.descriptor, 0);

            // it is not really defined what tperrno is set to when tprecv
            // returns != -1, but is most likely set to 0.
            ASSERT_TRUE( result);
          
            EXPECT_TRUE( result.payload == sent_data) << CASUAL_NAMED_VALUE( result);
                                                    
         }
         // This receive is expected to get the result from the tpreturn()
         auto result = local::receive::invoke( connection.descriptor, TPSIGRSTRT);
         EXPECT_TRUE( result.retval == -1) << CASUAL_NAMED_VALUE( result);
         EXPECT_TRUE( result.error == TPEEVENT) << CASUAL_NAMED_VALUE( result);
         EXPECT_TRUE( result.event == TPEV_SVCSUCC) << CASUAL_NAMED_VALUE( result);
         EXPECT_TRUE( result.payload == "Flags: 0x00001410") << CASUAL_NAMED_VALUE( result);

         EXPECT_TRUE( tx_rollback() == TX_OK);

         EXPECT_TRUE( tpdiscon( connection.descriptor) == -1);

#ifdef HANG_WORKAROUND
         common::process::sleep( 200ms);
#endif
      }

      TEST( casual_xatmi_conversation, connect_tpsend_tpsend__TX_TPRECVONLY__conversation_recv_send_service)
      {
         unittest::Trace trace;

         auto domain = local::domain();

         // This calls the conversational2 service. This test:
         // * has a started transaction
         // * send data in the connect
         // * connnects with TPSENDONLY ( i.e. keeps control of conversation).
         //   service has "transaction: join" so should be invoked with
         //   TPTRAN flag.
         // * sends data with tpsend and TPSENDONLY
         // * sends data with tpsend and TPRECVONLY flag ( transfers
         //   control of conversation)). 
         // * issues a tprecv, expects data sent in connect and tpsend back.
         // * issues a tprecv.
         //   Expects this tprecv to return with an event
         //   indicating service completion with success
         // * expects the data to contain the flags passed when the service
         //   was invoked. In this case TPCONV|TPRECVONLY|TPTRAN ( ==0x1410).
         //
         const std::string connect_string {"data in connect. "};

         EXPECT_TRUE( tx_begin() == TX_OK);

         auto connection = local::connect::invoke( "casual/example/conversation_recv_send", connect_string, TPSENDONLY);
         EXPECT_TRUE( connection) << CASUAL_NAMED_VALUE( connection);
         EXPECT_TRUE( connection.error == 0) << CASUAL_NAMED_VALUE( connection);

         std::string sent_data = connect_string;

         // first send
         {
            std::string send_data {"Data with tpsend"};

            auto result = local::send::invoke( connection.descriptor, send_data, TPSIGRSTRT);
            EXPECT_TRUE( result) << CASUAL_NAMED_VALUE( result);

            sent_data += send_data;

         }
         // second send, transfer control
         {
            std::string send_data {"Data2 with tpsend"};

            auto result = local::send::invoke( connection.descriptor, send_data, TPSIGRSTRT|TPRECVONLY);
            EXPECT_TRUE( result) << CASUAL_NAMED_VALUE( result);

            sent_data += send_data;
         }

         // As we have sent data with connect and tpsend, service is expected
         // to send the data back with a tpsend() ( followed by a tpreturn()).
         {
            auto result = local::receive::invoke( connection.descriptor, TPSIGRSTRT);

            // it is not really defined what tperrno is set to when tprecv
            // returns != -1, but is most likely set to 0 by casual.
            EXPECT_TRUE( result);
            EXPECT_TRUE( result.payload == sent_data) << CASUAL_NAMED_VALUE( result);
         }
         // This receive is expected to get the result from the tpreturn()
         auto result = local::receive::invoke( connection.descriptor, TPSIGRSTRT);
         EXPECT_TRUE( result.retval == -1) << CASUAL_NAMED_VALUE( result);
         EXPECT_TRUE( result.error == TPEEVENT) << CASUAL_NAMED_VALUE( result);
         EXPECT_TRUE( result.event == TPEV_SVCSUCC) << CASUAL_NAMED_VALUE( result);
         EXPECT_TRUE( result.payload == "Flags: 0x00001410") << CASUAL_NAMED_VALUE( result);

         EXPECT_TRUE( tx_rollback() == TX_OK);

         EXPECT_TRUE( tpdiscon( connection.descriptor) == -1);

#ifdef HANG_WORKAROUND
         common::process::sleep( 200ms);
#endif
      }

      namespace local
      {
         namespace
         {
            // helper that executes a conversational session that:
            // calls the conversational2 service. 
            // * starts a transaction
            // * send data in the connect
            // * connnects with TPSENDONLY ( i.e. keeps control of conversation).
            //   service has "transaction: join" so should be invoked with
            //   TPTRAN flag.
            // * sends data with tpsend and with TPRECVONLY flag ( transfers
            //   control of conversation)). 
            // * issues a tprecv, expects data sent in connect and tpsend back.
            // * issues a tprecv.
            //   Expects this tprecv to return with an event
            //   indicating service completion with success
            // * expects the data to contain the flags passed when the service
            //   was invoked. In this case TPCONV|TPRECVONLY|TPTRAN ( ==0x1410).
            // * rollbacks the transaction
            //
            // Used in a test cases that do this multiple times against
            // the same domain. 
            auto connect_send_recv()
            {
               std::string connect_string {"data in connect. "};

               EXPECT_TRUE( tx_begin() == TX_OK);

               auto connection = local::connect::invoke( "casual/example/conversation_recv_send", connect_string, TPSENDONLY);
               ASSERT_TRUE( connection) << CASUAL_NAMED_VALUE( connection);
               ASSERT_TRUE( connection.error == 0) << CASUAL_NAMED_VALUE( connection);

               // Expect descriptor to be #0. Other numbers probably mean that there is a
               // resource leak and that descriptors are not freed and reused. 
               EXPECT_TRUE( connection.descriptor == 0) << CASUAL_NAMED_VALUE( connection);

               std::string sent_data = connect_string;

               // send
               {
                  unittest::Trace::line( "send");

                  std::string send_data{ "Data with tpsend"};

                  auto result = local::send::invoke( connection.descriptor, send_data, TPRECVONLY);
                  ASSERT_TRUE( result) << CASUAL_NAMED_VALUE( result);

                  sent_data += send_data;
               }
               // As we have sent data with connect and tpsend, service is expected
               // to send the data back with a tpsend() ( followed by a tpreturn()).
               {
                  unittest::Trace::line( "receive");

                  auto result = local::receive::invoke( connection.descriptor, TPSIGRSTRT);
                  ASSERT_TRUE( result) << CASUAL_NAMED_VALUE( result);
               
                  EXPECT_TRUE( result.payload == sent_data) << CASUAL_NAMED_VALUE( result);
               }

               unittest::Trace::line( "receive tpreturn");

               // This receive is expected to get the result from the tpreturn()
               auto result = local::receive::invoke( connection.descriptor, TPSIGRSTRT);
               EXPECT_TRUE( result.retval == -1) << CASUAL_NAMED_VALUE( result);
               EXPECT_TRUE( result.error == TPEEVENT) << CASUAL_NAMED_VALUE( result);
               EXPECT_TRUE( result.event == TPEV_SVCSUCC) << CASUAL_NAMED_VALUE( result);
               EXPECT_TRUE( result.payload == "Flags: 0x00001410") << CASUAL_NAMED_VALUE( result);

               EXPECT_TRUE( tx_rollback() == TX_OK);

               EXPECT_TRUE( tpdiscon( connection.descriptor) == -1);
            }

         } // <unnamed>
      } // local


      // This test verifies that a service can be called multiple times.
      // In this test case the service calls are "independent" from a transaction
      // point of view.
      TEST( casual_xatmi_conversation, do_10_connect_tpsend__TX_TPRECVONLY__conversation_recv_send_service)
      {
         unittest::Trace trace;


         auto domain = local::domain();
         
         algorithm::for_n< 10>( []()
         {
            local::connect_send_recv();
         });

#ifdef HANG_WORKAROUND
         common::process::sleep( 200ms);
#endif
      }

      TEST( casual_xatmi_conversation, interdomain_connect_send_disconnect)
      {
         unittest::Trace trace;

         auto b = local::domain( local::configuration::example, R"(
domain:
   name: B
   gateway:
      inbound:
         groups:
            -  connections:
                  - address: 127.0.0.1:7001
)");

         auto a = local::domain( R"(
domain:
   name: A
   gateway:
      outbound:
         groups:
            -  connections:
                  -  address: 127.0.0.1:7001
)");

         test::unittest::gateway::state::until( test::unittest::gateway::state::predicate::outbound::connected());

         local::connect_send_recv();

      }



      // More test cases to consider/TBD:
      // * Multiple concurrent conversations
      // * disconnect? (no well designed application should
      //   use disconnect! At least in my opinion...)
      // * Conversational service that return failure
      // * conversatiomnal service that in turn uses tpcall/tpconnect
      // * failures in such services.
      //
      // Additional aspects to verify:
      // * Are transactions committed and rolled back properly.
      // * Service transaction modes (auto, join, none, atomic).
      // * Check descriptor number in service? (At one time had a server side
      //   leak of descriptors that only showed up in casual log on server
      //   shutdown. Could handle this by returning service side descriptor
      //   in the same way as service side flags.)
      //
      // IFF we implement a mechanism for test cases to tell the service what
      // to do we could in principle tell it what to check also!
      // But a "general language" maybe a bit of overkill... 
      // Note that a test case with multiple concurrent conversations requires
      // multiple server instances. 


   } // xatmi
} // casual

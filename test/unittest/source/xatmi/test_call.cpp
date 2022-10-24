//! 
//! Copyright (c) 2015, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!


// to be able to use 'raw' flags and codes
// since we undefine 'all' of them in common
#define CASUAL_NO_XATMI_UNDEFINE

#include "common/unittest.h"

#include "domain/unittest/manager.h"

#include "common/flag.h"

#include "common/message/transaction.h"
#include "common/transaction/id.h"
#include "common/environment/scoped.h"
#include "common/execute.h"
#include "common/unittest/file.h"


#include "casual/xatmi.h"
#include "casual/xatmi/extended.h"


#include <map>
#include <vector>

namespace casual
{
   using namespace common;

   namespace test
   {
      namespace local
      {
         namespace
         {
            namespace configuration
            {
               constexpr auto system = R"(
system:
   resources:
      -  key: rm-mockup
         server: ${CASUAL_MAKE_SOURCE_ROOT}/middleware/transaction/bin/rm-proxy-casual-mockup
         xa_struct_name: casual_mockup_xa_switch_static
         libraries:
            -  casual-mockup-rm
)";
            }

            template< typename... C>
            auto domain( C&&... configurations) 
            {
               return casual::domain::unittest::manager( configuration::system, std::forward< C>( configurations)...);
            }

            auto domain()
            {
               return domain( R"(
domain:
   name: test-default-domain

   transaction:
      log: ":memory:"
      resources:
         - key: rm-mockup
           name: example-resource-server
           instances: 1
           openinfo: "${CASUAL_UNITTEST_OPEN_INFO}"

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
      - path: ${CASUAL_MAKE_SOURCE_ROOT}/middleware/example/server/bin/casual-example-resource-server
        memberships: [ example]
)");
            }

         } // <unnamed>
      } // local




      TEST( test_xatmi_call, tpalloc_X_OCTET_binary__expect_ok)
      {
         common::unittest::Trace trace;

         auto buffer = tpalloc( X_OCTET, nullptr, 128);
         EXPECT_TRUE( buffer != nullptr) << "tperrno: " << tperrno;
         tpfree( buffer);
      }

      TEST( test_xatmi_call, tpacall_service_null__expect_TPEINVAL)
      {
         common::unittest::Trace trace;

         auto domain = local::domain();

         char buffer[ 100];
         EXPECT_TRUE( tpacall( nullptr, buffer, 0, 0) == -1);
         EXPECT_TRUE( tperrno == TPEINVAL) << "tperrno: " << tperrno;
      }

      TEST( test_xatmi_call, tpacall_TPNOREPLY_without_TPNOTRAN__in_transaction___expect_TPEINVAL)
      {
         common::unittest::Trace trace;

         auto domain = local::domain();

         tx_begin();

         auto rollback = common::execute::scope( [](){
            tx_rollback();
         });

         char buffer[ 100];
         EXPECT_TRUE( tpacall( "some-service", buffer, 100, TPNOREPLY) == -1);
         EXPECT_TRUE( tperrno == TPEINVAL) << "tperrno: " << tperrno;
      }

      TEST( test_xatmi_call, tpcancel_descriptor_42__expect_TPEBADDESC)
      {
         common::unittest::Trace trace;

         auto domain = local::domain();

         EXPECT_TRUE( tpcancel( 42) == -1);
         EXPECT_TRUE( tperrno == TPEBADDESC) << "tperrno: " << tperrno;
      }


      TEST( test_xatmi_call, tx_rollback__no_transaction__expect_TX_PROTOCOL_ERROR)
      {
         common::unittest::Trace trace;

         auto domain = local::domain();

         EXPECT_TRUE( tx_rollback() == TX_PROTOCOL_ERROR);
      }


      TEST( test_xatmi_call, tpgetrply_descriptor_42__expect_TPEBADDESC)
      {
         common::unittest::Trace trace;

         auto domain = local::domain();

         int descriptor = 42;
         auto buffer = tpalloc( X_OCTET, nullptr, 128);
         auto len = tptypes( buffer, nullptr, nullptr);

         EXPECT_TRUE( tpgetrply( &descriptor, &buffer, &len, 0) == -1);
         EXPECT_TRUE( tperrno == TPEBADDESC) << "tperrno: " << tperrno;

         tpfree( buffer);
      }

      TEST( test_xatmi_call, tpacall_service_XXX__expect_TPENOENT)
      {
         common::unittest::Trace trace;

         auto domain = local::domain();

         auto buffer = tpalloc( X_OCTET, nullptr, 128);

         EXPECT_TRUE( tpacall( "XXX", buffer, 128, 0) == -1);
         EXPECT_TRUE( tperrno == TPENOENT) << "tperrno: " << tperrnostring( tperrno);


         tpfree( buffer);
      }

      namespace local
      {
         namespace
         {
            auto allocate( platform::size::type size = 128)
            {
               auto buffer = tpalloc( X_OCTET, nullptr, size);
               unittest::random::range( range::make( buffer, size));
               return buffer;
            }

            bool call( const std::string& service, platform::size::type size = 128)
            {
               auto buffer = allocate( size);
               auto len = tptypes( buffer, nullptr, nullptr);
               auto code = tpcall( service.c_str(), buffer, size, &buffer, &len, 0);

               tpfree( buffer);

               return code == 0;
            }


         } // <unnamed>
      } // local

      TEST( test_xatmi_call, tpacall_buffer_null__expect_expect_ok)
      {
         common::unittest::Trace trace;

         auto domain = local::domain();

         auto descriptor = tpacall( "casual/example/echo", nullptr, 0, 0);
         EXPECT_TRUE( descriptor != -1) << "tperrno: " << tperrno;
         EXPECT_TRUE( tpcancel( descriptor) != -1);
      }


      TEST( test_xatmi_call, tpacall_service_echo_TPNOREPLY_TPNOTRAN__expect_ok)
      {
         common::unittest::Trace trace;

         auto domain = local::domain();

         auto buffer = local::allocate( 128);

         EXPECT_TRUE( tpacall( "casual/example/echo", buffer, 128, TPNOREPLY | TPNOTRAN) == 0) << "tperrno: " << tperrnostring( tperrno);

         tpfree( buffer);
      }

      TEST( test_xatmi_call, tpacall_echo_TPNOREPLY__no_current_transaction__expect_ok)
      {
         common::unittest::Trace trace;

         auto domain = local::domain();

         auto buffer = tpalloc( X_OCTET, nullptr, 128);

         EXPECT_TRUE( tpacall( "casual/example/echo", buffer, 128, TPNOREPLY ) == 0) << "tperrno: " << tperrnostring( tperrno);

         tpfree( buffer);
      }

      TEST( test_xatmi_call, tpacall_echo_TPNOREPLY_ongoing_current_transaction__expect_TPEINVAL)
      {
         common::unittest::Trace trace;

         auto domain = local::domain();

         EXPECT_TRUE( tx_begin() == TX_OK);

         auto buffer = tpalloc( X_OCTET, nullptr, 128);

         EXPECT_TRUE( tpacall( "casual/example/echo", buffer, 128, TPNOREPLY ) == -1) << "tperrno: " << tperrnostring( tperrno);
         EXPECT_TRUE( tperrno == TPEINVAL);

         EXPECT_TRUE( tx_rollback() == TX_OK);

         tpfree( buffer);
      }


      TEST( test_xatmi_call, tpcall_service_echo__expect_ok)
      {
         common::unittest::Trace trace;

         auto domain = local::domain();

         auto buffer = local::allocate( 128);
         auto len = tptypes( buffer, nullptr, nullptr);

         EXPECT_TRUE( tpcall( "casual/example/echo", buffer, 128, &buffer, &len, 0) == 0) << "tperrno: " << tperrnostring( tperrno);

         tpfree( buffer);
      }

      TEST( test_xatmi_call, tpcall_service_resource_echo__rm_xa_start_gives_XA_RBROLLBACK__expect_TPESVCERR)
      {
         common::unittest::Trace trace;

         // we set unittest environment variable to set "error"
         auto scope = common::environment::variable::scoped::set( "CASUAL_UNITTEST_OPEN_INFO", common::string::compose( "--start ", XA_RBROLLBACK));

         auto domain = local::domain();

         ASSERT_EQ( tx_begin(), TX_OK);

         auto buffer = local::allocate( 128);
         auto len = tptypes( buffer, nullptr, nullptr);

         EXPECT_TRUE( tpcall( "casual/example/resource/echo", buffer, 128, &buffer, &len, 0) == -1);
         EXPECT_TRUE( tperrno == TPESVCERR) << "tperrno: " << tperrnostring( tperrno);
         tpfree( buffer);

         EXPECT_EQ( tx_rollback(), TX_OK);
      }

      // Test with invalid arguments. Invalid flags should give TPEINVAL.
      TEST( casual_xatmi, tpcall_service_echo_invalid_flag__expect_TPEINVAL)
      {
         common::unittest::Trace trace;

         auto domain = local::domain();

         auto buffer = local::allocate( 128);
         auto len = tptypes( buffer, nullptr, nullptr);
         // TPSENDONLY is an invalid flag to tpcall. There are many more
         // possible invalid arguments or flags and we try some of them.
         EXPECT_TRUE( tpcall( "casual/example/echo", buffer, 128, &buffer, &len, TPSENDONLY) == -1);
         EXPECT_TRUE( tperrno == TPEINVAL) << "tperrno: " << tperrnostring( tperrno);

         EXPECT_TRUE( tpcall( "casual/example/echo", buffer, 128, &buffer, &len, TPRECVONLY) == -1);
         EXPECT_TRUE( tperrno == TPEINVAL) << "tperrno: " << tperrnostring( tperrno);

         EXPECT_TRUE( tpcall( "casual/example/echo", buffer, 128, &buffer, &len, TPSENDONLY|TPRECVONLY) == -1);
         EXPECT_TRUE( tperrno == TPEINVAL) << "tperrno: " << tperrnostring( tperrno);

         EXPECT_TRUE( tpcall( "casual/example/echo", buffer, 128, &buffer, &len, TPRECVONLY) == -1);
         EXPECT_TRUE( tperrno == TPEINVAL) << "tperrno: " << tperrnostring( tperrno);

         EXPECT_TRUE( tpcall( "casual/example/echo", buffer, 128, &buffer, &len, TPTRAN) == -1);
         EXPECT_TRUE( tperrno == TPEINVAL) << "tperrno: " << tperrnostring( tperrno);

         EXPECT_TRUE( tpcall( "casual/example/echo", buffer, 128, &buffer, &len, TPNOREPLY) == -1);
         EXPECT_TRUE( tperrno == TPEINVAL) << "tperrno: " << tperrnostring( tperrno);

         EXPECT_TRUE( tpcall( "casual/example/echo", buffer, 128, &buffer, &len, TPGETANY) == -1);
         EXPECT_TRUE( tperrno == TPEINVAL) << "tperrno: " << tperrnostring( tperrno);

         EXPECT_TRUE( tpcall( "casual/example/echo", buffer, 128, &buffer, &len, TPCONV) == -1);
         EXPECT_TRUE( tperrno == TPEINVAL) << "tperrno: " << tperrnostring( tperrno);

         // nullptr for service name
         EXPECT_TRUE( tpcall( nullptr, buffer, 128, &buffer, &len, 0) == -1);
         EXPECT_TRUE( tperrno == TPEINVAL) << "tperrno: " << tperrnostring( tperrno);

         tpfree( buffer);
      }

      TEST( test_xatmi_call, no_trid__tpcall_service_forward_echo__auto_tran____expect_TPESVCERR)
      {
         common::unittest::Trace trace;

         auto domain = local::domain();

         auto buffer = local::allocate( 128);
         auto len = tptypes( buffer, nullptr, nullptr);

         EXPECT_TRUE( tpcall( "casual/example/forward/echo", buffer, 128, &buffer, &len, 0) == -1);
         EXPECT_TRUE( tperrno == TPESVCERR) << "tperrno: " << tperrnostring( tperrno);

         tpfree( buffer);
      }

      TEST( test_xatmi_call, trid__tpcall_service_forward_echo__auto_tran____expect_OK)
      {
         common::unittest::Trace trace;

         auto domain = local::domain();

         EXPECT_TRUE( tx_begin() == TX_OK);

         auto buffer = local::allocate( 128);
         auto len = tptypes( buffer, nullptr, nullptr);

         EXPECT_TRUE( tpcall( "casual/example/forward/echo", buffer, 128, &buffer, &len, 0) == 0) << "tperrno: " << tperrnostring( tperrno);

         EXPECT_TRUE( tx_rollback() == TX_OK);

         tpfree( buffer);
      }

      TEST( test_xatmi_call, tpcall_service_echo__1MiB_buffer___expect_ok)
      {
         common::unittest::Trace trace;

         auto domain = local::domain();

         constexpr auto size = 1024 * 1024;

         auto input = local::allocate( size);
         auto output = local::allocate( 128);


         auto len = tptypes( output, nullptr, nullptr);

         EXPECT_TRUE( tpcall( "casual/example/echo", input, size, &output, &len, 0) == 0) << "tperrno: " << tperrnostring( tperrno);

         EXPECT_TRUE( algorithm::equal( range::make( input, size), range::make( output, len)));

         tpfree( input);
         tpfree( output);
      }

      TEST( test_xatmi_call, tpacall_service_echo__no_transaction__tpcancel___expect_ok)
      {
         common::unittest::Trace trace;

         auto domain = local::domain();

         auto buffer = tpalloc( X_OCTET, nullptr, 128);

         auto descriptor = tpacall( "casual/example/echo", buffer, 128, 0);
         EXPECT_TRUE( descriptor == 1) << "desc: " << descriptor << " tperrno: " << tperrnostring( tperrno);
         EXPECT_TRUE( tpcancel( descriptor) != -1)  << "tperrno: " << tperrnostring( tperrno);

         tpfree( buffer);
      }


      TEST( test_xatmi_call, tpacall_service_echo__10_times__no_transaction__tpcancel_all___expect_ok)
      {
         common::unittest::Trace trace;

         auto domain = local::domain();

         auto buffer = tpalloc( X_OCTET, nullptr, 128);

         std::vector< int> descriptors( 10);

         for( auto& desc : descriptors)
         {
            desc = tpacall( "casual/example/echo", buffer, 128, 0);
            EXPECT_TRUE( desc > 0) << "tperrno: " << tperrnostring( tperrno);
         }

         std::vector< int> expected_descriptors{ 1, 2, 3, 4, 5, 6, 7 ,8 , 9, 10};
         EXPECT_TRUE( descriptors == expected_descriptors) << CASUAL_NAMED_VALUE( descriptors);

         for( auto& desc : descriptors)
         {
            EXPECT_TRUE( tpcancel( desc) != -1)  << "tperrno: " << tperrnostring( tperrno);
         }

         tpfree( buffer);
      }


      TEST( test_xatmi_call, tpacall_service_echo__10_times___tpgetrply_any___expect_ok)
      {
         common::unittest::Trace trace;

         auto domain = local::domain();


         auto buffer = tpalloc( X_OCTET, nullptr, 128);

         std::vector< int> descriptors( 10);

         for( auto& desc : descriptors)
         {
            desc = tpacall( "casual/example/echo", buffer, 128, 0);
            EXPECT_TRUE( desc > 0) << "tperrno: " << tperrnostring( tperrno);
         }

         std::vector< int> expected_descriptors{ 1, 2, 3, 4, 5, 6, 7 ,8 , 9, 10};


         std::vector< int> fetched( 10);

         for( auto& fetch : fetched)
         {
            auto len = tptypes( buffer, nullptr, nullptr);
            EXPECT_TRUE( tpgetrply( &fetch, &buffer, &len, TPGETANY) != -1)  << "tperrno: " << tperrnostring( tperrno);
         }

         EXPECT_TRUE( descriptors == fetched) << CASUAL_NAMED_VALUE( descriptors) << " - " << CASUAL_NAMED_VALUE( fetched);

         tpfree( buffer);
      }


      TEST( test_xatmi_call, tx_begin__tpacall_service_echo__10_times___tpgetrply_all__tx_commit__expect_ok)
      {
         common::unittest::Trace trace;

         auto domain = local::domain();

         EXPECT_TRUE( tx_begin() == TX_OK);

         auto buffer = tpalloc( X_OCTET, nullptr, 128);

         std::vector< int> descriptors( 10);

         for( auto& desc : descriptors)
         {
            desc = tpacall( "casual/example/echo", buffer, 128, 0);
            EXPECT_TRUE( desc > 0) << "tperrno: " << tperrnostring( tperrno);
         }

         std::vector< int> expected_descriptors{ 1, 2, 3, 4, 5, 6, 7 ,8 , 9, 10};
         EXPECT_TRUE( descriptors == expected_descriptors) << CASUAL_NAMED_VALUE( descriptors);

         EXPECT_TRUE( tx_commit() == TX_PROTOCOL_ERROR);

         for( auto& desc : descriptors)
         {
            auto len = tptypes( buffer, nullptr, nullptr);
            EXPECT_TRUE( tpgetrply( &desc, &buffer, &len, 0) != -1)  << "tperrno: " << tperrnostring( tperrno);
         }

         EXPECT_TRUE( tx_commit() == TX_OK);

         tpfree( buffer);
      }

      TEST( test_xatmi_call, tx_begin__tpcall_service_echo__tx_commit___expect_ok)
      {
         common::unittest::Trace trace;

         auto domain = local::domain();


         EXPECT_TRUE( tx_begin() == TX_OK);

         auto buffer = tpalloc( X_OCTET, nullptr, 128);

         auto len = tptypes( buffer, nullptr, nullptr);
         EXPECT_TRUE( tpcall( "casual/example/echo", buffer, 128, &buffer, &len, 0) == 0) << "tperrno: " << tperrnostring( tperrno);

         EXPECT_TRUE( tx_commit() == TX_OK);

         tpfree( buffer);
      }

      TEST( test_xatmi_call, tx_begin__tpacall_service_echo__tx_commit___expect_TX_PROTOCOL_ERROR)
      {
         common::unittest::Trace trace;

         auto domain = local::domain();


         EXPECT_TRUE( tx_begin() == TX_OK);

         auto buffer = tpalloc( X_OCTET, nullptr, 128);

         EXPECT_TRUE( tpacall( "casual/example/echo", buffer, 128, 0) != 0) << "tperrno: " << tperrnostring( tperrno);

         // can't commit when there are pending replies
         EXPECT_TRUE( tx_commit() == TX_PROTOCOL_ERROR);
         // we can always rollback the transaction
         EXPECT_TRUE( tx_rollback() == TX_OK);

         tpfree( buffer);
      }


      TEST( test_xatmi_call, tpcall_service_urcode__expect_ok__urcode_42)
      {
         common::unittest::Trace trace;

         auto domain = local::domain();

         EXPECT_TRUE( local::call( "casual/example/error/urcode")) << "tperrno: " << tperrnostring( tperrno);
         EXPECT_TRUE( tpurcode == 42) << "urcode: " << tpurcode;
      }



      TEST( test_xatmi_call, tpcall_service_TPEOS___expect_error_TPEOS)
      {
         common::unittest::Trace trace;

         auto domain = local::domain();

         EXPECT_FALSE( local::call( "casual/example/error/TPEOS"));
         EXPECT_TRUE( tperrno == TPEOS) << "tperrno: " << tperrno;
      }

      TEST( test_xatmi_call, tpcall_service_TPEPROTO___expect_error_TPEPROTO)
      {
         common::unittest::Trace trace;

         auto domain = local::domain();

         EXPECT_FALSE( local::call( "casual/example/error/TPEPROTO"));
         EXPECT_TRUE( tperrno == TPEPROTO) << "tperrno: " << tperrno;
      }

      TEST( test_xatmi_call, tpcall_service_TPESVCERR___expect_error_TPESVCERR)
      {
         common::unittest::Trace trace;

         auto domain = local::domain();

         EXPECT_FALSE( local::call( "casual/example/error/TPESVCERR"));
         EXPECT_TRUE( tperrno == TPESVCERR) << "tperrno: " << tperrno;
      }

      TEST( test_xatmi_call, tpcall_service_TPESYSTEM___expect_error_TPESYSTEM)
      {
         common::unittest::Trace trace;

         auto domain = local::domain();

         EXPECT_FALSE( local::call( "casual/example/error/TPESYSTEM"));
         EXPECT_TRUE( tperrno == TPESYSTEM) << "tperrno: " << tperrno;
      }

      TEST( test_xatmi_call, tpcall_service_TPESVCFAIL___expect_error_TPESVCFAIL)
      {
         common::unittest::Trace trace;

         auto domain = local::domain();

         EXPECT_FALSE( local::call( "casual/example/error/TPESVCFAIL"));
         EXPECT_TRUE( tperrno == TPESVCFAIL) << "tperrno: " << tperrno;
      }

      namespace local
      {
         namespace
         {
            void service( TPSVCINFO *)
            {

            }

            int callback( const casual_browsed_service* service, void* context)
            {
               auto services = static_cast< std::vector< std::string>*>( context);
               services->emplace_back( service->name);
               return 0;

            }
         } // <unnamed>
      } // local

      TEST( test_xatmi_call_extended, casual_instance_browse_services)
      {
         common::unittest::Trace trace;

         auto domain = local::domain();

         tpadvertise( "a", &local::service);
         tpadvertise( "b", &local::service);
         tpadvertise( "c", &local::service);

         std::vector< std::string> services;

         casual_instance_browse_services( &local::callback, &services);

         EXPECT_TRUE(( algorithm::sort( services) == std::vector< std::string>{ "a", "b", "c"})) << CASUAL_NAMED_VALUE( services);

         tpunadvertise( "a");
         tpunadvertise( "b");
         tpunadvertise( "c");
      }

   } // test
} // casual

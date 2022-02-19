//!
//! Copyright (c) 2019, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!


#include "common/unittest.h"
#include "common/unittest/file.h"

#include "domain/unittest/manager.h"

#include "common/communication/instance.h"
#include "common/transcode.h"

#include "xatmi.h"

namespace casual
{
   using namespace common;

   namespace test
   {
      namespace domain
      {
         namespace local
         {
            namespace
            {
               namespace configuration
               {
                  auto a()
                  {
                     return  R"(
domain:
   name: A
   transaction:
      log: ":memory:"

   executables:
      - path: ${CASUAL_UNITTEST_HTTP_INBOUND_PATH}
        alias: casual-http-inbound
        arguments: [ -p, "${CASUAL_DOMAIN_HOME}", -c, "${CASUAL_UNITTEST_HTTP_INBOUND_CONFIG}" ]
   
   servers:
      - path: ${CASUAL_MAKE_SOURCE_ROOT}/middleware/service/bin/casual-service-manager
      - path: ${CASUAL_MAKE_SOURCE_ROOT}/middleware/example/server/bin/casual-example-server
)";
                  }

                  auto b( const std::string& outbound)
                  {
                     return R"(
domain:
   name: B
   transaction:
      log: ":memory:"

   servers:
      - path: ${CASUAL_MAKE_SOURCE_ROOT}/middleware/service/bin/casual-service-manager
      - path: ${CASUAL_MAKE_SOURCE_ROOT}/middleware/http/bin/casual-http-outbound
        arguments: [ --configuration, )" + outbound + "]";
      
                  }

                  auto outbound() 
                  {
                     return unittest::file::temporary::content( ".yaml", R"(
http:
  services:
    - name: casual/example/echo
      url: http://localhost:8042/casual/casual/example/echo
)");
                  }
                  
               } // configuration

               void call_echo_in_other_domain( const std::string& buffer_type)
               {
                  auto outbound = local::configuration::outbound();

                  auto a = casual::domain::unittest::manager( local::configuration::a());
                  auto b = casual::domain::unittest::manager( local::configuration::b( outbound));

                  {
                     b.activate();

                     auto string = unittest::random::string( 200);
                     string.push_back( '\0');

                     auto buffer = tpalloc( buffer_type.c_str(), nullptr, string.size());
                     assert( buffer);
                     common::algorithm::copy( string, buffer);
                     auto len = tptypes( buffer, nullptr, nullptr);
                     EXPECT_TRUE( len == static_cast< long>( string.size()));

                     EXPECT_TRUE( tpcall( "casual/example/echo", buffer, len, &buffer, &len, 0) == 0) << "tperrno: " << tperrnostring( tperrno);
                     auto result = common::range::make( buffer, len);

                     EXPECT_TRUE( algorithm::equal( string, result)) 
                        << "binary: " << common::transcode::hex::encode( string) << '\n'
                        << "result: " << common::transcode::hex::encode( result);


                     tpfree( buffer);
                  }

               }

            } // <unnamed>
         } // local
        


         TEST( test_http, call_echo_in_other_domain_X_OCTET)
         {
            common::unittest::Trace trace;

            local::call_echo_in_other_domain( X_OCTET);
         }

         TEST( test_http, call_echo_in_other_domain_CSTRING)
         {
            common::unittest::Trace trace;

            local::call_echo_in_other_domain( "CSTRING");
         }

         struct Count 
         {
            long payload{};
            long calls{};

            friend std::ostream& operator << ( std::ostream& out, const Count& value)
            {
               return out << "{ payload: " << value.payload
                  << "{ calls: " << value.calls
                  << '}';
            }
         };


         struct test_http_parallel : ::testing::TestWithParam< Count> 
         {

         };


         TEST_P( test_http_parallel, call_echo_in_other_domain)
         {
            common::unittest::Trace trace;

            auto count = GetParam();

            auto outbound = local::configuration::outbound();

            auto a = casual::domain::unittest::manager( local::configuration::a());
            auto b = casual::domain::unittest::manager( local::configuration::b( outbound));

            {
               b.activate();

               auto binary = unittest::random::binary( count.payload);
               std::vector< int> descriptors;
               descriptors.resize( count.calls);
               

               for( auto& descriptor : descriptors)
               {
                  auto buffer = tpalloc( X_OCTET, nullptr, binary.size());
                  common::algorithm::copy( binary, buffer);
                  auto len = tptypes( buffer, nullptr, nullptr);

                  descriptor = tpacall( "casual/example/echo", buffer, len, 0);

                  EXPECT_TRUE( descriptor > 0) << "tperrno: " << tperrnostring( tperrno);

                  // we do a flush to keep inbound clear
                  // TODO: remove this when http-outbound uses retry-call
                  common::communication::ipc::inbound::device().flush();

                  ::tpfree( buffer);
               }

               for( auto& descriptor : descriptors)
               {
                  auto buffer = tpalloc( X_OCTET, nullptr, binary.size());
                  common::algorithm::copy( binary, buffer);
                  auto len = tptypes( buffer, nullptr, nullptr);

                  EXPECT_TRUE( tpgetrply( &descriptor, &buffer, &len, 0) == 0) << "tperrno: " << tperrnostring( tperrno);

                  auto result = common::range::make( buffer, len);
                  EXPECT_TRUE( algorithm::equal( binary, result));

                  ::tpfree( buffer);
               }
            }
         }


         const std::vector< Count> counts{
            Count{ 100, 10},
            Count{ 100, 100},
            Count{ 1000, 100},
            Count{ 10000, 100},
            Count{ 1000, 1000},
            //Count{ 1000, 10000},
         };

         INSTANTIATE_TEST_SUITE_P( 
            http,
            test_http_parallel,
            ::testing::ValuesIn( counts)
         );
      
      } // domain
   } // test
} // casual
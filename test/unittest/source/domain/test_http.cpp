//!
//! Copyright (c) 2019, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!


#include "common/unittest.h"


#include "casual/test/domain.h"

#include "common/communication/instance.h"

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
                     return mockup::file::temporary::content( ".yaml", R"(
domain:
   name: A
   transaction:
      log: ":memory:"

   executables:
      - path: ${CASUAL_UNITTEST_HTTP_INBOUND_PATH}
        alias: casual-http-inbounds
        arguments: [ -p, "${CASUAL_DOMAIN_HOME}", -c, "${CASUAL_UNITTEST_HTTP_INBOUND_CONFIG}" ]
   
   servers:
      - path: ${CASUAL_HOME}/bin/casual-example-server
)");
                  }

                  auto b( const std::string& outbound)
                  {
                     return mockup::file::temporary::content( ".yaml", R"(
domain:
   name: B
   transaction:
      log: ":memory:"

   servers:
      - path: ${CASUAL_HOME}/bin/casual-http-outbound
        arguments: [ --configuration, )" + outbound + "]");
                  }

                  auto outbound() 
                  {
                     return mockup::file::temporary::content( ".yaml", R"(
http:
  services:
    - name: casual/example/echo
      url: http://localhost:8080/casual/casual/example/echo
)");
                  }
                  
               } // configuration
            } // <unnamed>
         } // local
        


         TEST( test_domain_http, call_echo_in_other_domain)
         {
            common::unittest::Trace trace;

            auto outbound = local::configuration::outbound();

            domain::Manager a{ local::configuration::a()};
            domain::Manager b{ local::configuration::b( outbound)};

            {
               b.activate();

               auto binary = unittest::random::binary( 200);

               auto buffer = tpalloc( X_OCTET, nullptr, binary.size());
               common::algorithm::copy( binary, buffer);
               auto len = tptypes( buffer, nullptr, nullptr);

               EXPECT_TRUE( tpcall( "casual/example/echo", buffer, len, &buffer, &len, 0) == 0) << "tperrno: " << tperrnostring( tperrno);
               auto result = common::range::make( buffer, len);

               EXPECT_TRUE( algorithm::equal( binary, result));

               tpfree( buffer);
            }
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


         struct test_parallel_http : ::testing::TestWithParam< Count> 
         {

         };


         TEST_P( test_parallel_http, call_echo_in_other_domain)
         {
            common::unittest::Trace trace;

            auto count = GetParam();

            auto outbound = local::configuration::outbound();

            domain::Manager a{ local::configuration::a()};
            domain::Manager b{ local::configuration::b( outbound)};

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


         INSTANTIATE_TEST_CASE_P( 
            http,
            test_parallel_http,
            ::testing::ValuesIn( counts)
         );

      } // domain

   } // test
} // casual
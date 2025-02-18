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
#include "common/environment.h"
#include "common/process.h"

// to be able to use curl with ease
#include "administration/unittest/cli/command.h"

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
                  constexpr std::string_view a = R"(
domain:
   name: A
   transaction:
      log: ":memory:"

   executables:
      -  path: ${CASUAL_MAKE_SOURCE_ROOT}/middleware/http/inbound/bin/casual-http-inbound
         alias: casual-http-inbound
         arguments: [ -p, "${CASUAL_DOMAIN_HOME}", -c, "${CASUAL_UNITTEST_HTTP_INBOUND_CONFIG}", -e, "${CASUAL_DOMAIN_HOME}/error.log"]

   servers:
      - path: ${CASUAL_MAKE_SOURCE_ROOT}/middleware/service/bin/casual-service-manager
      - path: ${CASUAL_MAKE_SOURCE_ROOT}/middleware/example/server/bin/casual-example-server
)";
   
                  constexpr std::string_view b = R"(
domain:
   name: B
   transaction:
      log: ":memory:"

   servers:
      - path: ${CASUAL_MAKE_SOURCE_ROOT}/middleware/service/bin/casual-service-manager
      - path: ${CASUAL_MAKE_SOURCE_ROOT}/middleware/http/bin/casual-http-outbound
        arguments: [ --configuration, "${CASUAL_UNITTEST_HTTP_OUTBOUND_CONFIG}"]
)";

                  auto outbound_config_file()
                  {
                     auto file = unittest::file::temporary::content( ".yaml", R"(
http:
   services:
      -  name: casual/example/echo
         url: http://localhost:7042/casual/example/echo
)");
                     
                     common::environment::variable::set( "CASUAL_UNITTEST_HTTP_OUTBOUND_CONFIG", file.string());
                     return file;
                  }
                  
                  auto nginx_config_file()
                  {
                     auto file = unittest::file::temporary::content( ".conf", R"(
worker_processes  1;
daemon off;

pid nginx-pid-file.pid;

env CASUAL_DOMAIN_HOME;
env LD_LIBRARY_PATH;

events {
   worker_connections  1000;
}

http {
   default_type  application/octet-stream;
   underscores_in_headers on;

   sendfile        on;

   client_max_body_size 500M;

   server {
      listen       7042;
      server_name  localhost;
      access_log   access.log;


      location / {
            casual_url_prefix /;
            casual_pass;
      }
   }
})");
                     
                     common::environment::variable::set( "CASUAL_UNITTEST_HTTP_INBOUND_CONFIG", file.string());
                     return file;
                  }
                  
               } // configuration

               void block_until_inbound_ready()
               {
                  constexpr auto curl = R"(curl -s -X POST -d 'ping' -H "Content-Type: text/plain" http://localhost:7042/casual/example/echo)";

                  for( int i = 0; i < 1000; ++i)
                  {
                     auto capture = administration::unittest::cli::command::execute( curl);
                     if( capture)
                        return;
                     
                     common::process::sleep( std::chrono::milliseconds{ 10});
                  }

                  common::code::raise::error( code::casual::invalid_semantics, "failed to get a response from inbound after 1000 attempts");
               }

               void call_echo_in_other_domain( const std::string& buffer_type)
               {
                  auto outbound_guard = configuration::outbound_config_file();
                  auto nginx_guard = configuration::nginx_config_file();

                  auto a = casual::domain::unittest::manager( local::configuration::a);
                  block_until_inbound_ready();
                  auto b = casual::domain::unittest::manager( local::configuration::b);

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

                     EXPECT_TRUE( algorithm::equal( string, result));

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

         TEST( test_http, call_with_curl)
         {
            common::unittest::Trace trace;

            auto nginx_guard = local::configuration::nginx_config_file();

            auto a = casual::domain::unittest::manager( local::configuration::a);

            local::block_until_inbound_ready();

            constexpr auto curl = R"(curl -s -X POST -d '{"a": 42}' -H "Content-Type: application/json" http://localhost:7042/casual/example/echo)";

            auto capture = administration::unittest::cli::command::execute( curl);

            EXPECT_TRUE( capture.standard.out == R"({"a": 42})") << CASUAL_NAMED_VALUE( capture);
         }

         TEST( test_http, call_with_curl__unknown_content_type__expect_error_500)
         {
            common::unittest::Trace trace;

            auto nginx_guard = local::configuration::nginx_config_file();

            auto a = casual::domain::unittest::manager( local::configuration::a);

            local::block_until_inbound_ready();

            constexpr auto curl = R"(curl -sS -o /dev/stderr -w "%{response_code}" -H "Content-Type: unknown/content" -X POST -d '{"a": 42}' http://localhost:7042/casual/example/echo)";

            auto capture = administration::unittest::cli::command::execute( curl);

            // this might be a bit fragile, but we're looking for a 500 error
            EXPECT_EQ( capture.standard.out, "500") << CASUAL_NAMED_VALUE( capture);
         
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


         TEST_P( test_http_parallel, call_echo_in_other_domain__xatmi_over_http)
         {
            common::unittest::Trace trace;

            auto count = GetParam();

            auto outbound_guard = local::configuration::outbound_config_file();
            auto nginx_guard = local::configuration::nginx_config_file();

            auto a = casual::domain::unittest::manager( local::configuration::a);
            local::block_until_inbound_ready();
            auto b = casual::domain::unittest::manager( local::configuration::b);

            {
               auto binary = unittest::random::binary( count.payload);
               std::vector< int> descriptors;
               descriptors.resize( count.calls);
               
               for( auto& descriptor : descriptors)
               {
                  auto buffer = tpalloc( X_OCTET, nullptr, binary.size());
                  common::algorithm::copy( binary, std::as_writable_bytes( std::span{ buffer, binary.size()}));
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
                  auto len = tptypes( buffer, nullptr, nullptr);

                  EXPECT_TRUE( tpgetrply( &descriptor, &buffer, &len, 0) == 0) << "tperrno: " << tperrnostring( tperrno);

                  const auto result = std::span( buffer, len);
                  EXPECT_TRUE( algorithm::equal( binary, std::as_bytes( result)));

                  ::tpfree( buffer);
               }
            }
         }



         const std::vector< Count> counts{
            Count{ 100, 10},
            Count{ 100, 100},
            Count{ 1000, 100},
            // we need more workers and/or connections to handle the below
            //Count{ 10000, 100},
            //Count{ 1000, 1000},
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
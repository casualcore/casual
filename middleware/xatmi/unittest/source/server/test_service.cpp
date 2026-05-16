//! 
//! Copyright (c) 2015, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!

#define CASUAL_NO_XATMI_UNDEFINE

#include "common/unittest.h"

#include "casual/xatmi/internal/server/service.h"
#include "casual/xatmi.h"
#include "casual/xatmi/extended.h"

#include "server/service/invoke.h"

namespace casual
{
   namespace xatmi
   {

      namespace local
      {
         namespace
         {
            void service1( TPSVCINFO *) {}
            void service2( TPSVCINFO *) {}

            void service_foo( TPSVCINFO* info) 
            {
               EXPECT_TRUE( info->name == std::string{ "service_foo"}) << "info->name: " << info->name;
               
               // tpreturn
               tpreturn( TPSUCCESS, 0, nullptr, 0, 0);
            };

            auto parameter( std::string name)
            {
               casual::server::service::invoke::Parameter parameter;
               parameter.service.name = std::move( name);
               parameter.payload.type = common::buffer::type::binary;
               
               return parameter;
            }

            auto extract_header( const char* buffer)
            {
               std::vector< std::string> result;
               ::casual_header_browse( buffer, []( const char* header, void* context) -> int
               {
                  auto state = static_cast< std::vector< std::string>*>( context);
                  state->push_back( header);
                  return 0;

               }, &result);

               return result;
            }

         } // <unnamed>
      } // local



      TEST( xatmi_server_service, equality)
      {
         common::unittest::Trace trace;

         auto s1 = internal::server::service::create( ".1", &local::service1);
         auto s2 = internal::server::service::create( ".2", &local::service1);

         EXPECT_TRUE( s1 == s2);
      }

      TEST( xatmi_server_service, in_equality)
      {
         common::unittest::Trace trace;

         auto s1 = internal::server::service::create( ".1", &local::service1);
         auto s2 = internal::server::service::create( ".2", &local::service2);

         EXPECT_TRUE( s1 != s2) << trace.compose( CASUAL_NAMED_VALUE( s1), " - ", CASUAL_NAMED_VALUE( s2));
      }

      TEST( xatmi_server_service, tpsvcinfo_name_correctly_copied)
      {
         common::unittest::Trace trace;

         auto service = internal::server::service::create( "service_foo", &local::service_foo);

         service( local::parameter( "service_foo"));

      }

      TEST( xatmi_server_service, invoke)
      {
         common::unittest::Trace trace;

         auto service = []( TPSVCINFO* info)
         {
            EXPECT_TRUE( info->data != nullptr);

            std::array< char, 8 + 1> type{};
            std::array< char, 16 + 1> subtype{};
            EXPECT_TRUE( ::tptypes( info->data, type.data(), subtype.data()) == 0);
            EXPECT_TRUE( type.data() == std::string_view{ ".http"}) << CASUAL_NAMED_VALUE( type.data());
            EXPECT_TRUE( subtype.data() == std::string_view{ "body"}) << CASUAL_NAMED_VALUE( subtype.data());


            // check the header fields
            auto fields = local::extract_header( info->data);

            EXPECT_TRUE( std::ranges::contains( fields, "a:foo")) << CASUAL_NAMED_VALUE( fields);
            EXPECT_TRUE( std::ranges::contains( fields, "b:bar")) << CASUAL_NAMED_VALUE( fields);
            EXPECT_TRUE( std::ranges::contains( fields, "c:baz")) << CASUAL_NAMED_VALUE( fields);

            tpreturn( TPSUCCESS, 0, info->data, info->len, 0);
         };

         auto a = internal::server::service::create( "a", service);

         server::service::invoke::Parameter argument{
            .service = { .name = "a" },
            .payload = common::buffer::Payload{ 
               .type = std::string{ common::buffer::type::http}, 
               .data = {},
               .header = { .fields = { { { "a", "foo"}, { "b", "bar"}, { "c", "baz"}}}}
            },
         };

         auto result = a( std::move( argument));

         EXPECT_TRUE( result.code.result == decltype( result.code.result)::success);
         EXPECT_TRUE( result.payload.type == common::buffer::type::http);
         EXPECT_TRUE( result.payload.data.empty());
         EXPECT_TRUE( result.payload.header.fields.at( "a").value() == "foo") << CASUAL_NAMED_VALUE( result.payload.header);
         EXPECT_TRUE( result.payload.header.fields.at( "b").value() == "bar") << CASUAL_NAMED_VALUE( result.payload.header);
         EXPECT_TRUE( result.payload.header.fields.at( "c").value() == "baz") << CASUAL_NAMED_VALUE( result.payload.header);

      }


   } // xatmi

} // casual

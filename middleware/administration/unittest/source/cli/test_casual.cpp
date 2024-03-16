//!
//! Copyright (c) 2023, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!


//! The intention with this test suite is to verify the `casual` cli application


#include "common/unittest.h"

#include "domain/unittest/manager.h"
#include "administration/unittest/cli/command.h"

#include "common/string.h"


namespace casual
{
   using namespace common;

   using namespace std::literals;

   namespace administration
   {

      namespace local
      {
         namespace
         {

            constexpr auto configuration_base = R"(
domain: 
   groups: 
      -  name: base
      -  name: user
         dependencies: [ base]
      -  name: gateway
         dependencies: [ user]
   
   servers:
      -  path: "${CASUAL_MAKE_SOURCE_ROOT}/middleware/service/bin/casual-service-manager"
         memberships: [ base]
)";

            template< typename... C>
            auto domain( C&&... configurations)
            {
               return casual::domain::unittest::manager( configuration_base, std::forward< C>( configurations)...);
            }

         } // <unnamed>
      } // local

      TEST( cli_casual, porcelain_help)
      {
         common::unittest::Trace trace;

         auto output = administration::unittest::cli::command::execute( R"(casual --help --porcelain )").consume();

         // Arbitrary check that help talks about --header true to help user...
         EXPECT_TRUE( algorithm::search( output, "--header true"sv)) << output;
      }

      TEST( cli_casual, porcelain_explicit_header)
      {
         common::unittest::Trace trace;

         auto domain = local::domain( R"(
domain:
   name: A
)");

         {
            auto header = string::split( administration::unittest::cli::command::execute( R"(casual service --list-services --porcelain true --header true | head -n 1 )").consume(), '|');
            EXPECT_TRUE( algorithm::equal( header, array::make(  "name", "category", "mode", "timeout", "I", "C", "AT", "min", "max", "P", "PAT", "RI", "RC", "last", "contract", "V\n" ))) << CASUAL_NAMED_VALUE( header);
         }

         // if we don't explicit set --header true we don't get porcelain header
         {
            auto header = string::split( administration::unittest::cli::command::execute( R"(casual service --list-services --porcelain true | head -n 1 )").consume(), '|');
            EXPECT_TRUE( header.empty()) << CASUAL_NAMED_VALUE( header);
         }

      }


   } // administration
} // casual

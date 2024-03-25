//!
//! Copyright (c) 2023, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!

#include "common/unittest.h"

#include "domain/unittest/manager.h"

#include "administration/unittest/cli/command.h"

#include "service/unittest/utility.h"

namespace casual
{
   using namespace common;

   namespace administration
   {
      namespace local
      {
         namespace
         {
            namespace cli
            {
               constexpr auto base = R"(
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
                  return casual::domain::unittest::manager( base, std::forward< C>( configurations)...);
               }
            } // cli
         } // <unnamed>
      } // local


      TEST( cli_service, list_services)
      {
         common::unittest::Trace trace;

         auto domain = local::cli::domain( R"(
domain:
   name: test-service-cli
   services:
      -  name: some-service
         execution:
            timeout:
               duration: 90
               contract: terminate
)");

         // name
         {
            const auto capture = administration::unittest::cli::command::execute( R"(casual service --list-services --porcelain true | awk -F'|' '{printf $1}')");
            constexpr auto expected = "some-service";
            EXPECT_EQ( capture.standard.out, expected);
         }

         // timeout duration
         {
            const auto capture = administration::unittest::cli::command::execute( R"(casual service --list-services --porcelain true | awk -F'|' '{printf $4}')");
            constexpr auto expected = "90.000";
            EXPECT_EQ( capture.standard.out, expected);
         }

         // timeout contract
         {
            const auto capture = administration::unittest::cli::command::execute( R"(casual service --list-services --porcelain true | awk -F'|' '{printf $15}')");
            constexpr auto expected = "terminate";
            EXPECT_EQ( capture.standard.out, expected);
         }
      }

      TEST( cli_service, list_services__global_values)
      {
         common::unittest::Trace trace;

         auto domain = local::cli::domain( R"(
domain:
   name: test-service-cli

   global:
      service:
         execution:
            timeout:
               duration: 30
               contract: kill

   services:
      -  name: some-service
)");

         // name
         {
            const auto capture = administration::unittest::cli::command::execute( R"(casual service --list-services --porcelain true | awk -F'|' '{printf $1}')");
            constexpr auto expected = "some-service";
            EXPECT_EQ( capture.standard.out, expected);
         }

         // timeout duration
         {
            const auto capture = administration::unittest::cli::command::execute( R"(casual service --list-services --porcelain true | awk -F'|' '{printf $4}')");
            constexpr auto expected = "30.000";
            EXPECT_EQ( capture.standard.out, expected);
         }

         // timeout contract
         {
            const auto capture = administration::unittest::cli::command::execute( R"(casual service --list-services --porcelain true | awk -F'|' '{printf $15}')");
            constexpr auto expected = "kill";
            EXPECT_EQ( capture.standard.out, expected);
         }
      }

      TEST( cli_service, list_services__override_global_value)
      {
         common::unittest::Trace trace;

         auto domain = local::cli::domain( R"(
domain:
   name: test-service-cli

   global:
      service:
         execution:
            timeout:
               duration: 30
               contract: kill

   services:
      -  name: some-service
         execution:
            timeout:
               duration: 90
)");

         // name
         {
            const auto capture = administration::unittest::cli::command::execute( R"(casual service --list-services --porcelain true | awk -F'|' '{printf $1}')");
            constexpr auto expected = "some-service";
            EXPECT_EQ( capture.standard.out, expected);
         }

         // timeout duration
         {
            const auto capture = administration::unittest::cli::command::execute( R"(casual service --list-services --porcelain true | awk -F'|' '{printf $4}')");
            constexpr auto expected = "90.000";
            EXPECT_EQ( capture.standard.out, expected);
         }

         // timeout contract
         {
            const auto capture = administration::unittest::cli::command::execute( R"(casual service --list-services --porcelain true | awk -F'|' '{printf $15}')");
            constexpr auto expected = "kill";
            EXPECT_EQ( capture.standard.out, expected);
         }
      }

      TEST( cli_service, list_services__unknown_contract__expect_hyphen)
      {
         common::unittest::Trace trace;

         auto domain = local::cli::domain( R"(
domain:
   name: A
   services:
      -  name: with-contract
         execution:
            timeout:
               contract: kill
)");

         {  
            const auto capture = administration::unittest::cli::command::execute( R"(casual --color false service --list-services | grep with-contract | awk '{ print $6}' )");
            EXPECT_EQ( capture.standard.out, "kill\n");
         }

         {  
            // list admin services to check oputput in contract column for a service
            // without contract. Admin services do not have contract specified. 
            const auto capture = administration::unittest::cli::command::execute( R"(casual --color false service --list-admin-services | grep configuration/get | awk '{ print $6}' )");
            EXPECT_EQ( capture.standard.out, "-\n");
         }
      }

   } // administration
} // casual

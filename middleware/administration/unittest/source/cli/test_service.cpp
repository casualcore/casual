//!
//! Copyright (c) 2023, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!

#include "common/unittest.h"

#include "domain/unittest/manager.h"
#include "domain/unittest/discover.h"

#include "administration/unittest/cli/command.h"

#include "service/unittest/utility.h"

#include "gateway/unittest/utility.h"

#include "common/communication/instance.h"

#include <regex>

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
   name: A
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

      namespace local
      {
         namespace
         {
            void advertise_concurrent( std::vector< std::string> services, auto process, auto alias, auto description)
            {
               common::message::service::concurrent::Advertise message{ process};
               message.alias = alias;
               message.description = description;
               message.order = 1;
               message.services.add = algorithm::transform( services, []( auto& name)
               {
                  return common::message::service::concurrent::advertise::Service{ 
                     name, "", common::service::transaction::Type::automatic, common::service::visibility::Type::discoverable};

               });
               communication::device::blocking::send( communication::instance::outbound::service::manager::device(), message);
            }
            
         } // <unnamed>
      } // local

      TEST( cli_service, list_instances_one_remote)
      {
         common::unittest::Trace trace;

         auto domain = local::cli::domain();

         // advertise a bunch of services
         local::advertise_concurrent( { "d", "b", "c", "a"}, process::handle(), "test", "domain-X");
         
         {  
            const auto capture = administration::unittest::cli::command::execute( R"(casual --color false service --list-instances | grep remote )");
            auto rows = string::split( capture.standard.out, '\n');
            ASSERT_TRUE( rows.size() == 4);

            EXPECT_TRUE( std::regex_match( rows.at( 0), std::regex{ R"(^a .* remote .* test .* domain-X.*)"})) << CASUAL_NAMED_VALUE( rows);
            EXPECT_TRUE( std::regex_match( rows.at( 1), std::regex{ R"(^b .* remote .* test .* domain-X.*)"})) << CASUAL_NAMED_VALUE( rows);
            EXPECT_TRUE( std::regex_match( rows.at( 2), std::regex{ R"(^c .* remote .* test .* domain-X.*)"})) << CASUAL_NAMED_VALUE( rows);
            EXPECT_TRUE( std::regex_match( rows.at( 3), std::regex{ R"(^d .* remote .* test .* domain-X.*)"})) << CASUAL_NAMED_VALUE( rows);
         }
      }


      TEST( cli_service, list_instances_one_remote_porcelain)
      {
         common::unittest::Trace trace;

         auto domain = local::cli::domain();

         // advertise a bunch of services
         local::advertise_concurrent( { "c", "b", "a", "d"}, process::handle(), "test", "domain-X");
         
         {  
            const auto capture = administration::unittest::cli::command::execute( R"(casual --porcelain true service --list-instances | grep '|remote|')");
            auto rows = string::split( capture.standard.out, '\n');
            ASSERT_TRUE( rows.size() == 4);

            EXPECT_TRUE( std::regex_match( rows.at( 0), std::regex{ R"(a\|[0-9]+\|remote\|[0-9]+\|test\|domain-X)"}));
            EXPECT_TRUE( std::regex_match( rows.at( 1), std::regex{ R"(b\|[0-9]+\|remote\|[0-9]+\|test\|domain-X)"}));
            EXPECT_TRUE( std::regex_match( rows.at( 2), std::regex{ R"(c\|[0-9]+\|remote\|[0-9]+\|test\|domain-X)"}));
            EXPECT_TRUE( std::regex_match( rows.at( 3), std::regex{ R"(d\|[0-9]+\|remote\|[0-9]+\|test\|domain-X)"}));
         }
      }

      TEST( cli_service, list_instances_several_remote)
      {
         common::unittest::Trace trace;

         auto domain = local::cli::domain();

         auto a = process::Handle{ process::id(), strong::ipc::id::generate()};
         auto b = process::Handle{ process::id(), strong::ipc::id::generate()};
         auto c = process::Handle{ process::id(), strong::ipc::id::generate()};
         auto d = process::Handle{ process::id(), strong::ipc::id::generate()};
         auto e = process::Handle{ process::id(), strong::ipc::id::generate()};

         // advertise a bunch of services for the different remotes
         local::advertise_concurrent( { "s1", "s2"}, a, "test", "domain-a");
         local::advertise_concurrent( { "s1", "s2"}, b, "test", "domain-b");
         local::advertise_concurrent( { "s1", "s2", "s3"}, c, "test", "domain-c");
         local::advertise_concurrent( { "s1", "s3", "s4"}, d, "test", "domain-d");
         local::advertise_concurrent( { "s1", "s2", "s3", "s4", "s5"}, e, "test", "domain-e");

         auto count_matches = []( const std::string& value, auto regex)
         {
            auto begin = std::sregex_iterator( std::begin( value), std::end( value), regex);
            return std::distance( begin, std::sregex_iterator());
         };
         
         const auto capture = administration::unittest::cli::command::execute( R"(casual --porcelain true service --list-instances | grep 'remote')");

         EXPECT_TRUE( count_matches( capture.standard.out, std::regex{ "s1"}) == 5);
         EXPECT_TRUE( count_matches( capture.standard.out, std::regex{ "s2"}) == 4);
         EXPECT_TRUE( count_matches( capture.standard.out, std::regex{ "s3"}) == 3);
         EXPECT_TRUE( count_matches( capture.standard.out, std::regex{ "s4"}) == 2);
         EXPECT_TRUE( count_matches( capture.standard.out, std::regex{ "s5"}) == 1);
      }


      TEST( cli_service, list_instances_two_domains)
      {
         common::unittest::Trace trace;

         constexpr auto base = R"(
domain:
   servers:
      - path: "${CASUAL_MAKE_SOURCE_ROOT}/middleware/transaction/bin/casual-transaction-manager"
        memberships: [ base]
      - path: "${CASUAL_MAKE_SOURCE_ROOT}/middleware/gateway/bin/casual-gateway-manager"
        memberships: [ gateway]
)";

         auto b = local::cli::domain( base, R"(
domain: 
   name: B
   gateway:
      inbound:
         groups:
            -  connections:
               -  address: 127.0.0.1:7001
)");

         // this unittest advertise
         casual::service::unittest::advertise( { "a", "b", "c"});

         auto a = local::cli::domain( base, R"(
domain: 
   name: A
   gateway:
      outbound:
         groups:
            -  alias: out
               connections:
               -  address: 127.0.0.1:7001
)");

         gateway::unittest::fetch::until( gateway::unittest::fetch::predicate::outbound::connected());

         casual::domain::unittest::service::discover( { "a", "b", "c"});

         const auto capture = administration::unittest::cli::command::execute( R"(casual --porcelain true service --list-instances | grep 'remote')");
         auto rows = string::split( capture.standard.out, '\n');

         EXPECT_TRUE( std::regex_match( rows.at( 0), std::regex{ R"(a\|[0-9]+\|remote\|[0-9]+\|out\|B)"}));
         EXPECT_TRUE( std::regex_match( rows.at( 1), std::regex{ R"(b\|[0-9]+\|remote\|[0-9]+\|out\|B)"}));
         EXPECT_TRUE( std::regex_match( rows.at( 2), std::regex{ R"(c\|[0-9]+\|remote\|[0-9]+\|out\|B)"}));
      }


   } // administration
} // casual

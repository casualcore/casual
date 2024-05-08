//!
//! Copyright (c) 2022, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!

#include "common/unittest.h"
#include "common/string.h"
#include "common/serialize/create.h"

#include "administration/unittest/cli/command.h"

#include "domain/unittest/manager.h"

#include "configuration/model/transform.h"
#include "configuration/unittest/utility.h"





namespace casual
{
   using namespace common;

   namespace administration
   {
      namespace local
      {
         namespace
         {
            namespace configuration
            {
               constexpr auto base = R"(
domain:
   groups: 
      -  name: base
      -  name: queue
         dependencies: [ base]
      -  name: user
         dependencies: [ queue]
      -  name: gateway
         dependencies: [ user]
   
   servers:
      - path: "${CASUAL_MAKE_SOURCE_ROOT}/middleware/service/bin/casual-service-manager"
        memberships: [ base]
      - path: "${CASUAL_MAKE_SOURCE_ROOT}/middleware/transaction/bin/casual-transaction-manager"
        memberships: [ base]
)";
            } // configuration

            template< typename... C>
            auto domain( C&&... configurations)
            {
               return casual::domain::unittest::manager( configuration::base, std::forward< C>( configurations)...);
            }

            auto aggregate_configuration( auto... configuration) -> file::scoped::Path
            {
               auto model = casual::configuration::unittest::load( configuration...);
               auto user = casual::configuration::model::transform( model);
               auto writer = casual::common::serialize::create::writer::from( "yaml");
               writer << user;

               return common::unittest::file::temporary::content( ".yaml", writer.consume< std::string>());
            }

         } // <unnamed>      

      } // local

      TEST( cli_configuration_runtime, post)
      {
         common::unittest::Trace trace;

         // empty domain... (SM, TM)
         auto a = local::domain();

         auto file = local::aggregate_configuration( local::configuration::base, R"(
domain:
   name: A
   servers:
      -  alias: xxx
         path: "${CASUAL_MAKE_SOURCE_ROOT}/middleware/example/server/bin/casual-example-server"
         memberships: [ user]
         instances: 1
)");

         {
            auto capture = administration::unittest::cli::command::execute( "cat ", file , " | casual configuration --post yaml");
            EXPECT_TRUE( capture.exit == 0);
         }

         {
            auto capture = administration::unittest::cli::command::execute( "casual --porcelain true domain --list-servers | grep casual-example-server");
            auto alias = string::split( capture.standard.out, '|').at( 0);
            EXPECT_TRUE( alias == "xxx") << CASUAL_NAMED_VALUE( capture);
         }
      }

      TEST( cli_configuration_runtime, put)
      {
         common::unittest::Trace trace;

         // empty domain... (SM, TM)
         auto a = local::domain();

         auto file = common::unittest::file::temporary::content( ".yaml", R"(
domain:
   name: A
   servers:
      -  alias: xxx
         path: "${CASUAL_MAKE_SOURCE_ROOT}/middleware/example/server/bin/casual-example-server"
         memberships: [ user]
         instances: 1
)");

         {
            auto capture = administration::unittest::cli::command::execute( "cat ", file , " | casual configuration --put yaml");
            EXPECT_TRUE( capture.exit == 0);
         }

         {
            auto capture = administration::unittest::cli::command::execute( "casual --porcelain true domain --list-servers | grep casual-example-server");
            auto alias = string::split( capture.standard.out, '|').at( 0);
            EXPECT_TRUE( alias == "xxx") << CASUAL_NAMED_VALUE( capture);
         }

      }


   } // administration
} // casual
//!
//! Copyright (c) 2022, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!

#include "common/unittest.h"

#include "administration/unittest/cli/command.h"

#include "domain/unittest/manager.h"
#include "domain/unittest/utility.h"

#include "common/unittest/file.h"

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
   name: internal
   groups: 
      -  name: base
      -  name: user
         dependencies: [ base]
   
   servers:
      - path: "${CASUAL_MAKE_SOURCE_ROOT}/middleware/service/bin/casual-service-manager"
        memberships: [ base]

)";
            } // configuration

            template< typename... C>
            auto domain( C&&... configurations)
            {
               return casual::domain::unittest::manager( configuration::base, std::forward< C>( configurations)...);
            }
         } // <unnamed>

      } // local

/*
      TEST( cli_internal, log_relocation__expect_example_server_to_log_elsewhere)
      {
         common::unittest::Trace trace;
         
         auto domain = local::domain( R"(
domain:
   servers:
      -  alias: example-server
         path: "${CASUAL_MAKE_SOURCE_ROOT}/middleware/example/server/bin/casual-example-server"
         memberships: [ user]
         instances: 1
)");

         auto state = casual::domain::unittest::fetch::until( casual::domain::unittest::fetch::predicate::alias::has::instances( "example-server", 1));

         auto example = algorithm::find( state.servers, "example-server").at( 0).instances.at( 0).handle;

         auto scoped_path = common::unittest::file::temporary::name( ".log");

         administration::unittest::cli::command::execute( "casual internal --log-path ", scoped_path, ' ', example.pid);

         EXPECT_TRUE( ! common::unittest::file::fetch::until::content( scoped_path).empty());

      }
      */
   } // administration
   
} // casual
//!
//! Copyright (c) 2026, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!

#include "common/unittest.h"

#include "domain/unittest/discover.h"
#include "domain/discovery/api.h"
#include "domain/unittest/manager.h"


namespace casual
{
   namespace domain::discovery
   {
      namespace local
      {
         namespace
         {
            namespace configuration
            {
               constexpr auto base = R"(
domain:   
   servers:
      - path: "${CASUAL_MAKE_SOURCE_ROOT}/middleware/service/bin/casual-service-manager"
)";
               
            } // configuration

            template< typename... C>
            auto domain( C&&... configurations)
            {
               return casual::domain::unittest::manager( configuration::base, std::forward< C>( configurations)...);
            }
            
         } // <unnamed>
      } // local

      TEST( test_discovery, register_as_provider__unregister___expect_no_providers)
      {
         common::unittest::Trace trace;

         auto domain = local::domain();

         // we register our self
         discovery::provider::registration( discovery::provider::Ability::discover);

         {
            auto state = casual::domain::unittest::discover::state();
            EXPECT_TRUE( common::algorithm::contains( state.providers, common::process::handle()));
         }

         // we 'unregister' our self
         discovery::provider::registration( discovery::provider::Ability::absent);

         // expect we're no longer a provider
         {
            auto state = casual::domain::unittest::discover::state();
            EXPECT_TRUE( ! common::algorithm::contains( state.providers, common::process::handle()));
         }
      }
      
   } // domain::discovery
   
} // casual

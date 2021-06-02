//!
//! Copyright (c) 2021, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!

#include "common/unittest.h"
#include "common/unittest/file.h"

#include "domain/manager/unittest/process.h"
#include "domain/manager/unittest/configuration.h"

#include "configuration/model.h"
#include "configuration/model/load.h"
#include "configuration/model/transform.h"

#include "common/environment.h"

namespace casual
{
   using namespace common;

   namespace test::domain::configuration
   {
      namespace local
      {
         namespace
         {
            namespace configuration
            {
               constexpr auto servers = R"(
domain:
   servers:
      - path: "${CASUAL_MAKE_SOURCE_ROOT}/middleware/service/bin/casual-service-manager"
        memberships: [ .casual.master]
      - path: "${CASUAL_MAKE_SOURCE_ROOT}/middleware/transaction/bin/casual-transaction-manager"
        memberships: [ .casual.transaction]
      - path: "${CASUAL_MAKE_SOURCE_ROOT}/middleware/queue/bin/casual-queue-manager"
        memberships: [ .casual.queue]
      - path: "${CASUAL_MAKE_SOURCE_ROOT}/middleware/gateway/bin/casual-gateway-manager"
        memberships: [ .casual.gateway]
)";



               template< typename... C>
               auto load( C&&... contents)
               {
                  auto files = common::unittest::file::temporary::contents( ".yaml", std::forward< C>( contents)...);

                  auto get_path = []( auto& file){ return static_cast< std::filesystem::path>( file);};

                  return casual::configuration::model::load( common::algorithm::transform( files, get_path));
               }
            } // configuration

            template< typename... C>
            auto domain( C&&... configurations)
            {
               return casual::domain::manager::unittest::process( local::configuration::servers, std::forward< C>( configurations)...);
            }

            namespace validate
            {
               template< typename... C>
               void configuration( C&&... configurations)
               {
                  constexpr auto resources = R"(
resources:

  - key: rm-mockup   
    server: "${CASUAL_MAKE_SOURCE_ROOT}/middleware/transaction/bin/rm-proxy-casual-mockup"
    xa_struct_name: casual_mockup_xa_switch_static
    libraries:
       - casual-mockup-rm
)";
                  // create the "resource property file"
                  static const auto resource_file = common::unittest::file::temporary::content( ".yaml", resources);

                  common::environment::variable::set( "CASUAL_RESOURCE_CONFIGURATION_FILE", resource_file.string());

                  auto domain = local::domain( std::forward< C>( configurations)...);

                  auto origin = local::configuration::load( std::forward< C>( configurations)...);

                  auto model = casual::configuration::model::transform( casual::domain::manager::unittest::configuration::get());

                  EXPECT_TRUE( origin.domain == model.domain) << CASUAL_NAMED_VALUE( origin.domain) << "\n " << CASUAL_NAMED_VALUE( model.domain);
                  EXPECT_TRUE( origin.service == model.service) << CASUAL_NAMED_VALUE( origin.service) << "\n " << CASUAL_NAMED_VALUE( model.service);
                  EXPECT_TRUE( origin.transaction == model.transaction) << CASUAL_NAMED_VALUE( origin.transaction) << "\n " << CASUAL_NAMED_VALUE( model.transaction);
                  EXPECT_TRUE( origin.queue == model.queue) << CASUAL_NAMED_VALUE( origin.queue) << "\n " << CASUAL_NAMED_VALUE( model.queue);
                  EXPECT_TRUE( origin.gateway == model.gateway) << CASUAL_NAMED_VALUE( origin.gateway) << "\n " << CASUAL_NAMED_VALUE( model.gateway);
                  EXPECT_TRUE( origin == model) << CASUAL_NAMED_VALUE( origin) << "\n " << CASUAL_NAMED_VALUE( model);
               }
               
            } // validate

         } // <unnamed>
      } // local
      

      TEST( test_domain_configuration, base_configuration)
      {
         unittest::Trace trace;

         constexpr auto configuration = R"(
domain:
   name: base
   transaction:
      log: ":memory:"
)";

         local::validate::configuration( configuration);
      }

      TEST( test_domain_configuration, full_configuration)
      {
         unittest::Trace trace;

         constexpr auto configuration = R"(
domain:
   name: full
   transaction:
      log: ":memory:"
      resources:
         - key: rm-mockup
           name: rm1
           instances: 2
         - key: rm-mockup
           name: rm2
           instances: 2
           openinfo: openinfo2

   groups:
      -  name: A
      -  name: B
         dependencies: [ A]

   servers:
      -  path: "${CASUAL_MAKE_SOURCE_ROOT}/middleware/example/server/bin/casual-example-server"
         alias: example-server
         memberships: [ B]
         note: x

   services:
      -  name: casual/example/echo
         routes: [ echo]
         execution:
            timeout:
               duration: 500ms
               contract: kill

   queue:
      note: some note...
      default:
         queue: 
            retry:
               count: 3

      groups:
         - name: A
           queuebase: ":memory:"
           queues:
            -  name: a1
               note: some note...
            -  name: a2
            -  name: a3
            -  name: delayed_100ms
               retry: { count: 10, delay: 100ms}
         - name: B
           queuebase: ":memory:"
           queues:
            - name: b1
            - name: b2
            - name: b3

   gateway:
      reverse:
         inbound:
            groups:
               -  alias: ri
                  note: x
                  connections:
                     -  address: 127.0.0.1:7001
                        note: x
         outbound:
            groups:
               -  alias: ro
                  note: z
                  connections:
                     -  address: 127.0.0.1:7002
                        services: [ a, b, c]
                        queues: [ a, b, c]
                        note: z
      inbound:
         groups:
            -  alias: ri
               note: x
               connections:
                  -  address: 127.0.0.1:7003
                     note: x
      outbound:
         groups:
            -  alias: ro
               note: z
               connections:
                  -  address: 127.0.0.1:7004
                     services: [ a, b, c]
                     queues: [ a, b, c]
                     note: z

            

         
)"; 

         local::validate::configuration( configuration);
      }
      
   } // test::domain::configuration
} // casual
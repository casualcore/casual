//! 
//! Copyright (c) 2015, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!


#include "common/unittest.h"
#include "common/unittest/eventually/send.h"

#include "domain/manager/unittest/process.h"

#include "casual/xatmi/server.h"

#include "common/functional.h"

namespace casual
{
   using namespace common;

   namespace test
   {
      namespace local
      {
         namespace
         {
            namespace global
            {
               bool init = false;
               bool done = false;
            } // global

            int server_init( int argc, char** argv)
            {
               global::init = true;
               return 0;
            }

            void server_done()
            {
               global::done = true;
            }

            auto domain()
            {
               return casual::domain::manager::unittest::Process{{
R"(
domain:
   name: test-default-domain

   groups: 
      - name: base
      - name: transaction
        dependencies: [ base]
      - name: queue
        dependencies: [ transaction]
      - name: example
        dependencies: [ queue]

   servers:
      - path: ${CASUAL_HOME}/bin/casual-service-manager
        memberships: [ base]
      - path: ${CASUAL_HOME}/bin/casual-transaction-manager
        memberships: [ transaction]
      - path: ${CASUAL_HOME}/bin/casual-queue-manager
        memberships: [ queue]
      - path: ${CASUAL_HOME}/bin/casual-example-error-server
        memberships: [ example]
      - path: ${CASUAL_HOME}/bin/casual-example-server
        memberships: [ example]
)"
               }};
            }

         } // <unnamed>
      } // local

      TEST( casual_xatmi_server_start, initialize_and_done_is_called)
      {
         common::unittest::Trace trace;
         
         auto domain = local::domain();

         local::global::init = false;
         local::global::done = false;

         // prepare the shutdown, since we block in casual_run_server
         unittest::eventually::send( process::handle().ipc, common::message::shutdown::Request{ process::handle()});
            
         casual_service_name_mapping service_mapping[] = { { 0, 0, 0, 0}};
         casual_xa_switch_map xa_mapping[] = {{ 0, 0, 0}};
         casual_server_arguments arguments{
            service_mapping,
            &local::server_init,
            &local::server_done,
            0,
            nullptr,
            xa_mapping
         };

         casual_run_server( &arguments);
         
         EXPECT_TRUE( local::global::init);
         EXPECT_TRUE( local::global::done);
      }

   


   } // test

} // casual

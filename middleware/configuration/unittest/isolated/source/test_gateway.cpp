//!
//! casual 
//!

#include <gtest/gtest.h>

#include "config/gateway.h"

#include "common/mockup/file.h"
#include "sf/log.h"



namespace casual
{
   namespace config
   {
      namespace local
      {
         namespace
         {
            common::file::scoped::Path example_1()
            {
               return common::mockup::file::temporary( ".yaml", R"(
gateway:
  
  address: 127.0.0.1:7777

  connections:
    
    - name: some_name
      type: tcp
      address: dolittle.laz.se:3432
      
      #
      # Services we "know" exists in the remote domain
      #
      services:
        - service1
        - service2
        - service3
        - service4
      
    - type: "ipc"
      address: "/Users/Lazan/casual/domain2"
)"
               );
            }

         } // <unnamed>
      } // local

      TEST( casual_configuration_gateway, expect_two_connections)
      {
         auto temp_file = local::example_1();
         auto gateway = config::gateway::get( temp_file);

         EXPECT_TRUE( gateway.address == "127.0.0.1:7777") << CASUAL_MAKE_NVP( gateway.address);
         EXPECT_TRUE( gateway.connections.size() == 2) << CASUAL_MAKE_NVP( gateway);

      }

      TEST( casual_configuration_gateway, expect_4_services_for_connection_1)
      {
         auto temp_file = local::example_1();
         auto gateway = config::gateway::get( temp_file);

         ASSERT_TRUE( gateway.connections.size() == 2);
         ASSERT_TRUE( gateway.connections.at( 0).type == "tcp");
         ASSERT_TRUE( gateway.connections.at( 0).address == "dolittle.laz.se:3432");
         ASSERT_TRUE( gateway.connections.at( 0).services.size() == 4);
      }


   } // gateway
} // casual

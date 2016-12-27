//!
//! casual 
//!

#include <gtest/gtest.h>
#include "configuration/gateway.h"

#include "common/mockup/file.h"
#include "sf/log.h"
#include "sf/archive/maker.h"



namespace casual
{
   namespace configuration
   {
      namespace local
      {
         namespace
         {

            gateway::Gateway get( const std::string& file)
            {

               //
               // Create the reader and deserialize configuration
               //
               auto reader = sf::archive::reader::from::file( file);

               gateway::Gateway gateway;
               reader >> CASUAL_MAKE_NVP( gateway);

               gateway.finalize();

               return gateway;
            }



            common::file::scoped::Path example_1()
            {
               return common::mockup::file::temporary( ".yaml", R"(
gateway:
  
  listeners:
    - address: 127.0.0.1:7777

  connections:
    
    - name: some_name
      # type: tcp  # tcp is default
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


            common::file::scoped::Path example_2()
            {
               return common::mockup::file::temporary( ".yaml", R"(
gateway:

  default:
    listener: 
       address: 127.0.0.1:7777
     
    connection:
       type: ipc
       address: dolittle.laz.se:3432
       restart: true


  listeners:
    - address: 127.0.0.1:7777

  connections:
    
    - name: some_name
      type: tcp
      
    - address: "/some/path/to/domain"
)"
               );
            }

         } // <unnamed>
      } // local

      TEST( casual_configuration_gateway, expect_two_connections)
      {
         auto temp_file = local::example_1();
         auto gateway = local::get( temp_file);

         EXPECT_TRUE( gateway.listeners.at( 0).address == "127.0.0.1:7777") << CASUAL_MAKE_NVP( gateway.listeners.at( 0).address);
         EXPECT_TRUE( gateway.connections.size() == 2) << CASUAL_MAKE_NVP( gateway);

      }

      TEST( casual_configuration_gateway, expect_4_services_for_connection_1)
      {
         auto temp_file = local::example_1();
         auto gateway = local::get( temp_file);

         ASSERT_TRUE( gateway.connections.size() == 2);
         ASSERT_TRUE( gateway.connections.at( 0).type == "tcp");
         ASSERT_TRUE( gateway.connections.at( 0).address == "dolittle.laz.se:3432");
         ASSERT_TRUE( gateway.connections.at( 0).services.size() == 4);
      }

      TEST( casual_configuration_gateway, example_2__expect_default_to_be_set)
      {
         auto temp_file = local::example_2();
         auto gateway = local::get( temp_file);

         EXPECT_TRUE( gateway.listeners.at( 0).address == "127.0.0.1:7777") << CASUAL_MAKE_NVP( gateway.listeners.at( 0).address);

         EXPECT_TRUE( gateway.connections.at( 0).address == "dolittle.laz.se:3432") << CASUAL_MAKE_NVP( gateway);
         EXPECT_TRUE( gateway.connections.at( 0).restart == "true") << CASUAL_MAKE_NVP( gateway);
         EXPECT_TRUE( gateway.connections.at( 0).type == "tcp") << CASUAL_MAKE_NVP( gateway);
         EXPECT_TRUE( gateway.connections.at( 0).services.empty()) << CASUAL_MAKE_NVP( gateway);

         EXPECT_TRUE( gateway.connections.at( 1).address == "/some/path/to/domain") << CASUAL_MAKE_NVP( gateway);
         EXPECT_TRUE( gateway.connections.at( 1).restart == "true") << CASUAL_MAKE_NVP( gateway);
         EXPECT_TRUE( gateway.connections.at( 1).type == "ipc") << CASUAL_MAKE_NVP( gateway);
         EXPECT_TRUE( gateway.connections.at( 1).services.empty()) << CASUAL_MAKE_NVP( gateway);


      }


   } // gateway
} // casual

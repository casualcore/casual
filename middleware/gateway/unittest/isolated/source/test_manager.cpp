//!
//! test_manager.cpp
//!
//! Created on: Nov 8, 2015
//!     Author: Lazan
//!

#include <gtest/gtest.h>


#include "common/mockup/file.h"
#include "common/mockup/process.h"
#include "common/mockup/domain.h"

#include "common/environment.h"
#include "common/trace.h"

namespace casual
{
   using namespace common;
   namespace gateway
   {

      namespace local
      {
         namespace
         {
            struct Gateway
            {
               Gateway( const std::string& configuration)
                : file{ mockup::file::temporary( ".yaml", configuration)},
                  process{ "./bin/casual-gateway-manager", {
                     "--configuration", file,
                  }}
               {

               }

               struct prepare_environment_type
               {
                  prepare_environment_type()
                  {
                     common::environment::variable::set(
                           common::environment::variable::name::home(),
                           directory::name::base( __FILE__) + "../../../" );
                  }


               } prepare_environment;

               file::scoped::Path file;
               mockup::Process process;
            };

            struct Domain
            {
               Domain( const std::string& configuration) : gateway{ configuration}
               {

               }


               common::mockup::domain::Domain domain;
               Gateway gateway;
            };



            std::string empty_configuration()
            {
               return R"yaml(
gateway:
  
  listeners:

  connections:

)yaml";

            }


            std::string one_connector_configuration()
            {
               return R"yaml(
gateway:
  
  listeners:

  connections:
    - type: "ipc"
      address: "${CASUAL_UNITTEST_IPC_PATH}"

)yaml";

            }

         } // <unnamed>
      } // local



      TEST( casual_gateway_manager, empty_configuration)
      {
         Trace trace{ "TEST( casual_gateway_manager, empty_configuration)"};


         local::Domain domain{ local::empty_configuration()};

         common::signal::timer::Scoped timer{ std::chrono::milliseconds{ 100}};

         EXPECT_TRUE( process::ping( domain.gateway.process.handle().queue) == domain.gateway.process.handle());
      }

      TEST( casual_gateway_manager, ipc_non_existent_path__configuration)
      {
         Trace trace{ "TEST( casual_gateway_manager, ipc_non_existent_path__configuration)"};

         environment::variable::set( "CASUAL_UNITTEST_IPC_PATH", "/non/existent/path");

         local::Domain domain{ local::one_connector_configuration()};

         common::signal::timer::Scoped timer{ std::chrono::milliseconds{ 100}};

         EXPECT_TRUE( process::ping( domain.gateway.process.handle().queue) == domain.gateway.process.handle());
      }


      TEST( casual_gateway_manager, ipc_same_path_as_unittest_domain__configuration___expect_connection)
      {
         Trace trace{ "TEST( casual_gateway_manager, ipc_same_path_as_unittest_domain__configuration___expect_connection)"};

         environment::variable::set( "CASUAL_UNITTEST_IPC_PATH", environment::domain::singleton::path());

         local::Domain domain{ local::one_connector_configuration()};

         common::process::sleep( std::chrono::milliseconds{ 200});

         common::signal::timer::Scoped timer{ std::chrono::milliseconds{ 100}};

         EXPECT_TRUE( process::ping( domain.gateway.process.handle().queue) == domain.gateway.process.handle());
      }


   } // gateway

} // casual

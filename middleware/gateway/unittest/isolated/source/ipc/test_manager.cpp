//!
//! test_manager.cpp
//!
//! Created on: Nov 8, 2015
//!     Author: Lazan
//!

#include <gtest/gtest.h>
#include "common/unittest.h"

#include "gateway/manager/admin/vo.h"

#include "common/mockup/file.h"
#include "common/mockup/process.h"
#include "common/mockup/domain.h"

#include "common/environment.h"
#include "common/trace.h"

#include "sf/xatmi_call.h"
#include "sf/log.h"

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


               file::scoped::Path file;
               mockup::Process process;
            };

            struct Domain
            {
               Domain( const std::string& configuration) : gateway{ configuration}
               {

               }

               struct set_environment_t
               {
                  set_environment_t()
                  {
                     environment::variable::set( environment::variable::name::home(), "./" );
                  }
               } set_environment;

               mockup::domain::Manager manager;
               mockup::domain::Broker broker;
               mockup::domain::transaction::Manager tm;

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


            namespace call
            {

               manager::admin::vo::State state()
               {
                  sf::xatmi::service::binary::Sync service( ".casual.gateway.state");

                  auto reply = service();

                  manager::admin::vo::State serviceReply;

                  reply >> CASUAL_MAKE_NVP( serviceReply);

                  return serviceReply;
               }


               namespace wait
               {
                  namespace ready
                  {

                     bool manager_ready( const manager::admin::vo::State& state)
                     {
                        if( state.connections.outbound.empty())
                           return false;


                        return range::any_of( state.connections.outbound, []( const manager::admin::vo::outbound::Connection& c){
                           return c.runlevel >= manager::admin::vo::outbound::Connection::Runlevel::online;
                        });
                     }

                     manager::admin::vo::State state()
                     {
                        auto state = local::call::state();

                        while( ! manager_ready( state))
                        {
                           common::process::sleep( std::chrono::milliseconds{ 5});
                           state = local::call::state();
                        }

                        return state;
                     }

                  } // ready

               } // wait

            } // call



         } // <unnamed>
      } // local



      TEST( casual_gateway_manager, empty_configuration)
      {
         CASUAL_UNITTEST_TRACE();

         local::Domain domain{ local::empty_configuration()};

         common::signal::timer::Scoped timer{ std::chrono::milliseconds{ 100}};

         EXPECT_TRUE( process::ping( domain.gateway.process.handle().queue) == domain.gateway.process.handle());
      }

      TEST( casual_gateway_manager, ipc_non_existent_path__configuration)
      {
         CASUAL_UNITTEST_TRACE();

         environment::variable::set( "CASUAL_UNITTEST_IPC_PATH", "/non/existent/path");

         local::Domain domain{ local::one_connector_configuration()};

         common::signal::timer::Scoped timer{ std::chrono::milliseconds{ 100}};

         EXPECT_TRUE( process::ping( domain.gateway.process.handle().queue) == domain.gateway.process.handle());
      }


      TEST( casual_gateway_manager, ipc_same_path_as_unittest_domain__configuration___expect_connection)
      {
         CASUAL_UNITTEST_TRACE();

         environment::variable::set( "CASUAL_UNITTEST_IPC_PATH", environment::domain::singleton::path());

         local::Domain domain{ local::one_connector_configuration()};

         common::signal::timer::Scoped timer{ std::chrono::milliseconds{ 100}};

         EXPECT_TRUE( process::ping( domain.gateway.process.handle().queue) == domain.gateway.process.handle());
      }

      TEST( casual_gateway_manager, ipc_same_path_as_unittest_domain__call_state___expect_1_outbound_and_1_inbound_connection)
      {
         CASUAL_UNITTEST_TRACE();

         environment::variable::set( "CASUAL_UNITTEST_IPC_PATH", environment::domain::singleton::path());

         local::Domain domain{ local::one_connector_configuration()};

         common::signal::timer::Scoped timer{ std::chrono::milliseconds{ 100}};

         //
         // We ping it so we know the gateway is up'n running
         //
         EXPECT_TRUE( process::ping( domain.gateway.process.handle().queue) == domain.gateway.process.handle());

         auto state = local::call::wait::ready::state();

         ASSERT_TRUE( state.connections.outbound.size() == 1) << CASUAL_MAKE_NVP( state);
         auto& outbound = state.connections.outbound.at( 0);
         EXPECT_TRUE( outbound.runlevel == manager::admin::vo::outbound::Connection::Runlevel::online) << CASUAL_MAKE_NVP( state);
         EXPECT_TRUE( outbound.type == manager::admin::vo::outbound::Connection::Type::ipc);
         ASSERT_TRUE( state.connections.inbound.size() == 1);
         auto& inbound = state.connections.inbound.at( 0);
         EXPECT_TRUE( inbound.runlevel == manager::admin::vo::inbound::Connection::Runlevel::online) << CASUAL_MAKE_NVP( state);
         EXPECT_TRUE( inbound.type == manager::admin::vo::inbound::Connection::Type::ipc);
      }

      TEST( casual_gateway_manager, ipc_same_path_as_unittest_domain__call_outbound____expect_call_to_service)
      {
         CASUAL_UNITTEST_TRACE();

         environment::variable::set( "CASUAL_UNITTEST_IPC_PATH", environment::domain::singleton::path());

         local::Domain domain{ local::one_connector_configuration()};

         common::signal::timer::Scoped timer{ std::chrono::milliseconds{ 100}};

         auto get_outbound_queue = []( local::Domain& domain)
               {
                  //
                  // We ping it so we know the gateway is up'n running
                  //
                  EXPECT_TRUE( process::ping( domain.gateway.process.handle().queue) == domain.gateway.process.handle());

                  auto state = local::call::wait::ready::state();

                  return state.connections.outbound.at( 0).process.queue;
               };


         auto outbound = get_outbound_queue( domain);


         //platform::binary_type paylaod{ 1, 2, 3, 4, 5, 6, 7, 8, 9};

         common::message::service::call::callee::Request request;
         {
            request.service.name = ".casual.gateway.state";
            request.process = process::handle();
         }

         auto correlation = communication::ipc::blocking::send( outbound, request);


         //
         // Get the reply from the outbound
         //
         {
            common::message::service::call::Reply reply;
            communication::ipc::blocking::receive( communication::ipc::inbound::device(), reply);

            EXPECT_TRUE( reply.correlation == correlation);
         }

      }


   } // gateway

} // casual

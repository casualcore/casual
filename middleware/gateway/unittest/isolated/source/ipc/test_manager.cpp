//!
//! casual
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

domain:
  gateway:
  
    listeners:

    connections:

)yaml";

            }


            std::string one_connector_configuration()
            {
               return R"yaml(
domain:
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
                        if( state.connections.empty())
                           return false;

                        return range::any_of( state.connections, []( const manager::admin::vo::Connection& c){
                           return c.runlevel >= manager::admin::vo::Connection::Runlevel::online;
                        });
                     }

                     manager::admin::vo::State state()
                     {
                        auto state = local::call::state();

                        auto count = 100;

                        while( ! manager_ready( state) && count-- > 0)
                        {
                           common::process::sleep( std::chrono::milliseconds{ 10});
                           state = local::call::state();
                        }

                        return state;
                     }

                  } // ready

               } // wait

            } // call



         } // <unnamed>
      } // local



      TEST( casual_gateway_manager_ipc, empty_configuration)
      {
         common::unittest::Trace trace;

         local::Domain domain{ local::empty_configuration()};

         common::signal::timer::Scoped timer{ std::chrono::milliseconds{ 100}};

         EXPECT_TRUE( process::ping( domain.gateway.process.handle().queue) == domain.gateway.process.handle());
      }

      TEST( casual_gateway_manager_ipc, non_existent_path__configuration)
      {
         common::unittest::Trace trace;

         environment::variable::set( "CASUAL_UNITTEST_IPC_PATH", "/non/existent/path");

         local::Domain domain{ local::one_connector_configuration()};

         common::signal::timer::Scoped timer{ std::chrono::milliseconds{ 100}};

         EXPECT_TRUE( process::ping( domain.gateway.process.handle().queue) == domain.gateway.process.handle());
      }


      TEST( casual_gateway_manager_ipc, same_path_as_unittest_domain__configuration___expect_connection)
      {
         common::unittest::Trace trace;

         environment::variable::set( "CASUAL_UNITTEST_IPC_PATH", environment::domain::singleton::path());

         local::Domain domain{ local::one_connector_configuration()};

         common::signal::timer::Scoped timer{ std::chrono::milliseconds{ 100}};

         EXPECT_TRUE( process::ping( domain.gateway.process.handle().queue) == domain.gateway.process.handle());
      }

      namespace local
      {
         namespace
         {

            namespace condition
            {
               template< typename P>
               auto call( P&& predicate) -> decltype( local::call::wait::ready::state())
               {
                  while( true)
                  {
                     auto state = local::call::wait::ready::state();

                     if( predicate( state))
                     {
                        return state;
                     }
                     process::sleep( std::chrono::milliseconds{ 5});
                  }
               }
            } // condition

            auto online() -> decltype( local::call::wait::ready::state())
            {
               using state_type = decltype( local::call::wait::ready::state());

               return condition::call( []( const state_type& state){

                  if( state.connections.empty()) { return false;}

                  using vo_type = manager::admin::vo::Connection;

                  return range::all_of( state.connections, []( const vo_type& vo){
                     return vo.runlevel == vo_type::Runlevel::online;
                  });

               });
            }

         } // <unnamed>
      } // local

      TEST( casual_gateway_manager_ipc, same_path_as_unittest_domain__call_state___expect_1_outbound_and_1_inbound_connection)
      {
         common::unittest::Trace trace;

         environment::variable::set( "CASUAL_UNITTEST_IPC_PATH", environment::domain::singleton::path());

         local::Domain domain{ local::one_connector_configuration()};

         common::signal::timer::Scoped timer{ std::chrono::milliseconds{ 100}};

         //
         // We ping it so we know the gateway is up'n running
         //
         EXPECT_TRUE( process::ping( domain.gateway.process.handle().queue) == domain.gateway.process.handle());

         auto state = local::online();



         ASSERT_TRUE( state.connections.size() == 2) << CASUAL_MAKE_NVP( state);
         range::sort( state.connections);

         using vo_type = manager::admin::vo::Connection;

         auto& outbound = state.connections.at( 0);
         EXPECT_TRUE( outbound.runlevel == vo_type::Runlevel::online) << CASUAL_MAKE_NVP( state);
         EXPECT_TRUE( outbound.type == vo_type::Type::ipc);
         auto& inbound = state.connections.at( 1);
         EXPECT_TRUE( inbound.runlevel == vo_type::Runlevel::online) << CASUAL_MAKE_NVP( state);
         EXPECT_TRUE( inbound.type == vo_type::Type::ipc);
      }

      TEST( casual_gateway_manager_ipc, same_path_as_unittest_domain__call_outbound____expect_call_to_service)
      {
         common::unittest::Trace trace;

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
                  range::sort( state.connections);

                  return state.connections.at( 0).process.queue;
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

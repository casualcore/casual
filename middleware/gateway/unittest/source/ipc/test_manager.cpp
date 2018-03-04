//! 
//! Copyright (c) 2015, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!


#include <gtest/gtest.h>
#include "common/unittest.h"

#include "gateway/manager/admin/vo.h"
#include "gateway/manager/admin/server.h"

#include "common/mockup/file.h"
#include "common/mockup/process.h"
#include "common/mockup/domain.h"

#include "common/environment.h"

#include "common/message/domain.h"

#include "sf/service/protocol/call.h"
#include "sf/log.h"

namespace casual
{
   using namespace common;
   namespace gateway
   {

      namespace local
      {
         using config_domain = common::message::domain::configuration::Domain;

         namespace
         {
            struct Gateway
            {
               Gateway()
                  : process{ "./bin/casual-gateway-manager"}
               {

               }

               mockup::Process process;
            };

            struct Domain
            {
               Domain( config_domain configuration) : manager{ std::move( configuration)}
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
               mockup::domain::service::Manager service;
               mockup::domain::transaction::Manager tm;

               Gateway gateway;
            };



            config_domain empty_configuration()
            {
               return {};
            }


            config_domain one_connector_configuration()
            {
               config_domain result;

               result.gateway.connections.resize( 1);
               result.gateway.connections.front().address = "${CASUAL_UNITTEST_IPC_PATH}";
               result.gateway.connections.front().type = decltype( result.gateway.connections.front().type)::ipc;

               return result;
            }


            namespace call
            {

               manager::admin::vo::State state()
               {
                  sf::service::protocol::binary::Call call;
                  auto reply = call( manager::admin::service::name::state());

                  manager::admin::vo::State result;
                  reply >> CASUAL_MAKE_NVP( result);

                  return result;
               }


               namespace wait
               {
                  namespace ready
                  {

                     bool manager_ready( const manager::admin::vo::State& state)
                     {
                        Trace trace{ "unittest::gateway::local::call::wait::ready::manager_ready"};

                        if( state.connections.empty())
                           return false;

                        return algorithm::all_of( state.connections, []( const manager::admin::vo::Connection& c){
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

                  return algorithm::all_of( state.connections, []( const vo_type& vo){
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

         common::signal::timer::Scoped timer{ std::chrono::seconds{ 5}};

         //
         // We ping it so we know the gateway is up'n running
         //
         EXPECT_TRUE( process::ping( domain.gateway.process.handle().queue) == domain.gateway.process.handle());

         auto state = local::online();



         ASSERT_TRUE( state.connections.size() == 2) << CASUAL_MAKE_NVP( state);
         algorithm::sort( state.connections);

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
                  algorithm::sort( state.connections);

                  return state.connections.at( 0).process.queue;
               };


         auto outbound = get_outbound_queue( domain);


         //platform::binary_type paylaod{ 1, 2, 3, 4, 5, 6, 7, 8, 9};

         common::message::service::call::callee::Request request;
         {
            request.service.name = manager::admin::service::name::state();
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

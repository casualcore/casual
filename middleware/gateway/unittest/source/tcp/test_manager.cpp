//!
//! casual
//!

#include <gtest/gtest.h>
#include "common/unittest.h"

#include "gateway/manager/admin/vo.h"
#include "gateway/manager/admin/server.h"

#include "common/mockup/file.h"
#include "common/mockup/process.h"
#include "common/mockup/domain.h"

#include "common/environment.h"
#include "common/service/lookup.h"

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

                  //
                  // Make sure we're up'n running before we let unittest-stuff interact with us...
                  //
                  process::instance::fetch::handle( process::instance::identity::gateway::manager());
               }

               struct set_environment_t
               {
                  set_environment_t()
                  {
                     environment::variable::set( environment::variable::name::home(), "./" );
                  }
               } set_environment;

               mockup::Process process;
            };

            struct Domain
            {
               Domain( config_domain configuration) : manager{ std::move( configuration)}
               {

               }

               mockup::domain::Manager manager;
               mockup::domain::service::Manager service;
               mockup::domain::transaction::Manager tm;

               Gateway gateway;
            };

            namespace domain
            {
               //!
               //! exposes service domain1
               //!
               struct Service
               {
                  Service( config_domain configuration)
                     : manager{ std::move( configuration)},
                       domain1{ mockup::domain::echo::create::service( "remote1")} {}

                  mockup::domain::Manager manager;
                  mockup::domain::service::Manager service;
                  mockup::domain::transaction::Manager tm;

                  mockup::domain::echo::Server domain1;

                  Gateway gateway;

               };

            } // domain



            config_domain empty_configuration()
            {
               return {};
            }


            config_domain one_listener_configuration()
            {
               config_domain result;

               result.gateway.listeners.resize( 1);
               result.gateway.listeners.front().address = "127.0.0.1:6666";

               result.gateway.connections.resize( 1);
               result.gateway.connections.front().address = "127.0.0.1:6666";

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
                        if( state.connections.empty())
                           return false;

                        return range::any_of( state.connections, []( const manager::admin::vo::Connection& c){
                           return c.runlevel >= manager::admin::vo::Connection::Runlevel::online &&
                                 c.bound == manager::admin::vo::Connection::Bound::out;
                        })
                        && range::any_of( state.connections, []( const manager::admin::vo::Connection& c){
                           return c.runlevel >= manager::admin::vo::Connection::Runlevel::online &&
                                 c.bound == manager::admin::vo::Connection::Bound::in;
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


      TEST( casual_gateway_manager_tcp, empty_configuration)
      {
         common::unittest::Trace trace;

         local::Domain domain{ local::empty_configuration()};

         common::signal::timer::Scoped timer{ std::chrono::milliseconds{ 100}};

         EXPECT_TRUE( process::ping( domain.gateway.process.handle().queue) == domain.gateway.process.handle());
      }


      TEST( casual_gateway_manager_tcp, listen_on_127_0_0_1__6666)
      {
         common::unittest::Trace trace;

         local::Domain domain{ local::one_listener_configuration()};

         common::signal::timer::Scoped timer{ std::chrono::milliseconds{ 100}};

         EXPECT_TRUE( process::ping( domain.gateway.process.handle().queue) == domain.gateway.process.handle());
      }

      TEST( casual_gateway_manager_tcp, listen_on_127_0_0_1__6666__outbound__127_0_0_1__6666__expect_connection)
      {
         common::unittest::Trace trace;

         local::Domain domain{ local::one_listener_configuration()};

         common::signal::timer::Scoped timer{ std::chrono::milliseconds{ 100}};


         EXPECT_TRUE( process::ping( domain.gateway.process.handle().queue) == domain.gateway.process.handle());

         auto state = local::call::wait::ready::state();

         EXPECT_TRUE( state.connections.size() == 2);
         EXPECT_TRUE( range::any_of( state.connections, []( const manager::admin::vo::Connection& c){
            return c.bound == manager::admin::vo::Connection::Bound::out;
         }));
      }

      namespace local
      {
         namespace
         {
            namespace configuration
            {

               config_domain connect_to_our_self_services_service1()
               {
                  auto domain = local::one_listener_configuration();

                  domain.gateway.connections.front().services.push_back( "remote1");

                  return domain;
               }

            } // configuration
         } // <unnamed>
      } // local


      TEST( casual_gateway_manager_tcp,  connect_to_our_self__remote1_advertise__expect_service_remote1)
      {
         common::unittest::Trace trace;

         // exposes service "remote1"
         local::domain::Service domain{ local::configuration::connect_to_our_self_services_service1()};

         common::signal::timer::Scoped timer{ std::chrono::seconds{ 5}};


         EXPECT_TRUE( process::ping( domain.gateway.process.handle().queue) == domain.gateway.process.handle());

         auto state = local::call::wait::ready::state();

         ASSERT_TRUE( state.connections.size() == 2);

         range::sort( state.connections);


         //
         // Expect service domain1 to be available from the outbound connection
         //
         {
            auto reply = common::service::Lookup{ "remote1"}();

            EXPECT_TRUE( reply.service.name == "remote1");
            EXPECT_TRUE( reply.process == state.connections.at( 0).process) << "reply.process: "
                  << reply.process << " - state.connections.at( 0).process: " << state.connections.at( 0).process;
         }

      }

      namespace local
      {
         namespace
         {

            namespace domain
            {
               struct Queue
               {
                  Queue( config_domain configuration) : manager{ std::move( configuration)}
                  {

                  }

                  mockup::domain::Manager manager;
                  mockup::domain::service::Manager service;
                  mockup::domain::transaction::Manager tm;
                  mockup::domain::queue::Broker queue;

                  Gateway gateway;

               };

            } // domain

         } // <unnamed>
      } // local

      TEST( casual_gateway_manager_tcp,  connect_to_our_self__enqueue_dequeue___expect_message)
      {
         common::unittest::Trace trace;

         local::domain::Queue domain{ local::one_listener_configuration()};

         common::signal::timer::Scoped timer{ std::chrono::seconds{ 5}};

         EXPECT_TRUE( process::ping( domain.gateway.process.handle().queue) == domain.gateway.process.handle());


         auto state = local::call::wait::ready::state();
         ASSERT_TRUE( state.connections.size() == 2);
         range::sort( state.connections);

         //
         // Gateway is connected to it self. Hence we can send a request to the outbound, and it
         // will send it to the corresponding inbound, and back in the current (mockup) domain
         //

         ASSERT_TRUE( state.connections.at( 0).bound == manager::admin::vo::Connection::Bound::out);
         auto outbound =  state.connections.at( 0).process;

         const auto payload = unittest::random::binary( 1000);

         // enqueue
         {
            message::queue::enqueue::Request request;
            request.process = process::handle();
            request.name = "queue1";
            request.message.type = "json";
            request.message.payload = payload;


            auto reply = communication::ipc::call( outbound.queue, request);
            EXPECT_TRUE( ! reply.id.empty());
         }

         // dequeue
         {
            message::queue::dequeue::Request request;
            request.process = process::handle();
            request.name = "queue1";

            auto reply = communication::ipc::call( outbound.queue, request);
            ASSERT_TRUE( ! reply.message.empty());
            EXPECT_TRUE( reply.message.front().payload == payload);
            EXPECT_TRUE( reply.message.front().type == "json");
         }

      }


   } // gateway

} // casual

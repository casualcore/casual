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
#include "common/service/lookup.h"

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

               struct set_environment_t
               {
                  set_environment_t()
                  {
                     environment::variable::set( environment::variable::name::home(), "./" );
                  }
               } set_environment;

               file::scoped::Path file;
               mockup::Process process;
            };

            struct Domain
            {
               Domain( const std::string& configuration) : gateway{ configuration}
               {

               }

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


            std::string one_listener_configuration()
            {
               return R"yaml(
domain:
  gateway:
  
    listeners:
      - address: 127.0.0.1:6666

    connections:
      - address: 127.0.0.1:6666

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
                        }) && range::any_of( state.connections.inbound, []( const manager::admin::vo::inbound::Connection& c){
                           return c.runlevel >= manager::admin::vo::inbound::Connection::Runlevel::online;
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
         CASUAL_UNITTEST_TRACE();

         local::Domain domain{ local::empty_configuration()};

         common::signal::timer::Scoped timer{ std::chrono::milliseconds{ 100}};

         EXPECT_TRUE( process::ping( domain.gateway.process.handle().queue) == domain.gateway.process.handle());
      }


      TEST( casual_gateway_manager_tcp, listen_on_127_0_0_1__6666)
      {
         CASUAL_UNITTEST_TRACE();

         local::Domain domain{ local::one_listener_configuration()};

         common::signal::timer::Scoped timer{ std::chrono::milliseconds{ 100}};

         EXPECT_TRUE( process::ping( domain.gateway.process.handle().queue) == domain.gateway.process.handle());
      }

      TEST( casual_gateway_manager_tcp, listen_on_127_0_0_1__6666__outbound__127_0_0_1__6666__expect_connection)
      {
         CASUAL_UNITTEST_TRACE();

         local::Domain domain{ local::one_listener_configuration()};

         common::signal::timer::Scoped timer{ std::chrono::milliseconds{ 100}};


         EXPECT_TRUE( process::ping( domain.gateway.process.handle().queue) == domain.gateway.process.handle());

         auto state = local::call::wait::ready::state();

         EXPECT_TRUE( ! state.connections.outbound.empty());
         EXPECT_TRUE( ! state.connections.inbound.empty());
      }

      namespace local
      {
         namespace
         {
            namespace configuration
            {

               std::string connect_to_our_self_services_service1()
               {
                  return R"yaml(
domain:
  gateway:
  
    listeners:
      - address: 127.0.0.1:6666

    connections:
      - address: 127.0.0.1:6666
        services: [ "remote1"]

)yaml";

               }



            } // configuration
         } // <unnamed>
      } // local


      TEST( casual_gateway_manager_tcp,  connect_to_our_self__remote1_advertise__expect_service_remote1)
      {
         CASUAL_UNITTEST_TRACE();

         local::Domain domain{ local::configuration::connect_to_our_self_services_service1()};

         common::signal::timer::Scoped timer{ std::chrono::milliseconds{ 100}};


         EXPECT_TRUE( process::ping( domain.gateway.process.handle().queue) == domain.gateway.process.handle());

         auto state = local::call::wait::ready::state();

         ASSERT_TRUE( ! state.connections.outbound.empty());
         EXPECT_TRUE( ! state.connections.inbound.empty());

         //
         // Expect service remote1 to be available from the outbound connection
         //
         {
            auto reply = common::service::Lookup{ "remote1"}();

            EXPECT_TRUE( reply.service.name == "remote1");
            EXPECT_TRUE( reply.process == state.connections.outbound.at( 0).process);
         }

      }


   } // gateway

} // casual

//! 
//! Copyright (c) 2015, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!


#include <gtest/gtest.h>
#include "common/unittest.h"

#include "domain/manager/state.h"
#include "domain/manager/configuration.h"
#include "domain/manager/manager.h"
#include "domain/manager/admin/vo.h"
#include "domain/manager/admin/server.h"


#include "common/string.h"
#include "common/environment.h"
#include "common/service/lookup.h"
#include "common/event/listen.h"
#include "common/execute.h"

#include "common/communication/instance.h"

//#include "serviceframework/service/protocol/call.h"
#include "serviceframework/archive/binary.h"
#include "serviceframework/log.h"

#include "domain/manager/unittest/process.h"

#include <fstream>

namespace casual
{
   using namespace common;

   namespace domain
   {
      namespace manager
      {
         namespace local
         {
            namespace
            {
               namespace configuration
               {

                  std::string empty()
                  {
                     return R"(
domain:
  name: empty

)";

                  }

                  std::string echo()
                  {
                     return R"(
domain:
  name: echo
  executables:
    - path: echo
      instances: 4    

)";
                  }

                  std::string echo_restart()
                  {
                     return R"(
domain:
  name: echo_restart
  executables:
    - path: echo
      instances: 4
      arguments: [poop]
      restart: true    

)";
                  }


                  std::string sleep()
                  {
                     return R"(
domain:
  name: sleep
  executables:
    - path: sleep
      arguments: [60]
      instances: 4    

)";
                  }



               } // configuration

            } // <unnamed>
         } // local

         TEST( casual_domain_manager, empty_configuration__expect_boot)
         {
            common::unittest::Trace trace;

            EXPECT_NO_THROW( {
               unittest::Process manager{ { local::configuration::empty()}};
            });
         }

         TEST( casual_domain_manager, echo_configuration__expect_boot)
         {
            common::unittest::Trace trace;

            EXPECT_NO_THROW( {
               unittest::Process manager{ { local::configuration::echo()}};
            });
         }


         TEST( casual_domain_manager, non_existing_path___expect_boot)
         {
            common::unittest::Trace trace;

            const auto configuration = R"(
domain:
   name: echo
   executables:
     - path: non-existing-e53ce069de5f49e6a2f546ad8e175093
       instances: 1    

)";

            EXPECT_NO_THROW( {
               unittest::Process manager{ { configuration}};
            });
         }

         TEST( casual_domain_manager, non_existing_path__restart___expect_restart_ignored_during_boot)
         {
            common::unittest::Trace trace;

            const auto configuration = R"(
domain:
   name: echo
   executables:
     - path: non-existing-e53ce069de5f49e6a2f546ad8e175093
       instances: 1
       restart: true

)";

            EXPECT_NO_THROW( {
               unittest::Process manager{ { configuration}};
            });
         }


         TEST( casual_domain_manager, echo_restart_configuration__expect_boot)
         {
            common::unittest::Trace trace;

            EXPECT_NO_THROW( {
               unittest::Process manager{ { local::configuration::echo_restart()}};
            });
         }


         TEST( casual_domain_manager, sleep_configuration__expect_boot)
         {
            common::unittest::Trace trace;

            EXPECT_NO_THROW( {
               unittest::Process manager{ { local::configuration::sleep()}};
            });
         }

         TEST( casual_domain_manager, non_existent_executable__expect_boot)
         {
            common::unittest::Trace trace;

            const std::string configuration{ R"(
domain:
  name: sleep
  executables:
    - path: a360ec9bec0b4e5fae131c0e7ad931f4-non-existent
      instances: 1

)"};

            EXPECT_NO_THROW( {
               unittest::Process manager{ { configuration}};
            });
         }


         namespace local
         {
            namespace
            {
               namespace call
               {

                  // we don't have access to service-manager (or other managers)
                  // when we build domain-manager.
                  // To be able to get state and such from domain-manager we call
                  // natively and do our serialization "by hand".
                  // 
                  // not that much code, hence I think it's worth it to be able to 
                  // test stuff locally within domain-manager.

                  template< typename A>
                  void serialize( A& archive) {}

                  template< typename A, typename T, typename... Ts>
                  void serialize( A& archive, T&& value, Ts&&... ts)
                  {
                     archive << CASUAL_MAKE_NVP( std::forward< T>( value));
                     serialize( archive, std::forward< Ts>( ts)...);
                  }
                  template< typename R, typename... Ts> 
                  R call( std::string service, Ts&&... arguments)
                  {
                     auto correlation = [&]()
                     {
                        common::message::service::call::callee::Request request;
                        request.process = process::handle();
                        request.service.name = std::move( service);
                        request.buffer.type = common::buffer::type::binary();

                        auto archive = serviceframework::archive::binary::writer( request.buffer.memory);
                        serialize( archive, std::forward< Ts>( arguments)...);

                        return communication::ipc::blocking::send( communication::instance::outbound::domain::manager::device(), request);
                     }();


                     common::message::service::call::Reply reply;
                     communication::ipc::blocking::receive( communication::ipc::inbound::device(), reply, correlation);
                     auto archive = serviceframework::archive::binary::reader( reply.buffer.memory);

                     R result;
                     archive >> CASUAL_MAKE_NVP( result);

                     return result;
                  }

                  admin::vo::State state()
                  {
                     return call< admin::vo::State>( admin::service::name::state());
                  }


                  std::vector< admin::vo::scale::Instances> scale( const std::vector< admin::vo::scale::Instances>& instances)
                  {
                     return call< std::vector< admin::vo::scale::Instances>>( admin::service::name::scale::instances(), instances);
                  }

                  std::vector< admin::vo::scale::Instances> scale( const std::string& alias, common::platform::size::type instances)
                  {
                     return scale( { { alias, instances}});
                  }

               } // call


               namespace configuration
               {

                  std::string long_running_processes_5()
                  {
                     return R"(
domain:
  name: long_running_processes_5
  executables:
    - alias: sleep
      path: sleep
      arguments: [60]
      instances: 5    

)";
                  }

               } // configuration

               namespace predicate
               {
                  struct Manager
                  {
                     bool operator () ( const admin::vo::Executable& value) const
                     {
                        return value.alias == "casual-domain-manager";
                     }
                  };

               } // predicate


            } // <unnamed>
         } // local


         TEST( casual_domain_manager, state_long_running_processes_5__expect_5)
         {
            common::unittest::Trace trace;

            unittest::Process manager{ { local::configuration::long_running_processes_5()}};

            auto state = local::call::state();

            ASSERT_TRUE( state.servers.size() == 1) << CASUAL_MAKE_NVP( state);
            EXPECT_TRUE( state.servers.at( 0).instances.size() == 1) << CASUAL_MAKE_NVP( state);
            ASSERT_TRUE( state.executables.size() == 1) << CASUAL_MAKE_NVP( state);
            EXPECT_TRUE( state.executables.at( 0).instances.size() == 5) << CASUAL_MAKE_NVP( state);

         }


         TEST( casual_domain_manager, state_long_running_processes_5__scale_out_to_10___expect_10)
         {
            common::unittest::Trace trace;

            unittest::Process manager{ { local::configuration::long_running_processes_5()}};

            auto instances = local::call::scale( "sleep", 10);
            EXPECT_TRUE( instances.size() == 1);


            auto state = local::call::state();

            ASSERT_TRUE( state.executables.size() == 1);
            EXPECT_TRUE( state.executables.at( 0).instances.size() == 10) << CASUAL_MAKE_NVP( state);
         }

         TEST( casual_domain_manager, long_running_processes_5__scale_in_to_0___expect_0)
         {
            common::unittest::Trace trace;

            unittest::Process manager{ { local::configuration::long_running_processes_5()}};

            auto instances = local::call::scale( "sleep", 0);
            EXPECT_TRUE( instances.size() == 1);


            auto state = local::call::state();

            ASSERT_TRUE( state.executables.size() == 1);

            // not deterministic how long it takes for the processes to terminate.
            // EXPECT_TRUE( state.executables.at( 0).instances.size() == 0) << CASUAL_MAKE_NVP( state);
         }


         TEST( casual_domain_manager, state_simple_server__expect_boot)
         {
            common::unittest::Trace trace;

            const std::string configuration{ R"(
domain:
  name: simple-server
  servers:
    - path: ./bin/test-simple-server
      instances: 1

)"};

            unittest::Process manager{ { configuration}};

            auto state = local::call::state();

            ASSERT_TRUE( state.servers.size() == 2) << CASUAL_MAKE_NVP( state);
            EXPECT_TRUE( state.servers.at( 0).instances.size() == 1) << CASUAL_MAKE_NVP( state);
            EXPECT_TRUE( state.servers.at( 1).instances.size() == 1) << CASUAL_MAKE_NVP( state);
            EXPECT_TRUE( state.servers.at( 1).alias == "test-simple-server");

         }

         TEST( casual_domain_manager, scale_in___expect__prepare_shutdown_to_service_manager)
         {
            common::unittest::Trace trace;

            constexpr auto configuration = R"(
domain:
  name: simple-server
  servers:
    - path: ./bin/test-simple-server
      instances: 2

)";

            unittest::Process manager{ { configuration}};

            // pretend that we're the service-manager
            // ...since we don't have access to service-manager when we build
            // domain-manager
            communication::instance::connect( communication::instance::identity::service::manager);

            auto instances = local::call::scale( "test-simple-server", 1);
            ASSERT_TRUE( instances.size() == 1) << "instances: " << CASUAL_MAKE_NVP( instances);

            // Consume the request and send reply.
            {
               message::domain::process::prepare::shutdown::Request request;
               EXPECT_TRUE( communication::ipc::blocking::receive( communication::ipc::inbound::device(), request));
               EXPECT_TRUE( request.processes.size() == 1) << "request: " << request;

               auto reply = message::reverse::type( request);
               reply.processes = std::move( request.processes);
               communication::ipc::blocking::send( request.process.ipc, reply);
            }

            // make sure we 'un-connect' our self as service-manager before shutdown
            // otherwise domain-manager will wait for approval from service-manager before
            // each shutdown.
            {
               message::event::process::Exit event;
               event.state.pid = common::process::id();
               event.state.reason = common::process::lifetime::Exit::Reason::exited;
               communication::ipc::blocking::send( communication::instance::outbound::domain::manager::device(), event);
            }
         }

         TEST( casual_domain_manager, configuration_service_routes)
         {
            common::unittest::Trace trace;

            const std::string configuration{ R"(
domain:
  name: routes
  services:
    - timeout: 2h
      name: service1
      routes:
        - service2
        - service3

)"};
            unittest::Process manager{ { configuration}};

            // We need to register this process to the manager
            communication::instance::connect( process::handle());

            message::domain::configuration::server::Request request;
            request.process = process::handle();
            auto reply = communication::ipc::call( communication::instance::outbound::domain::manager::device(), request);


            ASSERT_TRUE( reply.service.routes.size() == 1);
            EXPECT_TRUE( reply.service.routes.at( 0).name == "service1");
            EXPECT_TRUE( reply.service.routes.at( 0).routes.at( 0) == "service2");
            EXPECT_TRUE( reply.service.routes.at( 0).routes.at( 1) == "service3");

         }

         TEST( casual_domain_manager, groups_4__with_5_executables___start_with_instances_1__scale_to_10)
         {
            common::unittest::Trace trace;

            const std::string configuration{ R"(
domain:
  name: big
  groups:
    - name: groupA
    - name: groupB
    - name: groupC
    - name: groupD
  executables:
    - alias: sleepA1
      path: sleep
      arguments: [60]
      instances: 1
      memberships: [groupA]    
    - alias: sleepA2
      path: sleep
      arguments: [60]
      instances: 1
      memberships: [groupA]
    - alias: sleepA3
      path: sleep
      arguments: [60]
      instances: 1
      memberships: [groupA]
    - alias: sleepA4
      path: sleep
      arguments: [60]
      instances: 1
      memberships: [groupA]
    - alias: sleepA5
      path: sleep
      arguments: [60]
      instances: 1
      memberships: [groupA]

    - alias: sleepB1
      path: sleep
      arguments: [60]
      instances: 1
      memberships: [groupB]    
    - alias: sleepB2
      path: sleep
      arguments: [60]
      instances: 1
      memberships: [groupB]
    - alias: sleepB3
      path: sleep
      arguments: [60]
      instances: 1
      memberships: [groupB]
    - alias: sleepB4
      path: sleep
      arguments: [60]
      instances: 1
      memberships: [groupB]
    - alias: sleepB5
      path: sleep
      arguments: [60]
      instances: 1
      memberships: [groupB]

    - alias: sleepC1
      path: sleep
      arguments: [60]
      instances: 1
      memberships: [groupC]    
    - alias: sleepC2
      path: sleep
      arguments: [60]
      instances: 1
      memberships: [groupC]
    - alias: sleepC3
      path: sleep
      arguments: [60]
      instances: 1
      memberships: [groupC]
    - alias: sleepC4
      path: sleep
      arguments: [60]
      instances: 1
      memberships: [groupC]
    - alias: sleepC5
      path: sleep
      arguments: [60]
      instances: 1
      memberships: [groupC]

    - alias: sleepD1
      path: sleep
      arguments: [60]
      instances: 1
      memberships: [groupD]    
    - alias: sleepD2
      path: sleep
      arguments: [60]
      instances: 1
      memberships: [groupD]
    - alias: sleepD3
      path: sleep
      arguments: [60]
      instances: 1
      memberships: [groupD]
    - alias: sleepD4
      path: sleep
      arguments: [60]
      instances: 1
      memberships: [groupD]
    - alias: sleepD5
      path: sleep
      arguments: [60]
      instances: 1
      memberships: [groupD]
)"};

            unittest::Process manager{ { configuration}};

            auto state = local::call::state();
            state.executables = algorithm::trim( state.executables, algorithm::remove_if( state.executables, local::predicate::Manager{}));


            ASSERT_TRUE( state.executables.size() == 4 * 5) << CASUAL_MAKE_NVP( state);

            for( auto& instance : state.executables)
            {
               local::call::scale( instance.alias, 10);
            }

            state = local::call::state();

            for( auto executable : algorithm::remove_if( state.executables, local::predicate::Manager{}))
            {
               EXPECT_TRUE( executable.instances.size() == 10) << "executable.instances.size(): " << executable.instances.size();
            }
         }


      } // manager

   } // domain


} // casual

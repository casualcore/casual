//!
//! casual 
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
#include "common/mockup/file.h"
#include "common/mockup/domain.h"
#include "common/mockup/process.h"
#include "common/service/lookup.h"
#include "common/event/listen.h"
#include "sf/service/protocol/call.h"
#include "sf/log.h"


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

                  std::vector< file::scoped::Path> files( const std::vector< std::string>& config)
                  {
                     return range::transform( config, []( const std::string& c){
                        return file::scoped::Path{ mockup::file::temporary::content( ".yaml", c)};
                     });
                  }

                  std::string names( const std::vector< file::scoped::Path>& files)
                  {
                     return string::join( files, " ");
                  }

               } // configuration

               struct Manager
               {
                  Manager( const std::vector< std::string>& config)
                   : files( configuration::files( config)),
                     process{ "./bin/casual-domain-manager", {
                        "--event-queue", common::string::compose( common::communication::ipc::inbound::id()),
                        "--configuration-files", configuration::names( files),
                        "--bare",
                     }}
                  {

                     //
                     // Make sure we unregister the event subscription
                     //
                     auto unsubscribe = common::scope::execute( [](){
                        common::event::unsubscribe( common::process::handle(), {});
                     });

                     //
                     // Wait for the domain to boot
                     //
                     unittest::domain::manager::wait( common::communication::ipc::inbound::device());

                     //
                     // Set environment variable to make it easier for other processes to
                     // reach domain-manager (should work any way...)
                     //
                     common::environment::variable::process::set(
                           common::environment::variable::name::ipc::domain::manager(),
                           process.handle());

                  }

                  struct remove_singleton_file_t
                  {
                     remove_singleton_file_t()
                     {
                        if( file::exists( environment::domain::singleton::file()))
                        {
                           file::remove( environment::domain::singleton::file());
                        }
                     }
                  } remove_singleton_file;

                  std::vector< file::scoped::Path> files;
                  mockup::Process process;
               };



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
               local::Manager manager{ { local::configuration::empty()}};
            });
         }

         TEST( casual_domain_manager, echo_configuration__expect_boot)
         {
            common::unittest::Trace trace;

            EXPECT_NO_THROW( {
               local::Manager manager{ { local::configuration::echo()}};
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
               local::Manager manager{ { configuration}};
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
               local::Manager manager{ { configuration}};
            });
         }


         TEST( casual_domain_manager, echo_restart_configuration__expect_boot)
         {
            common::unittest::Trace trace;

            EXPECT_NO_THROW( {
               local::Manager manager{ { local::configuration::echo_restart()}};
            });
         }


         TEST( casual_domain_manager, sleep_configuration__expect_boot)
         {
            common::unittest::Trace trace;

            EXPECT_NO_THROW( {
               local::Manager manager{ { local::configuration::sleep()}};
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
               local::Manager manager{ { configuration}};
            });
         }


         namespace local
         {
            namespace
            {
               namespace call
               {

                  admin::vo::State state()
                  {
                     sf::service::protocol::binary::Call call;
                     auto reply = call( admin::service::name::state());

                     admin::vo::State serviceReply;

                     reply >> CASUAL_MAKE_NVP( serviceReply);

                     return serviceReply;
                  }


                  std::vector< admin::vo::scale::Instances> scale( const std::vector< admin::vo::scale::Instances>& instances)
                  {
                     sf::service::protocol::binary::Call call;

                     call << CASUAL_MAKE_NVP( instances);
                     auto reply = call( admin::service::name::scale::instances());

                     std::vector< admin::vo::scale::Instances> result;
                     reply >> CASUAL_MAKE_NVP( result);

                     return result;
                  }

                  std::vector< admin::vo::scale::Instances> scale( const std::string& alias, std::size_t instances)
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

            local::Manager manager{ { local::configuration::long_running_processes_5()}};

            mockup::domain::service::Manager service;

            auto state = local::call::state();

            ASSERT_TRUE( state.servers.size() == 1) << CASUAL_MAKE_NVP( state);
            EXPECT_TRUE( state.servers.at( 0).instances.size() == 1) << CASUAL_MAKE_NVP( state);
            ASSERT_TRUE( state.executables.size() == 1) << CASUAL_MAKE_NVP( state);
            EXPECT_TRUE( state.executables.at( 0).instances.size() == 5) << CASUAL_MAKE_NVP( state);

         }


         TEST( casual_domain_manager, state_long_running_processes_5__scale_out_to_10___expect_10)
         {
            common::unittest::Trace trace;

            local::Manager manager{ { local::configuration::long_running_processes_5()}};

            mockup::domain::service::Manager service;

            auto instances = local::call::scale( "sleep", 10);
            EXPECT_TRUE( instances.size() == 1);


            auto state = local::call::state();

            ASSERT_TRUE( state.executables.size() == 1);
            EXPECT_TRUE( state.executables.at( 0).instances.size() == 10) << CASUAL_MAKE_NVP( state);
         }

         TEST( casual_domain_manager, long_running_processes_5__scale_in_to_0___expect_0)
         {
            common::unittest::Trace trace;

            local::Manager manager{ { local::configuration::long_running_processes_5()}};

            mockup::domain::service::Manager service;

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

            local::Manager manager{ { configuration}};

            mockup::domain::service::Manager service;

            auto state = local::call::state();

            ASSERT_TRUE( state.servers.size() == 2) << CASUAL_MAKE_NVP( state);
            EXPECT_TRUE( state.servers.at( 0).instances.size() == 1) << CASUAL_MAKE_NVP( state);
            EXPECT_TRUE( state.servers.at( 1).instances.size() == 1) << CASUAL_MAKE_NVP( state);
            EXPECT_TRUE( state.servers.at( 1).alias == "test-simple-server");

         }

         TEST( casual_domain_manager, scale_in___expect__prepare_shutdown_to_broker)
         {
            common::unittest::Trace trace;

            const std::string configuration{ R"(
domain:
  name: simple-server
  servers:
    - path: ./bin/test-simple-server
      instances: 2

)"};

            local::Manager manager{ { configuration}};

            //
            // We add/override handler for prepare shutdown and forward to our
            // local ipc queue
            //
            auto local_queue = communication::ipc::inbound::id();

            mockup::domain::service::Manager service{
               [local_queue]( message::domain::process::prepare::shutdown::Request& m)
               {
                  Trace trace{ "local forward"};
                  mockup::ipc::eventually::send( local_queue, m);
               }
            };

            auto instances = local::call::scale( "test-simple-server", 1);
            ASSERT_TRUE( instances.size() == 1) << "instances: " << CASUAL_MAKE_NVP( instances);

            //
            // Consume the request and send reply.
            //
            {
               message::domain::process::prepare::shutdown::Request request;
               EXPECT_TRUE( communication::ipc::blocking::receive( communication::ipc::inbound::device(), request));
               EXPECT_TRUE( request.processes.size() == 1) << "request: " << request;

               auto reply = message::reverse::type( request);
               reply.processes = std::move( request.processes);
               communication::ipc::blocking::send( request.process.queue, reply);
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

            local::Manager manager{ { configuration}};

            mockup::ipc::Collector server;
            // We need to register this process to the manager
            process::instance::connect( process::handle());


            message::domain::configuration::server::Request request;
            request.process = process::handle();
            auto reply = communication::ipc::call( communication::ipc::domain::manager::device(), request);


            ASSERT_TRUE( reply.routes.size() == 1);
            EXPECT_TRUE( reply.routes.at( 0).name == "service1");
            EXPECT_TRUE( reply.routes.at( 0).routes.at( 0) == "service2");
            EXPECT_TRUE( reply.routes.at( 0).routes.at( 1) == "service3");

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

            local::Manager manager{ { configuration}};

            mockup::domain::service::Manager service;

            auto state = local::call::state();
            state.executables = range::trim( state.executables, range::remove_if( state.executables, local::predicate::Manager{}));


            ASSERT_TRUE( state.executables.size() == 4 * 5) << CASUAL_MAKE_NVP( state);

            for( auto& instance : state.executables)
            {
               local::call::scale( instance.alias, 10);
            }

            state = local::call::state();

            for( auto executable : range::remove_if( state.executables, local::predicate::Manager{}))
            {
               EXPECT_TRUE( executable.instances.size() == 10) << "executable.instances.size(): " << executable.instances.size();
            }
         }


      } // manager

   } // domain


} // casual

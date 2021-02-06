//! 
//! Copyright (c) 2015, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!


#include "common/unittest.h"

#include "domain/manager/state.h"
#include "domain/manager/configuration.h"
#include "domain/manager/manager.h"
#include "domain/manager/admin/model.h"
#include "domain/manager/admin/server.h"
#include "domain/manager/unittest/process.h"

#include "../../include/unittest/call.h"

#include "common/string.h"
#include "common/environment.h"
#include "common/service/lookup.h"
#include "common/service/type.h"
#include "common/event/listen.h"
#include "common/execute.h"



#include <fstream>

namespace casual
{
   using namespace common;

   using Contract = common::service::execution::timeout::contract::Type;

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

         TEST( domain_manager, empty_configuration__expect_boot)
         {
            common::unittest::Trace trace;

            EXPECT_NO_THROW( {
               unittest::Process manager{ { local::configuration::empty()}};
            });
         }

         TEST( domain_manager, echo_configuration__expect_boot)
         {
            common::unittest::Trace trace;

            EXPECT_NO_THROW( {
               unittest::Process manager{ { local::configuration::echo()}};
            });
         }


         TEST( domain_manager, non_existing_path___expect_boot)
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

         TEST( domain_manager, non_existing_path__restart___expect_restart_ignored_during_boot)
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


         TEST( domain_manager, echo_restart_configuration__expect_boot)
         {
            common::unittest::Trace trace;

            EXPECT_NO_THROW( {
               unittest::Process manager{ { local::configuration::echo_restart()}};
            });
         }


         TEST( domain_manager, sleep_configuration__expect_boot)
         {
            common::unittest::Trace trace;

            EXPECT_NO_THROW( {
               unittest::Process manager{ { local::configuration::sleep()}};
            });
         }

         TEST( domain_manager, non_existent_executable__expect_boot)
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
                  admin::model::State state()
                  {
                     return unittest::call< admin::model::State>( admin::service::name::state);
                  }

                  auto scale( const std::vector< admin::model::scale::Alias>& aliases)
                  {
                     return unittest::call< std::vector< common::Uuid>>( admin::service::name::scale::aliases, aliases);
                  }

                  auto scale( const std::string& alias, platform::size::type instances)
                  {
                     return scale( { { alias, instances}});
                  }

                  namespace restart
                  {
                     auto aliases( const std::vector< admin::model::restart::Alias>& aliases)
                     {
                        return unittest::call< std::vector< common::Uuid>>( admin::service::name::restart::aliases, aliases);
                     }

                     auto aliases( std::vector< std::string> aliases)
                     {
                        auto transform = []( auto& a) 
                        {
                           return admin::model::restart::Alias{ std::move( a)};
                        };
                        return restart::aliases( algorithm::transform( aliases, transform));
                     }

                     auto groups( std::vector< std::string> groups)
                     {
                        auto transform = []( auto& name) 
                        {
                           return admin::model::restart::Group{ std::move( name)};
                        };

                        return unittest::call< std::vector< common::Uuid>>( admin::service::name::restart::groups, algorithm::transform( groups, transform));
                     }
                  } // restart


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
                     bool operator () ( const admin::model::Executable& value) const
                     {
                        return value.alias == "casual-domain-manager";
                     }
                  };

               } // predicate

            } // <unnamed>
         } // local
         
         TEST( domain_manager, state_contains_domain_identity)
         {
            common::unittest::Trace trace;

            unittest::Process manager{ { local::configuration::sleep()}};

            auto state = local::call::state();

            EXPECT_TRUE( ! state.identity.name.empty());
            EXPECT_TRUE( state.identity.id);
         }

         TEST( domain_manager, state_long_running_processes_5__expect_5)
         {
            common::unittest::Trace trace;

            unittest::Process manager{ { local::configuration::long_running_processes_5()}};

            auto state = local::call::state();

            ASSERT_TRUE( state.servers.size() == 1) << CASUAL_NAMED_VALUE( state);
            EXPECT_TRUE( state.servers.at( 0).instances.size() == 1) << CASUAL_NAMED_VALUE( state);
            ASSERT_TRUE( state.executables.size() == 1) << CASUAL_NAMED_VALUE( state);
            EXPECT_TRUE( state.executables.at( 0).instances.size() == 5) << CASUAL_NAMED_VALUE( state);
         }



         TEST( domain_manager, state_long_running_processes_5__scale_out_to_10___expect_10)
         {
            common::unittest::Trace trace;

            unittest::Process manager{ { local::configuration::long_running_processes_5()}};

            auto tasks = local::call::scale( "sleep", 10);
            ASSERT_TRUE( tasks.size() == 1);


            auto state = local::call::state();

            ASSERT_TRUE( state.executables.size() == 1);
            EXPECT_TRUE( state.executables.at( 0).instances.size() == 10) << CASUAL_NAMED_VALUE( state);
         }

         TEST( domain_manager, long_running_processes_5__scale_in_to_0___expect_0)
         {
            common::unittest::Trace trace;

            unittest::Process manager{ { local::configuration::long_running_processes_5()}};

            auto tasks = local::call::scale( "sleep", 0);
            ASSERT_TRUE( tasks.size() == 1);


            auto state = local::call::state();

            ASSERT_TRUE( state.executables.size() == 1);

            // not deterministic how long it takes for the processes to terminate.
            // EXPECT_TRUE( state.executables.at( 0).instances.size() == 0) << CASUAL_NAMED_VALUE( state);
         }


         TEST( domain_manager, state_simple_server__expect_boot)
         {
            common::unittest::Trace trace;

            constexpr auto configuration = R"(
domain:
  name: simple-server
  servers:
    - path: ./bin/test-simple-server
      instances: 1

)";

            unittest::Process manager{ { configuration}};

            auto state = local::call::state();

            ASSERT_TRUE( state.servers.size() == 2) << CASUAL_NAMED_VALUE( state);
            EXPECT_TRUE( state.servers.at( 0).instances.size() == 1) << CASUAL_NAMED_VALUE( state);
            EXPECT_TRUE( state.servers.at( 1).instances.size() == 1) << CASUAL_NAMED_VALUE( state);
            EXPECT_TRUE( state.servers.at( 1).alias == "test-simple-server");

         }

         namespace local
         {
            namespace
            {
               auto get_global_state = []( auto& process)
               {
                  return common::communication::ipc::call( 
                     process.ipc,
                     common::message::domain::instance::global::state::Request{ common::process::handle()});
               };

               auto get_variable_checker = []( auto& process)
               {
                  return [reply = get_global_state( process)]( std::string key, std::string value)
                  {
                     auto has_name = [&key]( auto& value){ return value.name() == key;};
                     
                     if( auto found = common::algorithm::find_if( reply.environment.variables, has_name))
                        return found->value() == value;
                     
                     return false;
                  };
               };
            } // <unnamed>
         } // local

         TEST( domain_manager, simple_server__1_instance__expect_alias_and_index)
         {
            common::unittest::Trace trace;

            constexpr auto configuration = R"(
domain:
  name: simple-server
  servers:
    - path: ./bin/test-simple-server
      alias: foo
      instances: 1
)";

            unittest::Process manager{ { configuration}};

            auto state = local::call::state();

            auto server = local::get_global_state( state.servers.at( 1).instances.at( 0).handle);
            EXPECT_TRUE( server.instance.alias == "foo") << "server.instance.alias: " << server.instance.alias;
            EXPECT_TRUE( server.instance.index == 0) << "server.instance.index: " << server.instance.index;

         }

         TEST( domain_manager, simple_server__10_instance__expect_alias_and_index)
         {
            common::unittest::Trace trace;

            constexpr auto configuration = R"(
domain:
  name: simple-server
  servers:
    - path: ./bin/test-simple-server
      alias: foo
      instances: 10
)";

            unittest::Process manager{ { configuration}};

            auto state = local::call::state();

            EXPECT_TRUE( state.servers.at( 1).instances.size() == 10);

            auto index = 0;

            auto order_spawnpoint = []( auto& l, auto& r){ return l.spawnpoint < r.spawnpoint;};

            for( auto& instance : algorithm::sort( state.servers.at( 1).instances, order_spawnpoint))
            {
               auto instance_state = local::get_global_state( instance.handle);
               EXPECT_TRUE( instance_state.instance.alias == "foo");
               EXPECT_TRUE( instance_state.instance.index == index)  << "instance-index: " << instance_state.instance.index << " - index: " << index;
               ++index;
            }
         }         

         TEST( domain_manager, simple_server__nested_environment_variables__expect_boot)
         {
            common::unittest::Trace trace;

            constexpr auto configuration = R"(
domain:
  name: simple-server
  servers:
    - path: ./bin/test-simple-server
      instances: 1
      environment:
        variables: 
         - key: PARENT
           value: foo
         - key: CHILD
           value: "${PARENT}/bar"

)";

            unittest::Process manager{ { configuration}};

            auto state = local::call::state();

            ASSERT_TRUE( state.servers.size() == 2) << CASUAL_NAMED_VALUE( state);
            EXPECT_TRUE( state.servers.at( 0).instances.size() == 1) << CASUAL_NAMED_VALUE( state);
            EXPECT_TRUE( state.servers.at( 1).instances.size() == 1) << CASUAL_NAMED_VALUE( state);
            EXPECT_TRUE( state.servers.at( 1).alias == "test-simple-server");

            auto& simple = state.servers.at( 1);
            ASSERT_TRUE( simple.alias == "test-simple-server");
            EXPECT_TRUE( simple.instances.size() == 1) << CASUAL_NAMED_VALUE( simple);

            // get environment variables
            auto checker = local::get_variable_checker( simple.instances.at( 0).handle);
            EXPECT_TRUE( checker( "PARENT", "foo"));
            EXPECT_TRUE( checker( "CHILD", "foo/bar"));

         }

         TEST( domain_manager, simple_server__signal_hangup)
         {
            common::unittest::Trace trace;

            constexpr auto configuration = R"(
domain:
  name: simple-server
  servers:
    - path: ./bin/test-simple-server
      instances: 1
)";

            unittest::Process manager{ { configuration}};

            auto state = local::call::state();
            ASSERT_TRUE( state.servers.size() == 2) << CASUAL_NAMED_VALUE( state);

            auto& instance = state.servers.at( 1).instances.at( 0);

            common::signal::send( instance.handle.pid, common::code::signal::hangup);

            // check that the signal has been received.
            // There are no guarantess when the signal is received, just that it will be
            // eventually, so we need to loop...
            while( true)
            {
               auto checker = local::get_variable_checker( instance.handle);
               if( checker( "CASUAL_SIMPLE_SERVER_HANGUP_SIGNAL", "true"))
                  break;

               process::sleep( std::chrono::milliseconds{ 1});
            }

            common::communication::instance::ping( instance.handle.ipc);

            // check that the internal message that simple-server has pushed to it self has been received
            {
               auto checker = local::get_variable_checker( instance.handle);
               EXPECT_TRUE( checker( "CASUAL_SIMPLE_SERVER_HANGUP_MESSAGE", "true"));
            }
         }


         TEST( domain_manager, simple_server__assassinate_process_kill)
         {
            common::unittest::Trace trace;

            constexpr auto configuration = R"(
domain:
  name: simple-server
  servers:
    - path: ./bin/test-simple-server
      instances: 1
)";

            unittest::Process manager{ { configuration}};

            auto state_before = local::call::state();
            ASSERT_TRUE( state_before.servers.size() == 2) << CASUAL_NAMED_VALUE( state_before);

            auto& target = state_before.servers.at( 1).instances.at( 0).handle.pid;

            // setup subscription to see when hit is done
            message::event::process::Exit died;
            common::event::subscribe( common::process::handle(), { died.type()});

            // order hit
            message::event::process::Assassination assassination;
            assassination.target = target;
            assassination.contract = Contract::kill;
            communication::device::blocking::send( communication::instance::outbound::domain::manager::device(), assassination);

            // check if/when hit is performed
            common::communication::device::blocking::receive( common::communication::ipc::inbound::device(), died);
         
            auto state_after = local::call::state();

            ASSERT_TRUE( state_after.servers.size() == 2) << CASUAL_NAMED_VALUE( state_after);
            ASSERT_TRUE( state_after.servers.at( 1).instances.size() == 1) << CASUAL_NAMED_VALUE( state_after);
            
            EXPECT_TRUE( state_after.servers.at( 1).instances.at( 0).state == admin::model::instance::State::exit) << CASUAL_NAMED_VALUE( state_after);

            // is correct target killed
            EXPECT_TRUE( assassination.target == died.state.pid) << CASUAL_NAMED_VALUE( assassination) << '\n' << CASUAL_NAMED_VALUE( died);

         }

         TEST( domain_manager, simple_server__assassinate_process_terminate)
         {
            common::unittest::Trace trace;

            constexpr auto configuration = R"(
domain:
  name: simple-server
  servers:
    - path: ./bin/test-simple-server
      instances: 1
)";

            unittest::Process manager{ { configuration}};

            auto state_before = local::call::state();
            ASSERT_TRUE( state_before.servers.size() == 2) << CASUAL_NAMED_VALUE( state_before);

            auto& target = state_before.servers.at( 1).instances.at( 0).handle.pid;

            // setup subscription to see when hit is done
            message::event::process::Exit died;
            common::event::subscribe( common::process::handle(), { died.type()});

            // order hit
            message::event::process::Assassination assassination;
            assassination.target = target;
            assassination.contract = Contract::terminate;
            communication::device::blocking::send( communication::instance::outbound::domain::manager::device(), assassination);

            // check if/when hit is performed
            common::communication::device::blocking::receive( common::communication::ipc::inbound::device(), died);
         
            auto state_after = local::call::state();

            ASSERT_TRUE( state_after.servers.size() == 2) << CASUAL_NAMED_VALUE( state_after);
            ASSERT_TRUE( state_after.servers.at( 1).instances.size() == 1) << CASUAL_NAMED_VALUE( state_after);
            
            EXPECT_TRUE( state_after.servers.at( 1).instances.at( 0).state == admin::model::instance::State::exit) << CASUAL_NAMED_VALUE( state_after);

            // is correct target killed
            EXPECT_TRUE( assassination.target == died.state.pid) << CASUAL_NAMED_VALUE( assassination) << '\n' << CASUAL_NAMED_VALUE( died);

         }

         TEST( domain_manager, simple_server__assassinate_process_linger)
         {
            common::unittest::Trace trace;

            constexpr auto configuration = R"(
domain:
  name: simple-server
  servers:
    - path: ./bin/test-simple-server
      instances: 1
)";

            unittest::Process manager{ { configuration}};

            auto state_before = local::call::state();
            ASSERT_TRUE( state_before.servers.size() == 2) << CASUAL_NAMED_VALUE( state_before);

            auto& target = state_before.servers.at( 1).instances.at( 0).handle.pid;

            // order wrongfull hit
            message::event::process::Assassination assassination;
            assassination.target = target;
            assassination.contract = Contract::linger;
            communication::device::blocking::send( communication::instance::outbound::domain::manager::device(), assassination);

            process::sleep( std::chrono::milliseconds{ 500});

            auto state_after = local::call::state();

            ASSERT_TRUE( state_after.servers.size() == 2) << CASUAL_NAMED_VALUE( state_after);
            ASSERT_TRUE( state_after.servers.at( 1).instances.size() == 1) << CASUAL_NAMED_VALUE( state_after);
            
            EXPECT_TRUE( state_after.servers.at( 1).instances.at( 0).state == admin::model::instance::State::running) << CASUAL_NAMED_VALUE( state_after);
         }

         TEST( domain_manager, scale_in___expect__prepare_shutdown_to_service_manager)
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
            communication::instance::whitelist::connect( communication::instance::identity::service::manager);

            // make sure we 'disconnect' our self as service-manager before shutdown
            // otherwise domain-manager will wait for approval from service-manager before
            // each shutdown.
            auto disconnect = execute::scope( []()
            {
               message::event::process::Exit event;
               event.state.pid = common::process::id();
               event.state.reason = common::process::lifetime::Exit::Reason::exited;
               communication::device::blocking::send( communication::instance::outbound::domain::manager::device(), event);
            });

            auto tasks = local::call::scale( "test-simple-server", 1);
            ASSERT_TRUE( tasks.size() == 1) << "tasks: " << CASUAL_NAMED_VALUE( tasks);

            // Consume the request and send reply.
            {
               message::domain::process::prepare::shutdown::Request request;
               EXPECT_TRUE( communication::device::blocking::receive( communication::ipc::inbound::device(), request));
               EXPECT_TRUE( request.processes.size() == 1) << CASUAL_NAMED_VALUE( request);

               auto reply = message::reverse::type( request);
               reply.processes = std::move( request.processes);
               communication::device::blocking::send( request.process.ipc, reply);
            }
         }


         TEST( domain_manager, groups_4__with_5_executables___start_with_instances_1__scale_to_10)
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


            ASSERT_TRUE( state.executables.size() == 4 * 5) << CASUAL_NAMED_VALUE( state);

            for( auto& instance : state.executables)
            {
               ASSERT_TRUE( local::call::scale( instance.alias, 10).size() == 1);
            }

            state = local::call::state();

            for( auto executable : algorithm::remove_if( state.executables, local::predicate::Manager{}))
            {
               EXPECT_TRUE( executable.instances.size() == 10) << "executable.instances.size(): " << executable.instances.size();
            }
         }

         namespace local
         {
            namespace
            {
               namespace find
               {
                  auto alias = []( auto& entities, auto& alias)
                  {
                     return algorithm::find_if( entities, [&alias]( auto& e)
                     {
                        return e.alias == alias;
                     });
                  };
               } // find

               namespace predicate
               {
                  auto spawnpont = []( auto& timepoint)
                  {
                     return [&timepoint]( auto& instance)
                     {
                        return timepoint < instance.spawnpoint; 
                     };
                  };
                  
               } // predicate
            } // <unnamed>
         } // local
         TEST( domain_manager, restart_executable)
         {
            common::unittest::Trace trace;

            constexpr auto configuration = R"(
domain:
  name: simple-server
  executables:
    - alias: sleep
      path: sleep
      arguments: [60]
      instances: 2
)";

            unittest::Process manager{ { configuration}};

            auto now = platform::time::clock::type::now();

            decltype( local::call::restart::aliases( { ""})) result;

            auto condition = event::condition::compose( 
               event::condition::prelude( [&result](){ result = local::call::restart::aliases( { "sleep"});}), 
               event::condition::done( [&result](){ return result.empty();})
            );

            // start and listen for events
            event::listen( condition, 
               [ &result]( const message::event::Task& task)
               {
                  // remove correlated task
                  if( task.done())
                     algorithm::trim( result, algorithm::remove( result, task.correlation));
               }
            );

            auto state = local::call::state();

            auto found = local::find::alias( state.executables, "sleep");

            ASSERT_TRUE( found.size() == 1) << CASUAL_NAMED_VALUE( found);
            // all instances should have a spawnpoint later than before the restart
            EXPECT_TRUE( algorithm::all_of( found->instances, local::predicate::spawnpont( now))) << CASUAL_NAMED_VALUE( found) << "\n" << CASUAL_NAMED_VALUE( now);
         }


         TEST( domain_manager, restart_server)
         {
            common::unittest::Trace trace;

            constexpr auto configuration = R"(
domain:
  name: simple-server
  servers:
    - path: ./bin/test-simple-server
      alias: simple-server
      instances: 2

)";

            unittest::Process manager{ { configuration}};

            auto now = platform::time::clock::type::now();

            decltype( local::call::restart::aliases( { ""})) result;

            auto condition = event::condition::compose( 
               event::condition::prelude( [&result](){ result = local::call::restart::aliases( { "simple-server"});}), 
               event::condition::done( [&result](){ return result.empty();})
            );

            // start and listen for events
            common::event::listen( condition, 
               [ &result]( const message::event::Task& task)
               {
                  // remove correlated task
                  if( task.done())
                     algorithm::trim( result, algorithm::remove( result, task.correlation));
               }
            );

            auto state = local::call::state();

            auto found = local::find::alias( state.servers, "simple-server");

            ASSERT_TRUE( found.size() == 1) << CASUAL_NAMED_VALUE( found);
            // all instances should have a spawnpoint later than before the restart
            EXPECT_TRUE( algorithm::all_of( found->instances, local::predicate::spawnpont( now))) << CASUAL_NAMED_VALUE( found) << "\n" << CASUAL_NAMED_VALUE( now);
         }


         TEST( domain_manager, restart_group_A__expect_restartded_others_untouched)
         {
            common::unittest::Trace trace;

            constexpr auto configuration = R"(
domain:
  name: restart_groups

  groups:
    - name: A
    - name: B
      dependencies: [ A]
    - name: C
      dependencies: [ B]

  servers:
    - path: ./bin/test-simple-server
      alias: simple-server
      instances: 2
      memberships: [ A]

  executables:
    - alias: sleep
      path: sleep
      arguments: [60]
      instances: 2
      memberships: [ B]

)";

            unittest::Process manager{ { configuration}};

            auto now = platform::time::clock::type::now();

            decltype( local::call::restart::groups( { ""})) result;

            auto condition = event::condition::compose( 
               event::condition::prelude( [&result](){ result = local::call::restart::groups( { "A"});}), 
               event::condition::done( [&result](){ return result.empty();})
            );

            // start and listen for events
            common::event::listen( condition, 
               [ &result]( const message::event::Task& task)
               {
                  // remove correlated task
                  if( task.done())
                     algorithm::trim( result, algorithm::remove( result, task.correlation));
               }
            );

            auto state = local::call::state();

            auto found = local::find::alias( state.servers, "simple-server");

            ASSERT_TRUE( found.size() == 1) << CASUAL_NAMED_VALUE( found);
            // all instances should have a spawnpoint later than before the restart
            EXPECT_TRUE( algorithm::all_of( found->instances, local::predicate::spawnpont( now))) << CASUAL_NAMED_VALUE( found) << "\n" << CASUAL_NAMED_VALUE( now);

            // other instances should be untouched
            {
               auto found = local::find::alias( state.executables, "sleep");
               ASSERT_TRUE( found);
               EXPECT_TRUE( algorithm::none_of( found->instances, local::predicate::spawnpont( now)));
            }
         }

         TEST( domain_manager, restart_group_B__expect_restartded_others_untouched)
         {
            common::unittest::Trace trace;

            constexpr auto configuration = R"(
domain:
  name: restart_groups

  groups:
    - name: A
    - name: B
      dependencies: [ A]
    - name: C
      dependencies: [ B]

  servers:
    - path: ./bin/test-simple-server
      alias: simple-server
      instances: 2
      memberships: [ A]

  executables:
    - alias: sleep
      path: sleep
      arguments: [60]
      instances: 2
      memberships: [ B]

)";

            unittest::Process manager{ { configuration}};

            auto now = platform::time::clock::type::now();

            decltype( local::call::restart::groups( { ""})) result;

            auto condition = event::condition::compose( 
               event::condition::prelude( [&result](){ result = local::call::restart::groups( { "B"});}), 
               event::condition::done( [&result](){ return result.empty();})
            );

            // start and listen for events
            common::event::listen( condition, 
               [ &result]( const message::event::Task& task)
               {
                  // remove correlated task
                  if( task.done())
                     algorithm::trim( result, algorithm::remove( result, task.correlation));
               }
            );

            auto state = local::call::state();

            // all instances should have a spawnpoint later than before the restart
            {
               auto found = local::find::alias( state.executables, "sleep");
               ASSERT_TRUE( found);
               EXPECT_TRUE( algorithm::all_of( found->instances, local::predicate::spawnpont( now)));
            }

            // other instances should be untouched
            {
               auto found = local::find::alias( state.servers, "simple-server");
               ASSERT_TRUE( found);
               EXPECT_TRUE( algorithm::none_of( found->instances, local::predicate::spawnpont( now)));
            }
         }


         TEST( domain_manager, restart_group_A_B___expect_all_restartded)
         {
            common::unittest::Trace trace;

            constexpr auto configuration = R"(
domain:
  name: restart_groups

  groups:
    - name: A
    - name: B
      dependencies: [ A]
    - name: C
      dependencies: [ B]

  servers:
    - path: ./bin/test-simple-server
      alias: simple-server
      instances: 2
      memberships: [ A]

  executables:
    - alias: sleep
      path: sleep
      arguments: [60]
      instances: 2
      memberships: [ B]

)";

            unittest::Process manager{ { configuration}};

            auto now = platform::time::clock::type::now();

            decltype( local::call::restart::groups( { ""})) result;

            auto condition = event::condition::compose( 
               event::condition::prelude( [&result](){ result = local::call::restart::groups( { "A", "B"});}), 
               event::condition::done( [&result](){ return result.empty();})
            );

            // start and listen for events
            common::event::listen( condition, 
               [ &result]( const message::event::Task& task)
               {
                  // remove correlated task
                  if( task.done())
                     algorithm::trim( result, algorithm::remove( result, task.correlation));
               }
            );

            auto state = local::call::state();

            // all instances should have a spawnpoint later than before the restart
            {
               auto found = local::find::alias( state.executables, "sleep");
               ASSERT_TRUE( found);
               EXPECT_TRUE( algorithm::all_of( found->instances, local::predicate::spawnpont( now)));
            }
            {
               auto found = local::find::alias( state.servers, "simple-server");
               ASSERT_TRUE( found);
               EXPECT_TRUE( algorithm::all_of( found->instances, local::predicate::spawnpont( now)));
            }
         }

         TEST( domain_manager, restart_no_group___expect_all_restartded)
         {
            common::unittest::Trace trace;

            constexpr auto configuration = R"(
domain:
  name: restart_groups

  groups:
    - name: A
    - name: B
      dependencies: [ A]
    - name: C
      dependencies: [ B]

  servers:
    - path: ./bin/test-simple-server
      alias: simple-server
      instances: 2
      memberships: [ A]

  executables:
    - alias: sleep
      path: sleep
      arguments: [60]
      instances: 2
      memberships: [ B]

)";

            unittest::Process manager{ { configuration}};

            auto now = platform::time::clock::type::now();

            decltype( local::call::restart::groups( { ""})) result;

            auto condition = event::condition::compose( 
               event::condition::prelude( [&result](){ result = local::call::restart::groups( {});}), 
               event::condition::done( [&result](){ return result.empty();})
            );

            // start and listen for events
            common::event::listen( condition, 
               [ &result]( const message::event::Task& task)
               {
                  // remove correlated task
                  if( task.done())
                     algorithm::trim( result, algorithm::remove( result, task.correlation));
               }
            );

            auto state = local::call::state();

            // all instances should have a spawnpoint later than before the restart
            {
               auto found = local::find::alias( state.executables, "sleep");
               ASSERT_TRUE( found);
               EXPECT_TRUE( algorithm::all_of( found->instances, local::predicate::spawnpont( now)));
            }
            {
               auto found = local::find::alias( state.servers, "simple-server");
               ASSERT_TRUE( found);
               EXPECT_TRUE( algorithm::all_of( found->instances, local::predicate::spawnpont( now)));
            }
         }

      } // manager

   } // domain
} // casual

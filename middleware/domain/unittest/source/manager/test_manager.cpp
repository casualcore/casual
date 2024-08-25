//! 
//! Copyright (c) 2015, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!


#include "common/unittest.h"
#include "common/unittest/file.h"

#include "domain/unittest/utility.h"
#include "domain/manager/state.h"
#include "domain/manager/configuration.h"
#include "domain/manager/admin/model.h"
#include "domain/manager/admin/server.h"
#include "domain/unittest/manager.h"
#include "domain/unittest/internal/call.h"

#include "common/string.h"
#include "common/environment.h"
#include "common/service/lookup.h"
#include "common/service/type.h"
#include "common/event/listen.h"
#include "common/execute.h"

#include "configuration/model/load.h"
#include "configuration/model/transform.h"


#include <fstream>

namespace casual
{
   using namespace common;

   using Contract = common::service::execution::timeout::contract::Type;

   namespace domain::manager
   {

      namespace local
      {
         namespace
         {

            namespace expected::size
            {
               constexpr auto servers = 2;
            } // expected::size


            namespace configuration
            {
               template< typename... C>
               auto load( C&&... contents)
               {
                  auto files = common::unittest::file::temporary::contents( ".yaml", std::forward< C>( contents)...);

                  auto get_path = []( auto& file){ return static_cast< std::filesystem::path>( file);};

                  return casual::configuration::model::load( common::algorithm::transform( files, get_path));
               }

            } // configuration


            template< typename... C>
            auto domain( C&&... configurations)
            {
               return casual::domain::unittest::manager( std::forward< C>( configurations)...);
            }

            namespace find
            {
               auto alias = []( auto& entities, auto&& alias)
               {
                  return algorithm::find_if( entities, [alias]( auto& entity)
                  {
                     return entity.alias == alias;
                  });
               };
            } // find

         } // <unnamed>
      } // local

      TEST( domain_manager, empty_configuration__expect_boot)
      {
         common::unittest::Trace trace;

         constexpr auto empty =  R"(
domain:
   name: empty
)";

         EXPECT_NO_THROW( {
            auto domain = local::domain( empty);
         });
      }

      TEST( domain_manager, echo_configuration__expect_boot)
      {
         common::unittest::Trace trace;


         constexpr auto echo =  R"(
domain:
   name: echo
   executables:
      -  path: echo
         arguments: [ "casual" ]
         instances: 4    

)";

         EXPECT_NO_THROW( {
            auto domain = local::domain( echo);
         });
      }


      TEST( domain_manager, non_existing_path___expect_boot)
      {
         common::unittest::Trace trace;

         constexpr auto configuration = R"(
domain:
   name: echo
   executables:
      -  path: non-existing-e53ce069de5f49e6a2f546ad8e175093
         instances: 1    

)";

         EXPECT_NO_THROW( {
            auto domain = local::domain( configuration);
         });
      }

      TEST( domain_manager, non_existing_path__restart___expect_restart_ignored_during_boot)
      {
         common::unittest::Trace trace;

         constexpr auto configuration = R"(
domain:
   name: echo
   executables:
      -  path: non-existing-e53ce069de5f49e6a2f546ad8e175093
         instances: 1
         restart: true

)";

         EXPECT_NO_THROW( {
            auto domain = local::domain( configuration);
         });
      }


      TEST( domain_manager, echo_restart_configuration__expect_boot)
      {
         common::unittest::Trace trace;

         constexpr auto echo_restart =  R"(
domain:
   name: echo_restart
   executables:
      -  path: echo
         instances: 4
         arguments: [poop]
         restart: true    

)";

         EXPECT_NO_THROW( {
            auto domain = local::domain( echo_restart);
         });
      }


      TEST( domain_manager, sleep_configuration__expect_boot)
      {
         common::unittest::Trace trace;

         constexpr auto sleep =  R"(
domain:
   name: sleep
   executables:
      -  path: sleep
         arguments: [60]
         instances: 4
)";

         EXPECT_NO_THROW( {
            auto domain = local::domain( sleep);
         });
      }

      TEST( domain_manager, non_existent_executable__expect_boot)
      {
         common::unittest::Trace trace;

         constexpr auto configuration{ R"(
domain:
   name: sleep
   executables:
      -  path: a360ec9bec0b4e5fae131c0e7ad931f4-non-existent
         instances: 1

)"};

         EXPECT_NO_THROW( {
            auto domain = local::domain( configuration);
         });
      }


      TEST( domain_manager, unordered_dependent_groups)
      {
         common::unittest::Trace trace;

         constexpr auto configuration{ R"(
domain:
   name: groups
   groups:
      -  name: A
         dependencies: [ C]
      -  name: C
   executables:
      -  path: a360ec9bec0b4e5fae131c0e7ad931f4-non-existent
         instances: 1
         memberships: [ A]
)"};

         EXPECT_NO_THROW( {
            auto domain = local::domain( configuration);
         });
      }



      namespace local
      {
         namespace
         {
            //! we need to use _internal call_ stuff, since we don't have access to service-manager
            namespace call
            {
               admin::model::State state()
               {
                  return unittest::internal::call< admin::model::State>( admin::service::name::state);
               }

               auto scale( const std::vector< admin::model::scale::Alias>& aliases)
               {
                  return unittest::internal::call< std::vector< strong::correlation::id>>( admin::service::name::scale::aliases, aliases);
               }

               auto scale( const std::string& alias, platform::size::type instances)
               {
                  return scale( { { alias, instances}});
               }

               namespace restart
               {
                  auto aliases( const std::vector< admin::model::restart::Alias>& aliases)
                  {
                     return unittest::internal::call< std::vector< strong::correlation::id>>( admin::service::name::restart::aliases, aliases);
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

                     return unittest::internal::call< std::vector< strong::correlation::id>>( admin::service::name::restart::groups, algorithm::transform( groups, transform));
                  }
               } // restart
            } // call

            //! we need to use _internal call_ stuff, since we don't have access to service-manager
            namespace fetch
            {
               constexpr auto until = common::unittest::fetch::until( &call::state);
            } // fetch

            
            namespace configuration
            {

               constexpr auto long_running_processes_5 = R"(
domain:
   name: long_running_processes_5
   executables:
      -  alias: sleep
         path: sleep
         arguments: [60]
         instances: 5    

)";

            } // configuration

         } // <unnamed>
      } // local
      
      TEST( domain_manager, state_contains_domain_identity)
      {
         common::unittest::Trace trace;

         constexpr auto sleep =  R"(
domain:
   name: sleep
   executables:
      -  path: sleep
         arguments: [60]
         instances: 4    

)";

         auto domain = local::domain( sleep);

         auto state = local::call::state();

         EXPECT_TRUE( ! state.identity.name.empty());
         EXPECT_TRUE( state.identity.id);
      }

      TEST( domain_manager, state_long_running_processes_5__expect_5)
      {
         common::unittest::Trace trace;

         auto domain = local::domain( local::configuration::long_running_processes_5);

         auto state = local::fetch::until( casual::domain::unittest::fetch::predicate::alias::has::instances( "sleep", 5));

         ASSERT_TRUE( state.servers.size() == local::expected::size::servers) << CASUAL_NAMED_VALUE( state);
         EXPECT_TRUE( state.servers.at( 0).instances.size() == 1) << CASUAL_NAMED_VALUE( state);
      }



      TEST( domain_manager, state_long_running_processes_5__scale_out_to_10___expect_10)
      {
         common::unittest::Trace trace;

         auto domain = local::domain( R"(
domain:
   name: long_running_processes_5
   executables:
      -  alias: sleep
         path: sleep
         arguments: [60]
         instances: 5
)");

         local::fetch::until( casual::domain::unittest::fetch::predicate::alias::has::instances( "sleep", 5));

         auto tasks = local::call::scale( "sleep", 10);
         ASSERT_TRUE( ! tasks.empty());

         local::fetch::until( casual::domain::unittest::fetch::predicate::alias::has::instances( "sleep", 10));
      }

      TEST( domain_manager, long_running_processes_5__scale_in_to_0___expect_0)
      {
         common::unittest::Trace trace;

         auto domain = local::domain( local::configuration::long_running_processes_5);

         local::fetch::until( casual::domain::unittest::fetch::predicate::alias::has::instances( "sleep", 5));

         auto tasks = local::call::scale( "sleep", 0);
         ASSERT_TRUE( ! tasks.empty());
         
         local::fetch::until( casual::domain::unittest::fetch::predicate::alias::has::instances( "sleep", 0));

      }


      TEST( domain_manager, state_simple_server__expect_boot)
      {
         common::unittest::Trace trace;

         constexpr auto configuration = R"(
domain:
   name: simple-server
   servers:
      -  path: ./bin/test-simple-server
         instances: 1

)";

         auto domain = local::domain( configuration);

         auto state = local::call::state();

         ASSERT_TRUE( local::find::alias( state.servers, "test-simple-server")) << CASUAL_NAMED_VALUE( state);
         EXPECT_TRUE( local::find::alias( state.servers, "test-simple-server")->alias == "test-simple-server");

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
      -  path: ./bin/test-simple-server
         alias: foo
         instances: 1
)";

         auto domain = local::domain( configuration);

         auto state = local::call::state();

         ASSERT_TRUE( local::find::alias( state.servers, "foo"));

         auto server = local::get_global_state( local::find::alias( state.servers, "foo")->instances.at( 0).handle);
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
      -  path: ./bin/test-simple-server
         alias: foo
         instances: 10
)";

         auto domain = local::domain( configuration);

         auto state = local::call::state();

         ASSERT_TRUE( local::find::alias( state.servers, "foo"));
         ASSERT_TRUE( local::find::alias( state.servers, "foo")->instances.size() == 10);

         auto index = 0;

         auto order_spawnpoint = []( auto& l, auto& r){ return l.spawnpoint < r.spawnpoint;};

         for( auto& instance : algorithm::sort( local::find::alias( state.servers, "foo")->instances, order_spawnpoint))
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
      -  path: ./bin/test-simple-server
         instances: 1
         environment:
            variables: 
               -  key: PARENT
                  value: foo
               -  key: CHILD
                  value: "${PARENT}/bar"

)";

         auto domain = local::domain( configuration);

         auto state = local::call::state();

         ASSERT_TRUE( local::find::alias( state.servers, "test-simple-server")) << CASUAL_NAMED_VALUE( state);
         EXPECT_TRUE( local::find::alias( state.servers, "test-simple-server")->instances.size() == 1) << CASUAL_NAMED_VALUE( state);

         auto simple = local::find::alias( state.servers, "test-simple-server");

         // get environment variables
         auto checker = local::get_variable_checker( simple->instances.at( 0).handle);
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
      -  path: ./bin/test-simple-server
         instances: 1
)";

         auto domain = local::domain( configuration);

         auto state = local::call::state();
         ASSERT_TRUE( local::find::alias( state.servers, "test-simple-server")) << CASUAL_NAMED_VALUE( state);

         auto& instance = local::find::alias( state.servers, "test-simple-server")->instances.at( 0);

         common::signal::send( instance.handle.pid, common::code::signal::hangup);

         // check that the signal has been received.
         // There are no guarantees when the signal is received, just that it will be
         // eventually, so we need to loop...
         while( true)
         {
            auto checker = local::get_variable_checker( instance.handle);
            if( checker( "CASUAL_SIMPLE_SERVER_HANGUP_SIGNAL", "true"))
               break;

            process::sleep( std::chrono::milliseconds{ 1});
         }

         EXPECT_TRUE( common::communication::instance::ping( instance.handle.ipc) == instance.handle);

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
      -  path: ./bin/test-simple-server
         instances: 1
)";

         auto domain = local::domain( configuration);

         auto state_before = local::call::state();
         ASSERT_TRUE( local::find::alias( state_before.servers, "test-simple-server")) << CASUAL_NAMED_VALUE( state_before);

         auto& target = local::find::alias( state_before.servers, "test-simple-server")->instances.at( 0).handle.pid;

         // setup subscription to see when hit is done
         message::event::process::Exit died;
         common::event::subscribe( common::process::handle(), { died.type()});

         // order hit
         message::event::process::Assassination assassination;
         assassination.target = target;
         assassination.contract = Contract::kill;
         assassination.announcement = "Revenge is a dish that tastes best when it is cold.";
         communication::device::blocking::send( communication::instance::outbound::domain::manager::device(), assassination);

         // check if/when hit is performed
         common::communication::device::blocking::receive( common::communication::ipc::inbound::device(), died);
      
         auto state_after = local::call::state();

         ASSERT_TRUE( local::find::alias( state_after.servers, "test-simple-server")) << CASUAL_NAMED_VALUE( state_after);
         ASSERT_TRUE( local::find::alias( state_after.servers, "test-simple-server")->instances.size() == 1) << CASUAL_NAMED_VALUE( state_after);
         
         EXPECT_TRUE( local::find::alias( state_after.servers, "test-simple-server")->instances.at( 0).state == admin::model::instance::State::exit) << CASUAL_NAMED_VALUE( state_after);

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
      -  path: ./bin/test-simple-server
         alias: foo
         instances: 1
)";

         auto domain = local::domain( configuration);

         auto state_before = local::call::state();
         ASSERT_TRUE( local::find::alias( state_before.servers, "foo"))  << CASUAL_NAMED_VALUE( state_before);

         auto target = local::find::alias( state_before.servers, "foo")->instances.at( 0).handle.pid;

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

         {
            auto state = local::call::state();

            ASSERT_TRUE( local::find::alias( state.servers, "foo")) << CASUAL_NAMED_VALUE( state);
            ASSERT_TRUE( local::find::alias( state.servers, "foo")->instances.size() == 1) << CASUAL_NAMED_VALUE( state);
            
            EXPECT_TRUE( local::find::alias( state.servers, "foo")->instances.at( 0).state == admin::model::instance::State::exit) << CASUAL_NAMED_VALUE( state);

            // is correct target killed
            EXPECT_TRUE( assassination.target == died.state.pid) << CASUAL_NAMED_VALUE( assassination) << '\n' << CASUAL_NAMED_VALUE( died);
         }

      }

      TEST( domain_manager, simple_server__assassinate_process_linger)
      {
         common::unittest::Trace trace;

         constexpr auto configuration = R"(
domain:
   name: simple-server
   servers:
      -  path: ./bin/test-simple-server
         alias: foo
         instances: 1
)";

         auto domain = local::domain( configuration);

         auto state_before = local::call::state();
         ASSERT_TRUE( local::find::alias( state_before.servers, "foo")) << CASUAL_NAMED_VALUE( state_before);

         auto& target = local::find::alias( state_before.servers, "foo")->instances.at( 0).handle.pid;

         // order wrongfull hit
         message::event::process::Assassination assassination;
         assassination.target = target;
         assassination.contract = Contract::linger;
         communication::device::blocking::send( communication::instance::outbound::domain::manager::device(), assassination);

         process::sleep( std::chrono::milliseconds{ 500});

         {
            auto state = local::call::state();

            ASSERT_TRUE( local::find::alias( state.servers, "foo")) << CASUAL_NAMED_VALUE( state);
            ASSERT_TRUE( local::find::alias( state.servers, "foo")->instances.size() == 1) << CASUAL_NAMED_VALUE( state);
            
            EXPECT_TRUE( local::find::alias( state.servers, "foo")->instances.at( 0).state == admin::model::instance::State::running) << CASUAL_NAMED_VALUE( state);
         }
      }

      //! We need to rethink this test case. It might be to hard to pretend to be service-manager.
      //! It should be possible to make it work though :)
      TEST( DISABLED_domain_manager, scale_in___expect__prepare_shutdown_to_service_manager)
      {
         common::unittest::Trace trace;

         constexpr auto configuration = R"(
domain:
   name: simple-server
   servers:
      -  alias: foo
         path: ./bin/test-simple-server
         instances: 2
)";

         auto domain = local::domain( configuration);

         // pretend that we're the service-manager
         // ...since we don't have access to service-manager when we build
         // domain-manager
         communication::instance::whitelist::connect( communication::instance::identity::service::manager);

         // domain-manager will advertise it's services
         {
            auto message = communication::ipc::receive< common::message::service::Advertise>();
            EXPECT_TRUE( domain.handle() == message.process);
         }

         // setup subscription to see when hit is done
         common::event::subscribe( common::process::handle(), { message::event::process::Exit::type()});

         auto tasks = local::call::scale( "foo", 1);
         ASSERT_TRUE( tasks.size() == 1) << "tasks: " << CASUAL_NAMED_VALUE( tasks);

 
         // Consume the prepare::shutdown::Request
         auto send_reply = []()
         {
            auto request = communication::ipc::receive< message::domain::process::prepare::shutdown::Request>();
            EXPECT_TRUE( request.processes.size() == 1) << CASUAL_NAMED_VALUE( request);

            return [ request = std::move( request)]()
            {
               auto reply = message::reverse::type( request);
               reply.processes = std::move( request.processes);
               communication::device::blocking::send( request.process.ipc, reply);
            };
         }();


         // check that no instances has been shut downed.
         {
            // give some time for a possible bug to occur.
            process::sleep( std::chrono::milliseconds{ 1});

            auto state = local::call::state();
            auto found = algorithm::find( state.servers, "foo");
            ASSERT_TRUE( found);
            EXPECT_TRUE( found->instances.size() == 2);

            // we expect no exit event
            message::event::process::Exit event;
            EXPECT_TRUE( ! communication::device::non::blocking::receive( communication::ipc::inbound::device(), event));
         }

         send_reply();


         // check that one instances has been shut downed.
         {
            // we expect exit event
            communication::ipc::receive< message::event::process::Exit>();

            auto state = local::call::state();
            auto found = algorithm::find( state.servers, "foo");
            ASSERT_TRUE( found);
            EXPECT_TRUE( found->instances.size() == 1);
         }

         // make sure we 'disconnect' our self as service-manager before shutdown
         // otherwise domain-manager will wait for approval from service-manager before
         // each shutdown.
         {
            message::event::process::Exit event;
            event.state.pid = common::process::id(); 
            event.state.reason = decltype( event.state.reason)::exited;
            communication::device::blocking::send( domain.handle().ipc, event);
         };
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
      -  alias: sleepA1
         path: sleep
         arguments: [60]
         instances: 1
         memberships: [groupA]    
      -  alias: sleepA2
         path: sleep
         arguments: [60]
         instances: 1
         memberships: [groupA]
      -  alias: sleepA3
         path: sleep
         arguments: [60]
         instances: 1
         memberships: [groupA]
      -  alias: sleepA4
         path: sleep
         arguments: [60]
         instances: 1
         memberships: [groupA]
      -  alias: sleepA5
         path: sleep
         arguments: [60]
         instances: 1
         memberships: [groupA]

      -  alias: sleepB1
         path: sleep
         arguments: [60]
         instances: 1
         memberships: [groupB]    
      -  alias: sleepB2
         path: sleep
         arguments: [60]
         instances: 1
         memberships: [groupB]
      -  alias: sleepB3
         path: sleep
         arguments: [60]
         instances: 1
         memberships: [groupB]
      -  alias: sleepB4
         path: sleep
         arguments: [60]
         instances: 1
         memberships: [groupB]
      -  alias: sleepB5
         path: sleep
         arguments: [60]
         instances: 1
         memberships: [groupB]

      -  alias: sleepC1
         path: sleep
         arguments: [60]
         instances: 1
         memberships: [groupC]    
      -  alias: sleepC2
         path: sleep
         arguments: [60]
         instances: 1
         memberships: [groupC]
      -  alias: sleepC3
         path: sleep
         arguments: [60]
         instances: 1
         memberships: [groupC]
      -  alias: sleepC4
         path: sleep
         arguments: [60]
         instances: 1
         memberships: [groupC]
      -  alias: sleepC5
         path: sleep
         arguments: [60]
         instances: 1
         memberships: [groupC]

      -  alias: sleepD1
         path: sleep
         arguments: [60]
         instances: 1
         memberships: [groupD]    
      -  alias: sleepD2
         path: sleep
         arguments: [60]
         instances: 1
         memberships: [groupD]
      -  alias: sleepD3
         path: sleep
         arguments: [60]
         instances: 1
         memberships: [groupD]
      -  alias: sleepD4
         path: sleep
         arguments: [60]
         instances: 1
         memberships: [groupD]
      -  alias: sleepD5
         path: sleep
         arguments: [60]
         instances: 1
         memberships: [groupD]
)"};

         auto domain = local::domain( configuration);

         auto instance_count = []( auto count)
         { 
            return [ count]( auto& executable)
            { 
               return range::size( executable.instances) == count && algorithm::all_of( executable.instances, []( auto& instance){ return predicate::boolean( instance);});
            };
         };

         auto state = local::fetch::until( [ instance_count]( auto& state)
         {
            return state.executables.size() == 4 * 5 && algorithm::all_of( state.executables, instance_count( 1));
         });


         local::call::scale( algorithm::transform( state.executables, []( auto& executable)
         {
            return admin::model::scale::Alias{ executable.alias, 10};
         }));

         
         local::fetch::until( [ instance_count]( auto& state)
         {
            return state.executables.size() == 4 * 5 && algorithm::all_of( state.executables, instance_count( 10));
         });
      }

      namespace local
      {
         namespace
         {
            template< typename C>
            auto event_listen_call( C caller)
            {
               std::vector< strong::correlation::id> result;

               auto condition = event::condition::compose( 
                  event::condition::prelude( [&result, &caller](){ result = caller();}), 
                  event::condition::done( [&result](){ return result.empty();})
               );

               // start and listen for events
               event::listen( condition, 
                  [ &result]( const message::event::Task& task)
                  {
                     if( task.done())
                        if( algorithm::find( result, task.correlation))
                           result.clear();
                  });
            }
      

            namespace predicate
            {
               auto spawnpoint = []( auto& timepoint)
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
      -  alias: sleep
         path: sleep
         arguments: [60]
         instances: 2
)";

         auto domain = local::domain( configuration);

         auto now = platform::time::clock::type::now();

         local::event_listen_call( [](){ return local::call::restart::aliases( { "sleep"});});

         auto state = local::call::state();

         auto found = local::find::alias( state.executables, "sleep");

         ASSERT_TRUE( found.size() == 1) << CASUAL_NAMED_VALUE( found);
         // all instances should have a spawnpoint later than before the restart
         EXPECT_TRUE( algorithm::all_of( found->instances, local::predicate::spawnpoint( now))) << CASUAL_NAMED_VALUE( found) << "\n" << CASUAL_NAMED_VALUE( now);
      }


      TEST( domain_manager, restart_server)
      {
         common::unittest::Trace trace;

         constexpr auto configuration = R"(
domain:
   name: simple-server
   servers:
      -  path: ./bin/test-simple-server
         alias: foo
         instances: 2

)";

         auto domain = local::domain( configuration);

         auto now = platform::time::clock::type::now();

         local::event_listen_call( [](){ return local::call::restart::aliases( { "foo"});});

         auto state = local::call::state();

         auto found = local::find::alias( state.servers, "foo");

         ASSERT_TRUE( found);
         // all instances should have a spawnpoint later than before the restart
         EXPECT_TRUE( algorithm::all_of( found->instances, local::predicate::spawnpoint( now))) << CASUAL_NAMED_VALUE( found) << "\n" << CASUAL_NAMED_VALUE( now);
      }


      TEST( domain_manager, restart_group_A__expect_restarted_others_untouched)
      {
         common::unittest::Trace trace;

         constexpr auto configuration = R"(
domain:
   name: restart_groups

   groups:
      -  name: A
      -  name: B
         dependencies: [ A]
      -  name: C
         dependencies: [ B]

   servers:
      -  path: ./bin/test-simple-server
         alias: foo
         instances: 2
         memberships: [ A]

   executables:
      -  alias: sleep
         path: sleep
         arguments: [60]
         instances: 2
         memberships: [ B]

)";

         auto domain = local::domain( configuration);

         auto now = platform::time::clock::type::now();

         local::event_listen_call( [](){ return local::call::restart::groups( { "A"});});

         auto state = local::call::state();

         auto found = local::find::alias( state.servers, "foo");

         ASSERT_TRUE( found) << CASUAL_NAMED_VALUE( state);
         // all instances should have a spawnpoint later than before the restart
         EXPECT_TRUE( algorithm::all_of( found->instances, local::predicate::spawnpoint( now))) << CASUAL_NAMED_VALUE( found) << "\n" << CASUAL_NAMED_VALUE( now);

         // other instances should be untouched
         {
            auto found = local::find::alias( state.executables, "sleep");
            ASSERT_TRUE( found);
            EXPECT_TRUE( algorithm::none_of( found->instances, local::predicate::spawnpoint( now)));
         }
      }

      TEST( domain_manager, restart_group_B__expect_restarted_others_untouched)
      {
         common::unittest::Trace trace;

         constexpr auto configuration = R"(
domain:
   name: restart_groups

   groups:
      -  name: A
      -  name: B
         dependencies: [ A]
      -  name: C
         dependencies: [ B]

   servers:
      -  path: ./bin/test-simple-server
         alias: simple-server
         instances: 2
         memberships: [ A]

   executables:
      -  alias: sleep
         path: sleep
         arguments: [60]
         instances: 2
         memberships: [ B]

)";

         auto domain = local::domain( configuration);

         auto now = platform::time::clock::type::now();

         local::event_listen_call( [](){ return local::call::restart::groups( { "B"});});

         auto state = local::call::state();

         // all instances should have a spawnpoint later than before the restart
         {
            auto found = local::find::alias( state.executables, "sleep");
            ASSERT_TRUE( found);
            EXPECT_TRUE( algorithm::all_of( found->instances, local::predicate::spawnpoint( now)));
         }

         // other instances should be untouched
         {
            auto found = local::find::alias( state.servers, "simple-server");
            ASSERT_TRUE( found);
            EXPECT_TRUE( algorithm::none_of( found->instances, local::predicate::spawnpoint( now)));
         }
      }


      TEST( domain_manager, restart_group_A_B___expect_all_restarted)
      {
         common::unittest::Trace trace;

         constexpr auto configuration = R"(
domain:
   name: restart_groups

   groups:
      -  name: A
      -  name: B
         dependencies: [ A]
      -  name: C
         dependencies: [ B]

   servers:
      -  path: ./bin/test-simple-server
         alias: simple-server
         instances: 2
         memberships: [ A]

   executables:
      -  alias: sleep
         path: sleep
         arguments: [60]
         instances: 2
         memberships: [ B]
)";

         auto domain = local::domain( configuration);

         auto now = platform::time::clock::type::now();

         local::event_listen_call( [](){ return local::call::restart::groups( { "A", "B"});});

         auto state = local::call::state();

         // all instances should have a spawnpoint later than before the restart
         {
            auto found = local::find::alias( state.executables, "sleep");
            ASSERT_TRUE( found);
            EXPECT_TRUE( algorithm::all_of( found->instances, local::predicate::spawnpoint( now)));
         }
         {
            auto found = local::find::alias( state.servers, "simple-server");
            ASSERT_TRUE( found);
            EXPECT_TRUE( algorithm::all_of( found->instances, local::predicate::spawnpoint( now)));
         }
      }

      TEST( domain_manager, restart_no_group___expect_all_restarted)
      {
         common::unittest::Trace trace;

         constexpr auto configuration = R"(
domain:
   name: restart_groups

   groups:
      -  name: A
      -  name: B
         dependencies: [ A]
      -  name: C
         dependencies: [ B]

   servers:
      -  path: ./bin/test-simple-server
         alias: simple-server
         instances: 2
         memberships: [ A]

   executables:
      -  alias: sleep
         path: sleep
         arguments: [60]
         instances: 2
         memberships: [ B]

)";

         auto domain = local::domain( configuration);

         auto now = platform::time::clock::type::now();

         local::event_listen_call( [](){ return local::call::restart::groups( {});});

         auto state = local::call::state();

         // all instances should have a spawnpoint later than before the restart
         {
            auto found = local::find::alias( state.executables, "sleep");
            ASSERT_TRUE( found);
            EXPECT_TRUE( algorithm::all_of( found->instances, local::predicate::spawnpoint( now)));
         }
         {
            auto found = local::find::alias( state.servers, "simple-server");
            ASSERT_TRUE( found);
            EXPECT_TRUE( algorithm::all_of( found->instances, local::predicate::spawnpoint( now)));
         }
      }

      TEST( domain_manager, configuration_get)
      {
         common::unittest::Trace trace;

         constexpr auto configuration = R"(
domain:
   name: restart_groups

   groups:
      -  name: A
      -  name: B
         dependencies: [ A]
      -  name: C
         dependencies: [ B]

   servers:
      -  path: ./bin/test-simple-server
         alias: simple-server
         instances: 2
         memberships: [ A]
         restart: true

   executables:
      -  alias: sleep
         path: sleep
         arguments: [60]
         instances: 2
         memberships: [ B]
         restart: true

)";

         auto domain = local::domain( configuration);

         auto origin = local::configuration::load( configuration);

         auto model = casual::configuration::model::transform( unittest::internal::call< casual::configuration::user::Model>( admin::service::name::configuration::get));

         EXPECT_TRUE( origin.domain == model.domain) << CASUAL_NAMED_VALUE( origin.domain) << "\n " << CASUAL_NAMED_VALUE( model.domain);
      }

      namespace local
      {
         namespace
         {
            namespace call
            {
               auto configuration( auto wanted, auto service)
               {
                  local::event_listen_call( [ wanted, service]()
                  { 
                     return unittest::internal::call< std::vector< common::strong::correlation::id>>( service, wanted);
                  });

                  // return the new configuration model
                  return casual::configuration::model::transform( unittest::internal::call< casual::configuration::user::Model>( admin::service::name::configuration::get));
               }

               // post helper - post the wanted, we need to listen to events to know when it's done.
               // @returns the new configuration model transformed to internal model
               auto post( auto wanted)
               {
                  return configuration( wanted, admin::service::name::configuration::post);
               }

               auto put( auto wanted)
               {
                  return configuration( wanted, admin::service::name::configuration::put);
               }
               
            } // call
         } // <unnamed>
      } // local


      TEST( domain_manager, configuration_post_add_modify)
      {
         common::unittest::Trace trace;

         auto domain = local::domain( R"(
domain:
   name: post

   groups:
      -  name: B
         dependencies: [ A]
      -  name: C
         dependencies: [ B]
      -  name: Y
         dependencies: [ A]
      -  name: A

   servers:
      -  path: ./bin/test-simple-server
         alias: simple-server
         instances: 2
         memberships: [ A]

   executables:
      -  alias: sleep
         path: sleep
         arguments: [60]
         instances: 2
         memberships: [ B]

)");

         auto wanted = local::configuration::load( R"(
domain:
   name: post

   groups:
      -  name: B
         dependencies: [ A]
      -  name: C
         dependencies: [ X]
      -  name: A
      -  name: X
         dependencies: [ B]

   servers:
      -  path: ./bin/test-simple-server
         alias: simple-server
         instances: 1
         memberships: [ X]
      -  path: ./bin/test-simple-server
         alias: simple-server-added
         instances: 1
         memberships: [ X]

   executables:
      -  alias: sleep
         path: sleep
         arguments: [60]
         instances: 2
         memberships: [ X]
      -  alias: sleep_add
         path: sleep
         arguments: [60]
         instances: 1
         memberships: [ B]

)");

         auto origin = casual::configuration::model::transform( unittest::internal::call< casual::configuration::user::Model>( admin::service::name::configuration::get));
         EXPECT_TRUE( origin.domain != wanted.domain) << CASUAL_NAMED_VALUE( origin.domain) << "\n " << CASUAL_NAMED_VALUE( wanted.domain);

         auto updated = local::call::post( casual::configuration::model::transform( wanted));
         EXPECT_TRUE( updated.domain == wanted.domain) << CASUAL_NAMED_VALUE( updated.domain) << "\n " << CASUAL_NAMED_VALUE( wanted.domain);
         
      }

      TEST( domain_manager, configuration_post_remove_modify)
      {
         common::unittest::Trace trace;

         auto domain = local::domain( R"(
domain:
   name: post
   groups:
      -  name: A
   servers:
      -  path: ./bin/test-simple-server
         alias: simple-server
         instances: 2
         memberships: [ A]
   executables:
      -  alias: sleep
         path: sleep
         arguments: [60]
         instances: 2
         memberships: [ A]

)");


         auto wanted = local::configuration::load( R"(
domain:
   name: post
   groups:
      -  name: A
      -  name: B
         dependencies: [ A]
   servers:
      -  path: ./bin/test-simple-server
         alias: replaced-server
         instances: 3
         memberships: [ B]
   executables:
      -  alias: sleep
         path: sleep
         arguments: [60]
         instances: 1
         memberships: [ B]

)");


         auto origin = casual::configuration::model::transform( unittest::internal::call< casual::configuration::user::Model>( admin::service::name::configuration::get));
         EXPECT_TRUE( origin.domain != wanted.domain) << CASUAL_NAMED_VALUE( origin.domain) << "\n " << CASUAL_NAMED_VALUE( wanted.domain);

         auto updated = local::call::post( casual::configuration::model::transform( wanted));
         EXPECT_TRUE( updated.domain == wanted.domain) << CASUAL_NAMED_VALUE( updated.domain) << "\n " << CASUAL_NAMED_VALUE( wanted.domain);  
      }

      namespace local
      {
         namespace
         {
            auto instance_state( auto& executables, auto alias, auto state)
            {
               if( auto found = algorithm::find( executables, alias))
               {
                  return algorithm::all_of( found->instances, [ state]( auto& instance)
                  {
                     return instance.state == state;
                  });
               }

               return false;
            };

            auto instance_count( auto& executables, auto alias, platform::size::type count)
            {
               if( auto found = algorithm::find( executables, alias))
                  return std::ssize( found->instances) == count;

               return false;
            };
         } // <unnamed>
      } // local

      TEST( domain_manager, configuration_put__enable_group)
      {
         common::unittest::Trace trace;

         auto domain = local::domain( R"(
domain:
   name: post
   groups:
      -  name: A
      -  name: B
         enabled: false
         dependencies: [ A]
      -  name: C
         dependencies: [ B]
   executables:
      -  alias: a
         path: sleep
         arguments: [60]
         instances: 2
         memberships: [ A]
      -  alias: b
         path: sleep
         arguments: [60]
         instances: 2
         memberships: [ B]
      -  alias: c
         path: sleep
         arguments: [60]
         instances: 2
         memberships: [ C]

)");


         // check that b and c does not have any instances
         {
            auto state = local::call::state();
            EXPECT_TRUE( local::instance_state( state.executables, "a", manager::admin::model::instance::State::running));
            EXPECT_TRUE( local::instance_count( state.executables, "a", 2));
            EXPECT_TRUE( local::instance_state( state.executables, "b", manager::admin::model::instance::State::disabled));
            EXPECT_TRUE( local::instance_count( state.executables, "b", 2));
            EXPECT_TRUE( local::instance_state( state.executables, "c", manager::admin::model::instance::State::disabled));
            EXPECT_TRUE( local::instance_count( state.executables, "c", 2));
         }


         auto wanted = local::configuration::load( R"(
domain:
   groups:
      -  name: B
         enabled: true
         dependencies: [ A]
)");

         local::call::put( casual::configuration::model::transform( wanted));

         {
            auto state = local::call::state();
            EXPECT_TRUE( local::instance_state( state.executables, "a", manager::admin::model::instance::State::running));
            EXPECT_TRUE( local::instance_count( state.executables, "a", 2));
            EXPECT_TRUE( local::instance_state( state.executables, "b", manager::admin::model::instance::State::running));
            EXPECT_TRUE( local::instance_count( state.executables, "b", 2));
            EXPECT_TRUE( local::instance_state( state.executables, "c", manager::admin::model::instance::State::running));
            EXPECT_TRUE( local::instance_count( state.executables, "c", 2));
         }
          
      }

      TEST( domain_manager, configuration_error_path__put_sleep)
      {
         common::unittest::Trace trace;

         auto domain = local::domain( R"(
domain:
   name: post
   groups:
      -  name: A
      -  name: B
         enabled: false
         dependencies: [ A]
   executables:
      -  alias: a
         path: non-existent-path
         arguments: [60]
         instances: 2
         memberships: [ A]
      -  alias: b
         path: sleep
         arguments: [60]
         instances: 2
         memberships: [ B]
)");


         // check that b and c does not have any instances
         {
            auto state = local::call::state();
            EXPECT_TRUE( local::instance_state( state.executables, "a", manager::admin::model::instance::State::error));
            EXPECT_TRUE( local::instance_count( state.executables, "a", 2));
            EXPECT_TRUE( local::instance_state( state.executables, "b", manager::admin::model::instance::State::disabled));
            EXPECT_TRUE( local::instance_count( state.executables, "b", 2));
         }


         auto wanted = local::configuration::load( R"(
domain:
   groups:
      -  name: B
         enabled: true
         dependencies: [ A]
   executables:
      -  alias: a
         path: sleep
         arguments: [60]
         instances: 2
         memberships: [ A]
)");

         local::call::put( casual::configuration::model::transform( wanted));

         {
            auto state = local::call::state();
            EXPECT_TRUE( local::instance_state( state.executables, "a", manager::admin::model::instance::State::running));
            EXPECT_TRUE( local::instance_count( state.executables, "a", 2));
            EXPECT_TRUE( local::instance_state( state.executables, "b", manager::admin::model::instance::State::running));
            EXPECT_TRUE( local::instance_count( state.executables, "b", 2));
         }
      }

      namespace local
      {
         namespace
         {
            namespace grandchild::expected
            {
               const std::string alias = "some_alias";
               const std::filesystem::path path = "c/a/s/u/a/l";
            } // grandchild::expected
         }
      } // local

      TEST( domain_manager, connect_grandchild__expect_handle_alias_path)
      {
         common::unittest::Trace trace;

         auto domain = local::domain( R"(
domain:
   name: unittest
)");

         {
            auto state = local::call::state();
            EXPECT_TRUE( state.grandchildren.empty());
         }

         // We act as a grandchild and connect to the DM
         {
            auto request = common::message::domain::process::connect::Request{};
            request.information.alias = local::grandchild::expected::alias;
            request.information.path = local::grandchild::expected::path;
            request.information.handle.pid = common::process::id();
            communication::device::blocking::send( communication::instance::outbound::domain::manager::device(), request);
         }

         auto state = local::call::state();
         EXPECT_TRUE( state.grandchildren.size() == 1);

         auto grandchild = state.grandchildren.at( 0);
         EXPECT_TRUE( grandchild.handle == common::process::id());
         EXPECT_TRUE( grandchild.alias == local::grandchild::expected::alias);
         EXPECT_TRUE( grandchild.path == local::grandchild::expected::path);
      }

      TEST( domain_manager, connected_grandchild_exits__expect_removed)
      {
         common::unittest::Trace trace;

         auto domain = local::domain( R"(
domain:
   name: unittest
)");

         {
            auto state = local::call::state();
            EXPECT_TRUE( state.grandchildren.empty());
         }

         struct
         {
            communication::ipc::inbound::Device device;
            process::Handle handle = { strong::process::id{ process::id().value() - 1}, device.connector().handle().ipc()};
         } grandchild;


         // Connect to dm
         {
            auto request = common::message::domain::process::connect::Request{};
            request.information.handle = grandchild.handle;
            communication::device::blocking::send( communication::instance::outbound::domain::manager::device(), request);

            auto state = local::call::state();
            EXPECT_TRUE( state.grandchildren.size() == 1);
         }

         // We fake our own death
         {
            message::event::process::Exit event;
            event.state.pid = grandchild.handle.pid;
            event.state.reason = decltype( event.state.reason)::exited;
            communication::device::blocking::send( communication::instance::outbound::domain::manager::device(), event);
         }

         auto state = local::call::state();
         EXPECT_TRUE( state.grandchildren.empty());

      }

      TEST( domain_manager, request_process_information)
      {
         auto domain = local::domain( R"(
domain:
   name: unittest
)");


         struct
         {
            communication::ipc::inbound::Device device;
            process::Handle handle = { strong::process::id{ process::id().value() - 1}, device.connector().handle().ipc()};
         } grandchild;

         // Connect to dm
         {
            auto request = common::message::domain::process::connect::Request{};
            request.information.handle = grandchild.handle;
            request.information.alias = local::grandchild::expected::alias;
            request.information.path = local::grandchild::expected::path;
            communication::device::blocking::send( communication::instance::outbound::domain::manager::device(), request);
         }

         auto request = message::domain::process::information::Request{ process::handle()};
         // We lookup the grandchild...
         request.handles.push_back( grandchild.handle);
         // ...and the DM itself...
         request.handles.push_back( domain.handle());
         // ...as well as a non-existent pid.
         request.handles.push_back( common::process::Handle{ common::strong::process::id{}});

         auto processes = communication::ipc::call( communication::instance::outbound::domain::manager::device(), request).processes;

         // The invalid pid should not be included in the response since it's not a real process
         EXPECT_TRUE( processes.size() == 2) << CASUAL_NAMED_VALUE( processes);
         EXPECT_TRUE( algorithm::includes( processes, std::vector{ grandchild.handle.pid, domain.handle().pid}));

         auto find_process = []( const auto& processes, const auto& handle)
         {
            return algorithm::find_if( processes, [&handle]( const auto& process){ return process.handle.pid == handle;});
         };

         {
            auto process = find_process( processes, grandchild.handle);
            ASSERT_TRUE( process) << CASUAL_NAMED_VALUE( processes);
            EXPECT_TRUE( process->alias == local::grandchild::expected::alias) << process->alias;
            EXPECT_TRUE( process->path == local::grandchild::expected::path) << process->path;
         }

         auto dm = find_process( processes, domain.handle());
         ASSERT_TRUE( dm) << CASUAL_NAMED_VALUE( processes);
         EXPECT_TRUE( dm->alias == "casual-domain-manager") << dm->alias;
         EXPECT_TRUE( dm->path == "casual-domain-manager") << dm->path;

      }


      TEST( domain_manager, restart_executable_server__1_restarts)
      {
         common::unittest::Trace trace;

         constexpr auto sleep =  R"(
domain:
   name: sleep
   executables:
      -  path: sleep
         alias: sleep
         arguments: [60]
         instances: 1
         restart: true
   servers:
      -  path: ./bin/test-simple-server
         alias: simple-server
         instances: 1
         restart: true
)";

         auto domain = local::domain( sleep);
         message::event::process::Exit died;
         common::event::subscribe( common::process::handle(), { died.type()});

         auto order_hit = []( auto& state, auto& target)
         {
            message::event::process::Assassination assassination;
            assassination.target = target;
            assassination.contract = Contract::kill;
            communication::device::blocking::send( communication::instance::outbound::domain::manager::device(), assassination);
         };

         {
            auto state = local::call::state();

            auto executable = local::find::alias( state.executables, "sleep");
            auto server = local::find::alias( state.servers, "simple-server");
            ASSERT_TRUE( executable) << CASUAL_NAMED_VALUE( state);
            ASSERT_TRUE( server) << CASUAL_NAMED_VALUE( server);

            EXPECT_TRUE( executable->restarts == 0) << executable->restarts;
            EXPECT_TRUE( server->restarts == 0) << server->restarts;

            order_hit( state, executable->instances.at( 0).handle);
            order_hit( state, server->instances.at( 0).handle.pid);
         }

         // check if/when hit is performed
         algorithm::for_n< 2>([ &died]()
            {
               common::communication::device::blocking::receive( common::communication::ipc::inbound::device(), died);
            }
         );

         local::fetch::until( casual::domain::unittest::fetch::predicate::alias::has::instances( "sleep", 1));
         local::fetch::until( casual::domain::unittest::fetch::predicate::alias::has::instances( "simple-server", 1));

         {
            auto state = local::call::state();

            auto executable = local::find::alias( state.executables, "sleep");
            auto server = local::find::alias( state.servers, "simple-server");
            ASSERT_TRUE( executable) << CASUAL_NAMED_VALUE( state);
            ASSERT_TRUE( server) << CASUAL_NAMED_VALUE( server);

            EXPECT_TRUE( executable->restarts == 1) << executable->restarts;
            EXPECT_TRUE( server->restarts == 1) << server->restarts;
         }
      }

   } // domain::manager
} // casual

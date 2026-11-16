//! 
//! Copyright (c) 2015, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!


#include "common/unittest.h"

#include "service/manager/state.h"
#include "service/manager/admin/server.h"


#include <random>

namespace casual
{
   namespace service::manager
   {

      namespace local
      {
         namespace
         {
            auto state()
            {
               return manager::State{};
            }
            
         } // <unnamed>
      } // local
      
      TEST( service_manager_state, admin_services)
      {
         common::unittest::Trace trace;

         auto state = local::state();

         auto services = manager::admin::services( state);

         EXPECT_TRUE( casual::manager::service::name( services.at( 0)) == admin::service::name::state);
      }


      TEST( service_manager_state, advertise_empty_invalid_local__expect_no_op)
      {
         common::unittest::Trace trace;

         auto state = local::state();

         EXPECT_TRUE( state.update( common::message::service::Advertise{}).pending.empty());

         EXPECT_TRUE( state.instances.sequential.empty()) << CASUAL_NAMED_VALUE( state.instances.sequential);
         EXPECT_TRUE( state.instances.concurrent.empty());
      }

      TEST( service_manager_state, advertise_empty_invalid_concurrent__expect_no_op)
      {
         common::unittest::Trace trace;

         auto state = local::state();

         EXPECT_TRUE( state.update( common::message::service::concurrent::Advertise{}).empty());

         EXPECT_TRUE( state.instances.sequential.empty());
         EXPECT_TRUE( state.instances.concurrent.empty()) << CASUAL_NAMED_VALUE( state.instances.concurrent);
      }

      TEST( service_manager_state, advertise_local_service__expect_service_and_instance_added)
      {
         common::unittest::Trace trace;

         auto state = local::state();

         {
            common::message::service::Advertise message;
            message.process = common::process::handle();
            message.services.add.push_back( { .name = "service1"});
            EXPECT_TRUE( state.update( std::move( message)).pending.empty());
         }

         EXPECT_TRUE( state.instances.sequential.size() == 1);
         {
            auto service_id = state.services.lookup( "service1");
            ASSERT_TRUE( service_id) << CASUAL_NAMED_VALUE( state);
            EXPECT_TRUE( state.services[ service_id].information.name == "service1");
            ASSERT_TRUE( state.services[ service_id].instances.sequential().size() == 1);
            auto instance_id = state.services[ service_id].instances.sequential().at( 0);
            EXPECT_TRUE( state.instances.sequential[ instance_id].process == common::process::handle());
            EXPECT_TRUE( state.instances.sequential[ instance_id].idle());
            EXPECT_TRUE( state.instances.sequential[ instance_id].service( service_id));
         }

         EXPECT_TRUE( state.instances.concurrent.empty());
      }

      TEST( service_manager_state, advertise_local_service__2_instances__expect_discoverable_for_the_first)
      {
         common::unittest::Trace trace;

         auto state = local::state();

         auto advertise_message = []( common::strong::process::id pid)
         {
            common::message::service::Advertise message;
            message.process.pid = pid;
            message.process.ipc = common::process::handle().ipc;
            message.services.add.push_back( { .name = "a"});
            return message;
         };

         {
            auto result = state.update( advertise_message( common::strong::process::id{ 1}));
            EXPECT_TRUE( result.pending.empty());
            EXPECT_TRUE( result.discoverable);
         }

         {
            auto result = state.update( advertise_message( common::strong::process::id{ 2}));
            EXPECT_TRUE( result.pending.empty());
            EXPECT_FALSE( result.discoverable);
         }
      }

      TEST( service_manager_state, advertise_local_service__unadvertise___expect__service_instance_relation__removed)
      {
         common::unittest::Trace trace;

         auto state = local::state();

         // advertise
         {
            common::message::service::Advertise message;
            message.process = common::process::handle();
            message.services.add.push_back( { .name = "service1"});
            EXPECT_TRUE( state.update( std::move( message)).pending.empty());
         }

         // unadvertise
         {
            common::message::service::Advertise message;
            message.process = common::process::handle();
            message.services.remove.emplace_back( "service1");
            EXPECT_TRUE( state.update( std::move( message)).pending.empty());
         }

         {
            auto service_id = state.services.lookup( "service1");
            ASSERT_TRUE( service_id);
            EXPECT_TRUE( state.services[ service_id].information.name == "service1");
            EXPECT_TRUE( state.services[ service_id].instances.sequential().size() == 0);
         }

         {

            auto instance_id = state.instances.sequential.lookup( common::process::handle().ipc);
            ASSERT_TRUE( instance_id);
            EXPECT_TRUE( state.instances.sequential[ instance_id].process == common::process::handle());
            EXPECT_TRUE( state.instances.sequential[ instance_id].idle());
            EXPECT_FALSE( state.instances.sequential[ instance_id].service( state.services.lookup( "service1")));
         }
      }

      TEST( service_manager_state, advertise_10_local_services__expect_relation_from_instance_to_all_services)
      {
         common::unittest::Trace trace;

         auto state = local::state();

         common::message::service::Advertise message;

         {

            message.process = common::process::handle();
            {
               using Service = common::message::service::advertise::Service;
               message.services.add = { Service{ .name = "s0"}, Service{ .name = "s1"}, Service{ .name = "s2"}, Service{ .name = "s3"}, Service{ .name = "s4"}, Service{ .name = "s5"}, Service{ .name = "s6"}, Service{ .name = "s7"}, Service{ .name = "s8"}, Service{ .name = "s9"}};
               std::random_device device;
               std::mt19937 generator(device());
               std::shuffle( std::begin( message.services.add), std::end( message.services.add), generator);
            }

            EXPECT_TRUE( state.update( std::move( message)).pending.empty());
         }

         {
            auto instance_id = state.instances.sequential.lookup( common::process::handle().ipc);
            ASSERT_TRUE( instance_id);

            for( auto& service : message.services.add)
               ASSERT_TRUE( state.instances.sequential[ instance_id].service( state.services.lookup( service.name)));
         }


      }

      TEST( service_manager_state, advertise_10_local_services__unadvertise_2__expect_relation_from_instance_to_all_services_but_2)
      {
         common::unittest::Trace trace;

         auto state = local::state();


         {
            common::message::service::Advertise message;

            message.process = common::process::handle();
            {
               using Service = common::message::service::advertise::Service;
               message.services.add = { Service{ .name = "s0"}, Service{ .name = "s1"}, Service{ .name = "s2"}, Service{ .name = "s3"}, Service{ .name = "s4"}, Service{ .name = "s5"}, Service{ .name = "s6"}, Service{ .name = "s7"}, Service{ .name = "s8"}, Service{ .name = "s9"}};
               std::random_device device;
               std::mt19937 generator(device());
               std::shuffle( std::begin( message.services.add), std::end( message.services.add), generator);
            }

            EXPECT_TRUE( state.update( std::move( message)).pending.empty());
         }

         // unadvertise
         {
            common::message::service::Advertise message;
            message.process = common::process::handle();
            message.services.remove = { "s4", "s7"};
            EXPECT_TRUE( state.update( std::move( message)).pending.empty());
         }
         {
            auto service_id = state.services.lookup( "s4");
            ASSERT_TRUE( service_id);
            EXPECT_TRUE( state.services[ service_id].instances.sequential().empty());
            service_id = state.services.lookup( "s7");
            ASSERT_TRUE( service_id);
            EXPECT_TRUE( state.services[ service_id].instances.sequential().empty());
         }

         {
            auto instance_id = state.instances.sequential.lookup( common::process::handle().ipc);
            ASSERT_TRUE( instance_id);
            EXPECT_FALSE( state.instances.sequential[ instance_id].service( state.services.lookup( "s4")));
            EXPECT_FALSE( state.instances.sequential[ instance_id].service( state.services.lookup( "s7")));
            EXPECT_TRUE( state.instances.sequential[ instance_id].service( state.services.lookup( "s0")));

         }
      }

      TEST( service_manager_state, concurrent_instance_order)
      {
         common::unittest::Trace trace;

         using Property = state::instance::concurrent::Property;

         auto create_advertise = []( common::strong::process::id pid, platform::size::type order, Property property)
         {
            common::message::service::concurrent::Advertise result;
            result.order = order;
            result.process.pid = pid;
            result.process.ipc = common::strong::ipc::id::generate();
            {
               auto& service = result.services.add.emplace_back();
               service.name = "a";
               service.property = property;
            }
            return result;
         };

         manager::State state;

         std::ignore = state.update( create_advertise( common::strong::process::id{ 100}, 4, Property{ 1}));
         std::ignore = state.update( create_advertise( common::strong::process::id{ 100}, 4, Property{ 1}));
         std::ignore = state.update( create_advertise( common::strong::process::id{ 100}, 4, Property{ 1}));
         std::ignore = state.update( create_advertise( common::strong::process::id{ 101}, 3, Property{ 1}));
         std::ignore = state.update( create_advertise( common::strong::process::id{ 101}, 3, Property{ 1}));
         std::ignore = state.update( create_advertise( common::strong::process::id{ 102}, 2, Property{ 2}));
         std::ignore = state.update( create_advertise( common::strong::process::id{ 103}, 2, Property{ 1})); // we should get this. lowest order and lowest hops.

         auto service_id = state.services.lookup( "a");

         ASSERT_TRUE( service_id);

         const auto correlation = common::strong::correlation::id::generate();

         // 103 should be prioritized
         {
            auto reservation = state.reserve_concurrent( service_id, correlation, {});
            auto process = state.instances.concurrent[ reservation.instance].process;
            EXPECT_TRUE( process.pid == common::strong::process::id{ 103}) << CASUAL_NAMED_VALUE( process);
            
            // expect only 103 to be in the prioritized range
            EXPECT_TRUE( reservation.instance == state.reserve_concurrent( service_id, correlation, {}).instance);
         }
      }

      TEST( service_manager_state, concurrent_dissociate__expect_reply_if_idle)
      {
         common::unittest::Trace trace;

         auto create_advertise = []( common::strong::ipc::id ipc)
         {
            common::message::service::concurrent::Advertise result;
            result.process.pid = common::process::id();
            result.process.ipc = ipc;
            {
               auto& service = result.services.add.emplace_back();
               service.name = "a";
            }
            return result;
         };

         auto busy_instance = common::strong::ipc::id::generate();
         auto idle_instance = common::strong::ipc::id::generate();

         manager::State state;

         std::ignore = state.update( create_advertise( busy_instance));

         auto service_id = state.services.lookup( "a");
         ASSERT_TRUE( service_id);

         auto correlations = common::algorithm::generate_n( 4, [](){ return common::strong::correlation::id::generate();});

         auto reservations = common::algorithm::transform( correlations, [&]( auto correlation)
         {
            return state.reserve_concurrent( service_id, correlation, {}).instance;
         });

         std::ignore = state.update( create_advertise( idle_instance));

         // check busy
         {
            auto instance_id = state.instances.concurrent.lookup( busy_instance);
            ASSERT_TRUE( instance_id);
            EXPECT_FALSE( state.instances.concurrent[ instance_id].idle());
         }

         // check idle
         {
            auto instance_id = state.instances.concurrent.lookup( idle_instance);
            ASSERT_TRUE( instance_id);
            EXPECT_TRUE( state.instances.concurrent[ instance_id].idle());
         }


         // disassociate idle
         {
            common::message::service::concurrent::instance::disassociate::Request message;
            message.process.ipc = idle_instance;
            // we expect a reply, since the instance is idle.
            auto reply = state.disassociate( message);
            EXPECT_TRUE( reply) << CASUAL_NAMED_VALUE( state);

            EXPECT_TRUE( state.pending.disassociation.empty()) << CASUAL_NAMED_VALUE( state.pending.disassociation);
         }

         // disassociate busy
         {
            common::message::service::concurrent::instance::disassociate::Request message;
            message.process.ipc = busy_instance;
            // we expect no reply, since the instance is busy.
            auto reply = state.disassociate( message);
            EXPECT_FALSE( reply);

            EXPECT_TRUE( ! state.pending.disassociation.empty()) << CASUAL_NAMED_VALUE( state.pending.disassociation);
         }

         // disassociate unknown
         {
            common::message::service::concurrent::instance::disassociate::Request message;
            message.process.ipc = common::strong::ipc::id::generate();
            // will error-log, but we should still get a reply.
            auto reply = state.disassociate( message);
            EXPECT_TRUE( reply);
         }

         // unreserve busy, the first 3 correlations -> should not give us a disassociate reply, since the instance is still busy.
         for( auto correlation : std::views::take( correlations, 3))
         {
            auto instance_id = state.instances.concurrent.lookup( busy_instance);
            ASSERT_TRUE( instance_id);

            auto reply = state.unreserve( instance_id, common::message::event::service::Metric{ 
               .service = "a", 
               .process = common::process::Handle{ common::process::id(), busy_instance},
               .correlation = correlation
            });
            
            EXPECT_FALSE( reply) << CASUAL_NAMED_VALUE( state);
         }
         
         // the last metric should give us a disassociate reply, since the instance is now idle.
         {
            auto instance_id = state.instances.concurrent.lookup( busy_instance);
            ASSERT_TRUE( instance_id);

            auto reply = state.unreserve( instance_id, common::message::event::service::Metric{ 
               .service = "a", 
               .process = common::process::Handle{ common::process::id(), busy_instance},
               .correlation = correlations.back()
            });
            
            EXPECT_TRUE( reply) << CASUAL_NAMED_VALUE( state);
         }

         // check that the state is clean, no pending disassociation and the instance is idle.
         {
            auto instance_id = state.instances.concurrent.lookup( busy_instance);
            ASSERT_TRUE( instance_id);
            EXPECT_TRUE( state.instances.concurrent[ instance_id].idle());
            EXPECT_TRUE( state.pending.disassociation.empty()) << CASUAL_NAMED_VALUE( state.pending.disassociation);
         }

      }

      TEST( service_manager_state, disable_instance__reserve_instance__expect_no_instance_reserved)
      {
         common::unittest::Trace trace;

         auto state = local::state();

         // advertise
         {
            common::message::service::Advertise message;
            message.process = common::process::handle();
            message.services.add.push_back( { .name = "a"});
            EXPECT_TRUE( state.update( std::move( message)).pending.empty());
         }

         auto a_id = state.services.lookup( "a");
         ASSERT_TRUE( a_id);

         {
            auto instance_id = state.instances.sequential.lookup( common::process::handle().ipc);
            ASSERT_TRUE( instance_id);
            EXPECT_TRUE( state.instances.sequential[ instance_id].idle());
            EXPECT_TRUE( state.instances.sequential[ instance_id].service( a_id));
            
            // disable the instance
            state.disabled.sequential.push_back( instance_id);

            auto lookup = common::message::service::lookup::Request{ common::process::handle()};
            lookup.correlation = common::strong::correlation::id::generate();
            lookup.requested = "a";
            lookup.trid = common::transaction::ID{ common::strong::process::id{ 999}};

               // expect no instance to be reserved
               EXPECT_FALSE(( state.reserve_sequential( lookup, a_id)));
         }
      }

      TEST( service_manager_state, remove_process__expect_all_sequential_and_concurrent_instances_removed)
      {
         common::unittest::Trace trace;

         auto state = local::state();
         const auto removed_pid = common::strong::process::id{ 100};
         const auto retained_pid = common::strong::process::id{ 101};
         const auto removed_sequential_ipc = common::strong::ipc::id::generate();
         const auto retained_sequential_ipc = common::strong::ipc::id::generate();
         const auto removed_concurrent_ipc = common::strong::ipc::id::generate();
         const auto retained_concurrent_ipc = common::strong::ipc::id::generate();

         auto advertise_sequential = [&]( auto pid, auto ipc)
         {
            common::message::service::Advertise message;
            message.process = { pid, ipc};
            message.services.add.push_back( { .name = "sequential"});
            EXPECT_TRUE( state.update( std::move( message)).pending.empty());
         };

         auto advertise_concurrent = [&]( auto pid, auto ipc)
         {
            common::message::service::concurrent::Advertise message;
            message.process = { pid, ipc};
            message.services.add.push_back( { .name = "concurrent"});
            std::ignore = state.update( std::move( message));
         };

         advertise_sequential( removed_pid, removed_sequential_ipc);
         advertise_sequential( retained_pid, retained_sequential_ipc);
         advertise_concurrent( removed_pid, removed_concurrent_ipc);
         advertise_concurrent( retained_pid, retained_concurrent_ipc);

         auto result = state.remove( removed_pid);

         EXPECT_TRUE( result.reservations.empty());
         EXPECT_TRUE( result.lookups.empty());
         EXPECT_FALSE( state.instances.sequential.lookup( removed_sequential_ipc));
         EXPECT_FALSE( state.instances.concurrent.lookup( removed_concurrent_ipc));
         EXPECT_TRUE( state.instances.sequential.lookup( retained_sequential_ipc));
         EXPECT_TRUE( state.instances.concurrent.lookup( retained_concurrent_ipc));

         auto sequential = state.services.lookup( "sequential");
         auto concurrent = state.services.lookup( "concurrent");
         ASSERT_TRUE( sequential);
         ASSERT_TRUE( concurrent);
         EXPECT_TRUE( state.services[ sequential].instances.sequential().size() == 1);
         EXPECT_TRUE( state.services[ concurrent].instances.concurrent().size() == 1);
      }

      TEST( service_manager_state, remove_process__service_has_other_instance__expect_pending_lookup_retained)
      {
         common::unittest::Trace trace;

         auto state = local::state();
         const auto removed_pid = common::strong::process::id{ 110};
         const auto retained_pid = common::strong::process::id{ 111};

         auto advertise = [&]( auto pid)
         {
            common::message::service::Advertise message;
            message.process.pid = pid;
            message.process.ipc = common::strong::ipc::id::generate();
            message.services.add.push_back( { .name = "service"});
            EXPECT_TRUE( state.update( std::move( message)).pending.empty());
         };

         advertise( removed_pid);
         advertise( retained_pid);

         auto service = state.services.lookup( "service");
         ASSERT_TRUE( service);

         common::message::service::lookup::Request request;
         request.requested = "service";
         request.correlation = common::strong::correlation::id::generate();
         state.pending.service.lookups.push_back( {
            .service = service,
            .request = request,
            .when = common::chronology::time_point::clock::now()
         });

         auto result = state.remove( removed_pid);

         EXPECT_TRUE( result.lookups.empty());
         ASSERT_TRUE( state.pending.service.lookups.size() == 1);
         EXPECT_TRUE( state.pending.service.lookups.front().request.correlation == request.correlation);
         EXPECT_TRUE( state.services[ service].instances.sequential().size() == 1);
      }

      TEST( service_manager_state, remove_process__last_instance_for_service__expect_pending_lookup_returned)
      {
         common::unittest::Trace trace;

         auto state = local::state();
         const auto pid = common::strong::process::id{ 120};
         const auto ipc = common::strong::ipc::id::generate();

         common::message::service::Advertise advertise;
         advertise.process.pid = pid;
         advertise.process.ipc = ipc;
         advertise.services.add.push_back( { .name = "service"});
         EXPECT_TRUE( state.update( std::move( advertise)).pending.empty());

         auto service = state.services.lookup( "service");
         ASSERT_TRUE( service);
         auto instance = state.instances.sequential.lookup( ipc);
         ASSERT_TRUE( instance);

         common::message::service::lookup::Request request;
         request.requested = "service";
         request.correlation = common::strong::correlation::id::generate();
         state.pending.service.lookups.push_back( {
            .service = service,
            .request = request,
            .when = common::chronology::time_point::clock::now()
         });
         state.pending.service.deadline.add( {
            .when = common::chronology::time_point::clock::now(),
            .correlation = request.correlation,
            .target = instance,
            .service = service
         });

         auto result = state.remove( pid);

         ASSERT_TRUE( result.lookups.size() == 1);
         EXPECT_TRUE( result.lookups.front().service == service);
         EXPECT_TRUE( result.lookups.front().request.correlation == request.correlation);
         ASSERT_TRUE( result.deadline);
         EXPECT_TRUE( std::holds_alternative< state::service::pending::deadline::Unset>( *result.deadline));
         EXPECT_TRUE( state.pending.service.deadline.entries().empty());
         EXPECT_TRUE( state.pending.service.lookups.empty());
         EXPECT_TRUE( state.instances.sequential.empty());
         EXPECT_TRUE( state.services[ service].instances.sequential().empty());
      }

      TEST( service_manager_state, remove_unknown_process__expect_state_unchanged)
      {
         common::unittest::Trace trace;

         auto state = local::state();
         common::message::service::Advertise advertise;
         advertise.process = common::process::handle();
         advertise.services.add.push_back( { .name = "service"});
         EXPECT_TRUE( state.update( std::move( advertise)).pending.empty());

         auto result = state.remove( common::strong::process::id{ 999});

         EXPECT_TRUE( result.reservations.empty());
         EXPECT_TRUE( result.lookups.empty());
         EXPECT_TRUE( state.instances.sequential.size() == 1);
         auto service = state.services.lookup( "service");
         ASSERT_TRUE( service);
         EXPECT_TRUE( state.services[ service].instances.sequential().size() == 1);
      }

   } // service::manager
} // casual

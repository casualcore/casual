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
   using namespace common;

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

         auto arguments = manager::admin::services( state);

         EXPECT_TRUE( arguments.services.at( 0).name == admin::service::name::state);
      }


      TEST( service_manager_state, advertise_empty_invalid_local__expect_no_op)
      {
         common::unittest::Trace trace;

         auto state = local::state();

         EXPECT_TRUE( state.update( common::message::service::Advertise{}).empty());

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
            EXPECT_TRUE( state.update( std::move( message)).empty());
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

      TEST( service_manager_state, advertise_local_service__unadvertise___expect__service_instance_relation__removed)
      {
         common::unittest::Trace trace;

         auto state = local::state();

         // advertise
         {
            common::message::service::Advertise message;
            message.process = common::process::handle();
            message.services.add.push_back( { .name = "service1"});
            EXPECT_TRUE( state.update( std::move( message)).empty());
         }

         // unadvertise
         {
            common::message::service::Advertise message;
            message.process = common::process::handle();
            message.services.remove.emplace_back( "service1");
            EXPECT_TRUE( state.update( std::move( message)).empty());
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

            EXPECT_TRUE( state.update( std::move( message)).empty());
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

            EXPECT_TRUE( state.update( std::move( message)).empty());
         }

         // unadvertise
         {
            common::message::service::Advertise message;
            message.process = common::process::handle();
            message.services.remove = { "s4", "s7"};
            EXPECT_TRUE( state.update( std::move( message)).empty());
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

         // 103 should be prioritized
         {
            auto instance_id = state.reserve_concurrent( service_id, {});
            auto process = state.instances.concurrent[ instance_id].process;
            EXPECT_TRUE( process.pid == common::strong::process::id{ 103}) << CASUAL_NAMED_VALUE( process);
            
            // expect only 103 to be in the prioritized range
            EXPECT_TRUE( instance_id == state.reserve_concurrent( service_id, {}));
         }
      }

      namespace local
      {
         namespace
         {
            auto process()
            {
               static platform::process::native::type id = 10;
               return common::process::Handle{ common::strong::process::id{ id++}, common::strong::ipc::id::generate()};
            }

            auto advertise( manager::State& state, const common::process::Handle& process, auto service)
            {
               common::message::service::Advertise message;
               message.process = process;
               message.services.add.push_back( { .name = service});

               return state.update( std::move( message));
            } 

            auto reserve( manager::State& state, const process::Handle& caller, const std::string& service, const auto& trid, const auto& correlation)
            {
               auto service_id = state.services.lookup( service);
               EXPECT_TRUE( service_id);
               return state.reserve_sequential( service_id, caller, correlation, transaction::id::range::global( trid));
            }

            auto create_metric( const process::Handle& instance, const std::string& service, const transaction::ID& trid)
            {
               common::message::event::service::Metric metric;
               metric.process = instance;
               metric.service = service;
               metric.trid = trid;
               metric.code.result = decltype( metric.code.result)::ok;
               return metric;
            }

            auto unreserve( manager::State& state, const process::Handle& instance, const std::string& service, const auto& trid, const auto& correlation)
            {
               common::message::service::call::ACK message;
               message.metric = create_metric( instance, service, trid);
               message.correlation = correlation;

               auto instance_id = state.instances.sequential.lookup( instance.ipc);

               return state.unreserve_sequential( instance_id, message);
            }

            namespace concurrent
            {
               auto advertise( manager::State& state, const common::process::Handle& process, auto service)
               {
                  common::message::service::concurrent::Advertise message;
                  message.process = process;
                  message.services.add.push_back( { .name = service});

                  return state.update( std::move( message));
               } 

               auto reserve( manager::State& state, const std::string& service, const transaction::ID& trid)
               {
                  auto service_id = state.services.lookup( service);
                  EXPECT_TRUE( service_id);
                  return state.reserve_concurrent( service_id, transaction::id::range::global( trid));
               }

               auto unreserve( manager::State& state, const process::Handle& instance, const std::string& service, std::span< const transaction::ID> trids)
               {
                  common::message::event::service::Calls message;
                  message.metrics = algorithm::transform( trids, [&]( const auto& trid)
                  {
                     return create_metric( instance, service, trid);
                  });

                  return state.unreserve_concurrent( message);
               }
               
            } // concurrent

         } // <unnamed>
      } // local


      TEST( service_manager_state, transaction_instance_reserve_unreserve__expect_empty_transaction_state)
      {
         common::unittest::Trace trace;

         auto state = State{};

         EXPECT_TRUE( state.transaction.empty());

         auto a = local::process();
         auto b = local::process();
         auto c = local::process();

         local::advertise( state, a, "a");
         local::advertise( state, b, "b");
         local::advertise( state, c, "c");

         EXPECT_TRUE( state.instances.sequential.size() == 3);

         auto trid = common::transaction::id::create();
         auto correlation = common::strong::correlation::id::generate();

         EXPECT_TRUE( local::reserve( state, process::handle(), "a", trid, correlation));
         EXPECT_TRUE( local::reserve( state, process::handle(), "b", trid, correlation));
         EXPECT_TRUE( local::reserve( state, process::handle(), "c", trid, correlation));

         ASSERT_TRUE( state.transaction.find( transaction::id::range::global( trid)));
         EXPECT_TRUE( state.transaction.find( transaction::id::range::global( trid))->sequential.size() == 3);

         EXPECT_TRUE( ! std::get< 1>( local::unreserve( state, a, "a", trid, correlation)).has_value());
         EXPECT_TRUE( ! std::get< 1>( local::unreserve( state, b, "b", trid, correlation)).has_value());
         EXPECT_TRUE( ! std::get< 1>( local::unreserve( state, c, "c", trid, correlation)).has_value());

         // we still keep track of the transaction. This might not be needed...
         EXPECT_TRUE( state.transaction.find( transaction::id::range::global( trid)));

         state.transaction.disassociate( transaction::id::range::global( trid));

         // now the transaction should be gone
         EXPECT_TRUE( state.transaction.empty());
      }

      TEST( service_manager_state, transaction_concurrent_instance_reserve_unreserve__expect_empty_transaction_state)
      {
         common::unittest::Trace trace;

         auto state = State{};

         EXPECT_TRUE( state.transaction.empty());

         auto a = local::process();
         auto b = local::process();
         auto c = local::process();

         local::concurrent::advertise( state, a, "a");
         local::concurrent::advertise( state, b, "b");
         local::concurrent::advertise( state, c, "c");

         EXPECT_TRUE( state.instances.concurrent.size() == 3);

         auto trid = common::transaction::id::create();

         EXPECT_TRUE( local::concurrent::reserve( state, "a", trid));
         EXPECT_TRUE( local::concurrent::reserve( state, "b", trid));
         EXPECT_TRUE( local::concurrent::reserve( state, "c", trid));

         ASSERT_TRUE( state.transaction.find( transaction::id::range::global( trid)));
         EXPECT_TRUE( state.transaction.find( transaction::id::range::global( trid))->concurrent.size() == 3);

         EXPECT_TRUE( local::concurrent::unreserve( state, a, "a", std::span{ &trid, 1}).empty());
         EXPECT_TRUE( local::concurrent::unreserve( state, b, "b", std::span{ &trid, 1}).empty());
         EXPECT_TRUE( local::concurrent::unreserve( state, c, "c", std::span{ &trid, 1}).empty());

         // we still keep track of the transaction. This might not be needed...
         EXPECT_TRUE( state.transaction.find( transaction::id::range::global( trid)));

         state.transaction.disassociate( transaction::id::range::global( trid));

         // now the transaction should be gone
         EXPECT_TRUE( state.transaction.empty());
      }




      TEST( service_manager_state, transaction_instance_reserve__disassociate_trid__unreserve__expect_stale_transaction)
      {
         common::unittest::Trace trace;

         auto state = State{};

         EXPECT_TRUE( state.transaction.empty());

         auto a = local::process();
         auto b = local::process();
         auto c = local::process();

         local::advertise( state, a, "a");
         local::advertise( state, b, "b");
         local::advertise( state, c, "c");

         EXPECT_TRUE( state.instances.sequential.size() == 3);

         auto trid = common::transaction::id::create();
         auto correlation = common::strong::correlation::id::generate();

         EXPECT_TRUE( local::reserve( state, process::handle(), "a", trid, correlation));
         EXPECT_TRUE( local::reserve( state, process::handle(), "b", trid, correlation));
         EXPECT_TRUE( local::reserve( state, process::handle(), "c", trid, correlation));

         EXPECT_TRUE( ! std::get< 1>( local::unreserve( state, a, "a", trid, correlation)).has_value());

         // we get a disassociate message from TM -> the transaction is potentially stale
         state.transaction.disassociate( transaction::id::range::global( trid));

         // the transaction is potentially stale, but we "keep" it until we unreserve the last instance
         EXPECT_TRUE( ! std::get< 1>( local::unreserve( state, b, "b", trid, correlation)).has_value());

         // we should get the stale transaction, this would be sent to the transaction manager
         auto gtrid = std::get< 1>( local::unreserve( state, c, "c", trid, correlation));
         EXPECT_TRUE( gtrid);
         EXPECT_TRUE( *gtrid == transaction::id::range::global( trid));         

         // now the transaction should be gone
         EXPECT_TRUE( state.transaction.empty()) << CASUAL_NAMED_VALUE( state.transaction);
      }


      TEST( service_manager_state, transaction_concurrent_instance_reserve__disassociate_trid__unreserve__expect_stale_transaction)
      {
         common::unittest::Trace trace;

         auto state = State{};

         EXPECT_TRUE( state.transaction.empty());

         auto a = local::process();
         auto b = local::process();
         auto c = local::process();

         local::concurrent::advertise( state, a, "a");
         local::concurrent::advertise( state, b, "b");
         local::concurrent::advertise( state, c, "c");

         EXPECT_TRUE( state.instances.concurrent.size() == 3);

         auto trids = std::array{ common::transaction::id::create(), common::transaction::id::create()};
         algorithm::sort( trids);


         // we reserve the services 'a', 'b', 'c' twice for each of the transactions
         for( auto service : { "a", "b", "c"})
         {
            EXPECT_TRUE( local::concurrent::reserve( state, service, trids[ 0]));
            EXPECT_TRUE( local::concurrent::reserve( state, service, trids[ 1]));
            EXPECT_TRUE( local::concurrent::reserve( state, service, trids[ 0]));
            EXPECT_TRUE( local::concurrent::reserve( state, service, trids[ 1]));
         }

         // we should have 2 "reservations" for each gtrid and instance (a, b, c)
         for( auto& trid : trids)
         {
            auto transaction = state.transaction.find( transaction::id::range::global( trid));
            ASSERT_TRUE( transaction);
            EXPECT_TRUE( algorithm::all_of( transaction->concurrent, []( const auto& instance)
            {
               return instance.count == 2;
            }));
         }

         EXPECT_TRUE( local::concurrent::unreserve( state, a, "a", trids).empty());
         EXPECT_TRUE( local::concurrent::unreserve( state, a, "a", trids).empty());
         EXPECT_TRUE( local::concurrent::unreserve( state, b, "b", trids).empty());

         // we get a disassociate message from TM -> the transaction 0 is potentially stale
         state.transaction.disassociate( transaction::id::range::global( trids[ 0]));
         

         EXPECT_TRUE( local::concurrent::unreserve( state, b, "b", trids).empty());

         // we get a disassociate message from TM -> the transaction 1 is potentially stale
         state.transaction.disassociate( transaction::id::range::global( trids[ 1]));

         EXPECT_TRUE( local::concurrent::unreserve( state, c, "c", trids).empty());


         EXPECT_TRUE( ! state.transaction.empty());

         {
            // the last unreserve should remove the transactions and return the stale transactions
            auto gtrid = local::concurrent::unreserve( state, c, "c", trids);

            // the transaction state should be empty
            EXPECT_TRUE( state.transaction.empty()) << CASUAL_NAMED_VALUE( state.transaction);

            EXPECT_TRUE( gtrid.size() == 2) << CASUAL_NAMED_VALUE( gtrid);

            // the order of the gtrids is not guaranteed, we need to sort them
            algorithm::sort( gtrid);

            EXPECT_TRUE( gtrid.at( 0) == transaction::id::range::global( trids[ 0]));
            EXPECT_TRUE( gtrid.at( 1) == transaction::id::range::global( trids[ 1]));
         }

         // now the transaction should be gone
         EXPECT_TRUE( state.transaction.empty());
      }

     TEST( service_manager_state, transaction_instance_reserve__disassociate_trid__remove_instance__expect_stale_transaction)
      {
         common::unittest::Trace trace;

         auto state = State{};

         EXPECT_TRUE( state.transaction.empty());

         auto a = local::process();
         auto b = local::process();
         auto c = local::process();

         local::advertise( state, a, "a");
         local::advertise( state, b, "b");
         local::advertise( state, c, "c");

         EXPECT_TRUE( state.instances.sequential.size() == 3);

         auto trid = common::transaction::id::create();
         auto correlation = common::strong::correlation::id::generate();

         EXPECT_TRUE( local::reserve( state, process::handle(), "a", trid, correlation));
         EXPECT_TRUE( local::reserve( state, process::handle(), "b", trid, correlation));
         EXPECT_TRUE( local::reserve( state, process::handle(), "c", trid, correlation));

         EXPECT_TRUE( ! std::get< 1>( local::unreserve( state, a, "a", trid, correlation)).has_value());

         // we get a disassociate message from TM -> the transaction is potentially stale
         state.transaction.disassociate( transaction::id::range::global( trid));

         // the transaction is potentially stale, but we "keep" it until we unreserve the last instance
         EXPECT_TRUE( ! std::get< 1>( local::unreserve( state, b, "b", trid, correlation)).has_value());

         // we should get the stale transaction, this would be sent to the transaction manager
         {
            auto consequence = state.remove( c.pid);
            EXPECT_TRUE( consequence.callers.at( 0).process == process::handle());
            EXPECT_TRUE( consequence.stale.at( 0) == transaction::id::range::global( trid));
         }

         // now the transaction should be gone
         EXPECT_TRUE( state.transaction.empty()) << CASUAL_NAMED_VALUE( state.transaction);
      }
   } // service::manager
} // casual

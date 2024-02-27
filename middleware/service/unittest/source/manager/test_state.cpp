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
            message.services.add.emplace_back( "service1");
            EXPECT_TRUE( state.update( std::move( message)).empty());
         }

         EXPECT_TRUE( state.instances.sequential.size() == 1);
         {
            auto service = state.service( "service1");
            ASSERT_TRUE( service);
            EXPECT_TRUE( service->information.name == "service1");
            ASSERT_TRUE( service->instances.sequential().size() == 1);
            auto& instance = service->instances.sequential().at( 0);
            EXPECT_TRUE( instance.process() == common::process::handle());
            EXPECT_TRUE( instance.idle());
            EXPECT_TRUE( instance.get().service( "service1"));
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
            message.services.add.emplace_back( "service1");
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
            auto service = state.service( "service1");
            ASSERT_TRUE( service);
            EXPECT_TRUE( service->information.name == "service1");
            EXPECT_TRUE( service->instances.sequential().size() == 0);
         }

         {

            auto instance = state.sequential( common::process::handle().ipc);
            ASSERT_TRUE( instance);
            EXPECT_TRUE( instance->process == common::process::handle());
            EXPECT_TRUE( instance->idle());
            EXPECT_FALSE( instance->service( "service1"));
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
               message.services.add = { { "s0"}, { "s1"}, { "s2"}, { "s3"}, { "s4"}, { "s5"}, { "s6"}, { "s7"}, { "s8"}, { "s9"}};
               std::random_device device;
               std::mt19937 generator(device());
               std::shuffle( std::begin( message.services.add), std::end( message.services.add), generator);
            }

            EXPECT_TRUE( state.update( std::move( message)).empty());
         }

         {
            auto instance = state.sequential( common::process::handle().ipc);
            ASSERT_TRUE( instance);

            for( auto& service : message.services.add)
               ASSERT_TRUE( instance->service( service.name));
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
               message.services.add = { { "s0"}, { "s1"}, { "s2"}, { "s3"}, { "s4"}, { "s5"}, { "s6"}, { "s7"}, { "s8"}, { "s9"}};
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
            auto service = state.service( "s4");
            ASSERT_TRUE( service);
            EXPECT_TRUE( service->instances.sequential().empty());
            service = state.service( "s7");
            ASSERT_TRUE( service);
            EXPECT_TRUE( service->instances.sequential().empty());
         }

         {
            auto instance = state.sequential( common::process::handle().ipc);
            ASSERT_TRUE( instance);
            EXPECT_FALSE( instance->service( "s4"));
            EXPECT_FALSE( instance->service( "s7"));
            EXPECT_TRUE( instance->service( "s0"));

         }
      }

      TEST( service_manager_state, concurrent_instance_order)
      {
         common::unittest::Trace trace;

         using Property = state::service::instance::Concurrent::Property;
         using property_type = decltype( Property::type);

         auto create_advertise = []( common::strong::process::id pid, platform::size::type order, Property property)
         {
            common::message::service::concurrent::Advertise result;
            result.order = order;
            result.process.pid = pid;
            result.process.ipc = common::strong::ipc::id{ common::uuid::make()};
            {
               auto& service = result.services.add.emplace_back();
               service.name = "a";
               service.property = property;
            }
            return result;
         };

         manager::State state;

         (void)state.update( create_advertise( common::strong::process::id{ 100}, 0, Property{ property_type::discovered, 0}));
         (void)state.update( create_advertise( common::strong::process::id{ 100}, 0, Property{ property_type::discovered, 0}));
         (void)state.update( create_advertise( common::strong::process::id{ 100}, 0, Property{ property_type::discovered, 0}));
         (void)state.update( create_advertise( common::strong::process::id{ 101}, 1, Property{ property_type::discovered, 0}));
         (void)state.update( create_advertise( common::strong::process::id{ 101}, 1, Property{ property_type::discovered, 0}));
         (void)state.update( create_advertise( common::strong::process::id{ 102}, 2, Property{ property_type::discovered, 0}));
         (void)state.update( create_advertise( common::strong::process::id{ 103}, 3, Property{ property_type::configured, 3}));

         auto service = state.service( "a");

         ASSERT_TRUE( service);

         // 103 should be prioritized
         {
            auto process = service->reserve_concurrent( {});
            EXPECT_TRUE( process.pid == common::strong::process::id{ 103});
            
            // expect only 103 to be in the prioritized range
            EXPECT_TRUE( process == service->reserve_concurrent( {}));

            // we remove the instance
            //service->remove( )
         }

      }


   } // service::manager
} // casual

//!
//! casual 
//!

#include "common/unittest.h"

#include "service/manager/state.h"
#include "service/manager/admin/server.h"



#include <random>

namespace casual
{
   namespace service
   {
      namespace manager
      {

         TEST( service_manager_state, admin_services)
         {
            common::unittest::Trace trace;

            manager::State state;

            auto arguments = manager::admin::services( state);

            EXPECT_TRUE( arguments.services.at( 0).name == admin::service::name::state());
         }


         TEST( service_manager_state, advertise_empty_invalid_local__expect_no_op)
         {
            common::unittest::Trace trace;

            manager::State state;

            {
               common::message::service::Advertise message;
               state.update( message);
            }

            EXPECT_TRUE( state.instances.local.empty()) << "state.instances.local.size(): "<< state.instances.local.begin()->second.process;
            EXPECT_TRUE( state.instances.remote.empty());
         }

         TEST( service_manager_state, advertise_empty_invalid_remote__expect_no_op)
         {
            common::unittest::Trace trace;

            manager::State state;

            {
               common::message::gateway::domain::Advertise message;
               state.update( message);
            }

            EXPECT_TRUE( state.instances.local.empty());
            EXPECT_TRUE( state.instances.remote.empty()) << "state.instances.remote.size(): "<< state.instances.remote.begin()->second.process;
         }


         TEST( service_manager_state, advertise_local_service__expect_service_and_instance_added)
         {
            common::unittest::Trace trace;

            manager::State state;

            {
               common::message::service::Advertise message;
               message.process = common::process::handle();
               message.services.emplace_back( "service1");
               state.update( message);
            }

            EXPECT_TRUE( state.instances.local.size() == 1);
            {
               auto service = state.find_service( "service1");
               ASSERT_TRUE( service);
               EXPECT_TRUE( service->information.name == "service1");
               ASSERT_TRUE( service->instances.local.size() == 1);
               auto& instance = service->instances.local.at( 0);
               EXPECT_TRUE( instance.process() == common::process::handle());
               EXPECT_TRUE( instance.idle());
               EXPECT_TRUE( instance.get().service( "service1"));
            }

            EXPECT_TRUE( state.instances.remote.empty());
         }

         TEST( service_manager_state, advertise_local_service__unadvertise___expect__service_instance_relation__removed)
         {
            common::unittest::Trace trace;

            manager::State state;

            // advertise
            {
               common::message::service::Advertise message;
               message.process = common::process::handle();
               message.services.emplace_back( "service1");
               state.update( message);
            }

            // unadvertise
            {
               common::message::service::Advertise message;
               message.process = common::process::handle();
               message.services.emplace_back( "service1");
               message.directive = decltype( message.directive)::remove;
               state.update( message);
            }


            {
               auto service = state.find_service( "service1");
               ASSERT_TRUE( service);
               EXPECT_TRUE( service->information.name == "service1");
               EXPECT_TRUE( service->instances.local.size() == 0);
            }

            {

               auto& instance = state.local( common::process::id());

               EXPECT_TRUE( instance.process == common::process::handle());
               EXPECT_TRUE( instance.idle());
               EXPECT_FALSE( instance.service( "service1"));
            }
         }

         TEST( service_manager_state, advertise_10_local_services__expect_relation_from_instance_to_all_services)
         {
            common::unittest::Trace trace;

            manager::State state;

            common::message::service::Advertise message;

            {

               message.process = common::process::handle();
               {
                  message.services = { { "s0"}, { "s1"}, { "s2"}, { "s3"}, { "s4"}, { "s5"}, { "s6"}, { "s7"}, { "s8"}, { "s9"}};
                  std::random_device device;
                  std::mt19937 generator(device());
                  std::shuffle( std::begin( message.services), std::end( message.services), generator);
               }

               state.update( message);
            }

            {
               auto& instance = state.local( common::process::id());

               for( auto& s : message.services)
               {
                  ASSERT_TRUE( instance.service( s.name));
               }
            }


         }

         TEST( service_manager_state, advertise_10_local_services__unadvertise_2__expect_relation_from_instance_to_all_services_but_2)
         {
            common::unittest::Trace trace;

            manager::State state;


            {
               common::message::service::Advertise message;

               message.process = common::process::handle();
               {
                  message.services = { { "s0"}, { "s1"}, { "s2"}, { "s3"}, { "s4"}, { "s5"}, { "s6"}, { "s7"}, { "s8"}, { "s9"}};
                  std::random_device device;
                  std::mt19937 generator(device());
                  std::shuffle( std::begin( message.services), std::end( message.services), generator);
               }

               state.update( message);
            }

            // unadvertise
            {
               common::message::service::Advertise message;
               message.process = common::process::handle();
               message.services = { { "s4"}, { "s7"}};
               message.directive = decltype( message.directive)::remove;
               state.update( message);
            }
            {
               auto service = state.find_service( "s4");
               ASSERT_TRUE( service);
               EXPECT_TRUE( service->instances.local.empty());
               service = state.find_service( "s7");
               ASSERT_TRUE( service);
               EXPECT_TRUE( service->instances.local.empty());
            }

            {
               auto& instance = state.local( common::process::id());
               EXPECT_FALSE( instance.service( "s4"));
               EXPECT_FALSE( instance.service( "s7"));

               EXPECT_TRUE( instance.service( "s0"));

            }
         }
      } // manager
   } // service
} // casual

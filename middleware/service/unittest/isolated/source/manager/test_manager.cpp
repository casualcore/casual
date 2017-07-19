//!
//! casual
//!


#include <gtest/gtest.h>
#include "common/unittest.h"

#include "service/manager/admin/server.h"
#include "service/manager/admin/managervo.h"

#include "common/message/domain.h"

#include "common/mockup/ipc.h"
#include "common/mockup/domain.h"
#include "common/mockup/process.h"
#include "common/service/lookup.h"

#include "sf/service/protocol/call.h"

namespace casual
{
	namespace service
	{
	   namespace local
      {
         namespace
         {



            struct Manager
            {
               Manager() : process{ "./bin/casual-service-manager", { "--forward", "./bin/casual-service-forward"}}
               {
                  //
                  // Wait for manager to get online
                  //
                  EXPECT_TRUE( process.handle() != common::process::handle());
               }

               ~Manager()
               {
               }

               common::mockup::Process process;

            };

            struct Domain : common::mockup::domain::Manager
            {
               using common::mockup::domain::Manager::Manager;

               local::Manager service;
               common::mockup::domain::transaction::Manager tm;
            };

            namespace call
            {
               manager::admin::StateVO state()
               {
                  sf::service::protocol::binary::Call call;

                  auto reply = call( manager::admin::service::name::state());

                  manager::admin::StateVO serviceReply;

                  reply >> CASUAL_MAKE_NVP( serviceReply);

                  return serviceReply;
               }

            } // call

            namespace service
            {
               const manager::admin::ServiceVO* find( const manager::admin::StateVO& state, const std::string& name)
               {
                  auto found = common::range::find_if( state.services, [&name]( auto& s){
                     return s.name == name;
                  });

                  if( found) { return &( *found);}

                  return nullptr;
               }
            } // service

            namespace instance
            {
               const manager::admin::instance::LocalVO* find( const manager::admin::StateVO& state, common::platform::pid::type pid)
               {
                  auto found = common::range::find_if( state.instances.local, [pid]( auto& i){
                     return i.process.pid == pid;
                  });

                  if( found) { return &( *found);}

                  return nullptr;
               }
            } // instance

         } // <unnamed>
      } // local


      TEST( service_manager, startup_shutdown__expect_no_throw)
      {
         common::unittest::Trace trace;

         EXPECT_NO_THROW({
            local::Domain domain;
         });

      }

      namespace local
      {
         namespace
         {
            bool has_services( const std::vector< manager::admin::ServiceVO>& services, std::initializer_list< const char*> wanted)
            {
               return common::range::all_of( wanted, [&services]( auto& s){
                  return common::range::find_if( services, [&s]( auto& service){
                     return service.name == s;
                  });
               });
            }

         } // <unnamed>
      } // local

      TEST( service_manager, startup___expect_1_service_in_state)
      {
         common::unittest::Trace trace;

         local::Domain domain;

         auto state = local::call::state();

         EXPECT_TRUE( local::has_services( state.services, { manager::admin::service::name::state()}));
         EXPECT_FALSE( local::has_services( state.services, { "non/existent/service"}));
      }


      TEST( service_manager, advertise_2_services_for_1_server__expect__3_services)
      {
         common::unittest::Trace trace;

         local::Domain domain;

         common::mockup::domain::echo::Server server{ { { "service1"}, { "service2"}}};

         auto state = local::call::state();

         ASSERT_TRUE( local::has_services( state.services, { manager::admin::service::name::state(), "service1", "service2"}));

         {
            auto service = local::service::find( state, "service1");
            ASSERT_TRUE( service);
            ASSERT_TRUE( service->instances.local.size() == 1);
            EXPECT_TRUE( service->instances.local.at( 0).pid == server.process().pid);
         }
         {
            auto service = local::service::find( state, "service2");
            ASSERT_TRUE( service);
            ASSERT_TRUE( service->instances.local.size() == 1);
            EXPECT_TRUE( service->instances.local.at( 0).pid == server.process().pid);
         }
      }

      TEST( service_manager, advertise_2_services_for_1_server__expect__2_local_instances)
      {
         common::unittest::Trace trace;

         local::Domain domain;

         common::mockup::domain::echo::Server server{ { { "service1"}, { "service2"}}};

         auto state = local::call::state();

         ASSERT_TRUE( state.instances.local.size() == 2);
         ASSERT_TRUE( state.instances.remote.empty());

         {
            auto instance = local::instance::find( state, server.process().pid);
            ASSERT_TRUE( instance);
            EXPECT_TRUE( instance->state == manager::admin::instance::LocalVO::State::idle);
         }
      }


      TEST( service_manager, advertise_2_services_for_1_server__prepare_shutdown___expect_instance_exiting)
      {
         common::unittest::Trace trace;

         local::Domain domain;

         common::mockup::domain::echo::Server server{ { { "service1"}, { "service2"}}};

         {
            common::message::domain::process::prepare::shutdown::Request request;
            request.process = common::process::handle();
            request.processes.push_back( server.process());

            auto reply = common::communication::ipc::call( common::communication::ipc::service::manager::device(), request);

            EXPECT_TRUE( request.processes == reply.processes);

         }

         auto state = local::call::state();

         {
            auto instance = local::instance::find( state, server.process().pid);
            ASSERT_TRUE( instance);
            EXPECT_TRUE( instance->state == manager::admin::instance::LocalVO::State::exiting);
         }
      }

      TEST( service_manager, advertise_2_services_for_1_server__prepare_shutdown__lookup___expect_absent)
      {
         common::unittest::Trace trace;

         local::Domain domain;

         common::mockup::domain::echo::Server server{ { { "service1"}, { "service2"}}};

         {
            common::message::domain::process::prepare::shutdown::Request request;
            request.process = common::process::handle();
            request.processes.push_back( server.process());

            auto reply = common::communication::ipc::call( common::communication::ipc::service::manager::device(), request);

            EXPECT_TRUE( request.processes == reply.processes);

         }

         EXPECT_THROW({
            auto service = common::service::Lookup{ "service1"}();
         }, common::exception::xatmi::service::no::Entry);

         EXPECT_THROW({
            auto service = common::service::Lookup{ "service2"}();
         }, common::exception::xatmi::service::no::Entry);
      }

		TEST( service_manager, advertise_new_services_current_server)
      {
		   common::unittest::Trace trace;

		   local::Domain domain;

		   common::mockup::domain::echo::Server server{ { { "service1"}, { "service2"}}};

		   {
		      auto service = common::service::Lookup{ "service1"}();
		      EXPECT_TRUE( service.service.name == "service1");
		      EXPECT_TRUE( service.process == server.process());
		      EXPECT_TRUE( service.state == decltype( service)::State::idle);
		   }

         {
            auto service = common::service::Lookup{ "service2"}();
            EXPECT_TRUE( service.service.name == "service2");

            // we only have one instance, we expect this to be busy
            EXPECT_TRUE( service.state == decltype( service)::State::busy);
         }
      }


		TEST( service_manager, unadvertise_service)
      {
         common::unittest::Trace trace;

         local::Domain domain;

         common::mockup::domain::echo::Server server{ { { "service1"}, { "service2"}}};

         server.undadvertise( { { "service2"}});

         //
         // echo server has unadvertise this service. The service is
         // still "present" in service-manager with no instances. Hence it's absent
         //
         EXPECT_THROW({
            auto service = common::service::Lookup{ "service2"}();
         }, common::exception::xatmi::service::no::Entry);
      }



      TEST( service_manager, service_lookup_non_existent__expect_absent_reply)
      {
         common::unittest::Trace trace;

         local::Domain domain;

         common::mockup::domain::echo::Server server{ { { "service1"}, { "service2"}}};

         EXPECT_THROW({
            auto service = common::service::Lookup{ "non-existent-service"}();
         }, common::exception::xatmi::service::no::Entry);
      }


      TEST( service_manager, service_lookup_service1__expect__busy_reply__send_ack____expect__idle_reply)
      {
         common::unittest::Trace trace;

         local::Domain domain;

         common::mockup::domain::echo::Server server{ { { "service1"}, { "service2"}}};

         {
            auto service = common::service::Lookup{ "service1"}();
            EXPECT_TRUE( service.service.name == "service1");
            EXPECT_TRUE( service.process == server.process());
            EXPECT_TRUE( service.state == decltype( service)::State::idle);
         }

         common::service::Lookup lookup{ "service2"};
         {
            auto service = lookup();
            EXPECT_TRUE( service.service.name == "service2");

            // we only have one instance, we expect this to be busy
            EXPECT_TRUE( service.state == decltype( service)::State::busy);
         }

         {
            // Send Ack
            server.send_ack();

            // get next pending reply
            auto service = lookup();

            EXPECT_TRUE( service.state == decltype( service)::State::idle);
         }
      }


      TEST( service_manager, service_lookup_service1__service_lookup_service1_forward_context____expect_forward_reply)
      {
         common::unittest::Trace trace;

         local::Domain domain;

         common::mockup::domain::echo::Server server{ { "service1"}};

         {
            auto service = common::service::Lookup{ "service1"}();
            EXPECT_TRUE( service.service.name == "service1");
            EXPECT_TRUE( service.process == server.process());
            EXPECT_TRUE( service.state == decltype( service)::State::idle);
         }


         {
            common::service::Lookup lookup{ "service1", common::service::Lookup::Context::forward};
            auto service = lookup();
            EXPECT_TRUE( service.service.name == "service1");

            // service-manager will let us think that the service is idle, and send us the queue to the forward-cache
            EXPECT_TRUE( service.state == decltype( service)::State::idle);
            EXPECT_TRUE( service.process.queue);
            EXPECT_TRUE( service.process.queue != server.process().queue);
         }
      }
	} // service
} // casual

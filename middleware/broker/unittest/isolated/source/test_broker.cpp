//!
//! casual
//!


#include <gtest/gtest.h>
#include "common/unittest.h"

#include "broker/broker.h"
#include "broker/handle.h"
#include "broker/admin/server.h"

#include "common/mockup/ipc.h"
#include "common/mockup/domain.h"
#include "common/mockup/process.h"
#include "common/message/type.h"
#include "common/service/lookup.h"

#include "sf/service/protocol/call.h"

namespace casual
{

   using namespace common;

	namespace broker
	{
	   namespace local
      {
         namespace
         {



            struct Broker
            {
               Broker() : process{ "./bin/casual-broker", { "--forward", "./bin/casual-forward-cache"}}
               {
                  //
                  // Wait for broker to get online
                  //
                  EXPECT_TRUE( process.handle() != common::process::handle());
               }

               ~Broker()
               {
               }

               mockup::Process process;

            };

            struct Domain : mockup::domain::Manager
            {
               using mockup::domain::Manager::Manager;

               local::Broker broker;
               mockup::domain::transaction::Manager tm;
            };

            namespace call
            {
               admin::StateVO state()
               {
                  sf::service::protocol::binary::Call call;

                  auto reply = call( admin::service::name::state());

                  admin::StateVO serviceReply;

                  reply >> CASUAL_MAKE_NVP( serviceReply);

                  return serviceReply;
               }

            } // call

            namespace service
            {
               const admin::ServiceVO* find( const admin::StateVO& state, const std::string& name)
               {
                  auto found = range::find_if( state.services, [&name]( auto& s){
                     return s.name == name;
                  });

                  if( found) { return &( *found);}

                  return nullptr;
               }
            } // service

            namespace instance
            {
               const admin::instance::LocalVO* find( const admin::StateVO& state, platform::pid::type pid)
               {
                  auto found = range::find_if( state.instances.local, [pid]( auto& i){
                     return i.process.pid == pid;
                  });

                  if( found) { return &( *found);}

                  return nullptr;
               }
            } // instance

         } // <unnamed>
      } // local


      TEST( casual_broker, startup_shutdown__expect_no_throw)
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
            bool has_services( const std::vector< admin::ServiceVO>& services, std::initializer_list< const char*> wanted)
            {
               return range::all_of( wanted, [&services]( auto& s){
                  return range::find_if( services, [&s]( auto& service){
                     return service.name == s;
                  });
               });
            }

         } // <unnamed>
      } // local

      TEST( casual_broker, startup___expect_1_service_in_state)
      {
         common::unittest::Trace trace;

         local::Domain domain;

         auto state = local::call::state();

         EXPECT_TRUE( local::has_services( state.services, { admin::service::name::state()}));
         EXPECT_FALSE( local::has_services( state.services, { "non/existent/service"}));
      }


      TEST( casual_broker, advertise_2_services_for_1_server__expect__3_services)
      {
         common::unittest::Trace trace;

         local::Domain domain;

         mockup::domain::echo::Server server{ { { "service1"}, { "service2"}}};

         auto state = local::call::state();

         ASSERT_TRUE( local::has_services( state.services, { admin::service::name::state(), "service1", "service2"}));

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

      TEST( casual_broker, advertise_2_services_for_1_server__expect__2_local_instances)
      {
         common::unittest::Trace trace;

         local::Domain domain;

         mockup::domain::echo::Server server{ { { "service1"}, { "service2"}}};

         auto state = local::call::state();

         ASSERT_TRUE( state.instances.local.size() == 2);
         ASSERT_TRUE( state.instances.remote.empty());

         {
            auto instance = local::instance::find( state, server.process().pid);
            ASSERT_TRUE( instance);
            EXPECT_TRUE( instance->state == admin::instance::LocalVO::State::idle);
         }
      }


      TEST( casual_broker, advertise_2_services_for_1_server__prepare_shutdown___expect_instance_exiting)
      {
         common::unittest::Trace trace;

         local::Domain domain;

         mockup::domain::echo::Server server{ { { "service1"}, { "service2"}}};

         {
            common::message::domain::process::prepare::shutdown::Request request;
            request.process = common::process::handle();
            request.processes.push_back( server.process());

            auto reply = common::communication::ipc::call( communication::ipc::broker::device(), request);

            EXPECT_TRUE( request.processes == reply.processes);

         }

         auto state = local::call::state();

         {
            auto instance = local::instance::find( state, server.process().pid);
            ASSERT_TRUE( instance);
            EXPECT_TRUE( instance->state == admin::instance::LocalVO::State::exiting);
         }
      }

      TEST( casual_broker, advertise_2_services_for_1_server__prepare_shutdown__lookup___expect_absent)
      {
         common::unittest::Trace trace;

         local::Domain domain;

         mockup::domain::echo::Server server{ { { "service1"}, { "service2"}}};

         {
            common::message::domain::process::prepare::shutdown::Request request;
            request.process = common::process::handle();
            request.processes.push_back( server.process());

            auto reply = common::communication::ipc::call( communication::ipc::broker::device(), request);

            EXPECT_TRUE( request.processes == reply.processes);

         }

         EXPECT_THROW({
            auto service = service::Lookup{ "service1"}();
         }, exception::xatmi::service::no::Entry);

         EXPECT_THROW({
            auto service = service::Lookup{ "service2"}();
         }, exception::xatmi::service::no::Entry);
      }

		TEST( casual_broker, advertise_new_services_current_server)
      {
		   common::unittest::Trace trace;

		   local::Domain domain;

		   mockup::domain::echo::Server server{ { { "service1"}, { "service2"}}};

		   {
		      auto service = service::Lookup{ "service1"}();
		      EXPECT_TRUE( service.service.name == "service1");
		      EXPECT_TRUE( service.process == server.process());
		      EXPECT_TRUE( service.state == decltype( service)::State::idle);
		   }

         {
            auto service = service::Lookup{ "service2"}();
            EXPECT_TRUE( service.service.name == "service2");

            // we only have one instance, we expect this to be busy
            EXPECT_TRUE( service.state == decltype( service)::State::busy);
         }
      }


		TEST( casual_broker, unadvertise_service)
      {
         common::unittest::Trace trace;

         local::Domain domain;

         mockup::domain::echo::Server server{ { { "service1"}, { "service2"}}};

         server.undadvertise( { { "service2"}});

         //
         // echo server has unadvertise this service. The service is
         // still "present" in broker with no instances. Hence it's absent
         //
         EXPECT_THROW({
            auto service = service::Lookup{ "service2"}();
         }, exception::xatmi::service::no::Entry);
      }



      TEST( casual_broker, service_lookup_non_existent__expect_absent_reply)
      {
         common::unittest::Trace trace;

         local::Domain domain;

         mockup::domain::echo::Server server{ { { "service1"}, { "service2"}}};

         EXPECT_THROW({
            auto service = service::Lookup{ "non-existent-service"}();
         }, exception::xatmi::service::no::Entry);
      }


      TEST( casual_broker, service_lookup_service1__expect__busy_reply__send_ack____expect__idle_reply)
      {
         common::unittest::Trace trace;

         local::Domain domain;

         mockup::domain::echo::Server server{ { { "service1"}, { "service2"}}};

         {
            auto service = service::Lookup{ "service1"}();
            EXPECT_TRUE( service.service.name == "service1");
            EXPECT_TRUE( service.process == server.process());
            EXPECT_TRUE( service.state == decltype( service)::State::idle);
         }

         service::Lookup lookup{ "service2"};
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


      TEST( casual_broker, service_lookup_service1__service_lookup_service1_forward_context____expect_forward_reply)
      {
         common::unittest::Trace trace;

         local::Domain domain;

         mockup::domain::echo::Server server{ { "service1"}};

         {
            auto service = service::Lookup{ "service1"}();
            EXPECT_TRUE( service.service.name == "service1");
            EXPECT_TRUE( service.process == server.process());
            EXPECT_TRUE( service.state == decltype( service)::State::idle);
         }


         {
            service::Lookup lookup{ "service1", service::Lookup::Context::forward};
            auto service = lookup();
            EXPECT_TRUE( service.service.name == "service1");

            // broker will let us think that the service is idle, and send us the queue to the forward-cache
            EXPECT_TRUE( service.state == decltype( service)::State::idle);
            EXPECT_TRUE( service.process.queue != 0);
            EXPECT_TRUE( service.process.queue != server.process().queue);
         }
      }
	}
}

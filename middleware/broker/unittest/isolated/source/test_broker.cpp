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

               }

               ~Broker()
               {
               }

               mockup::Process process;

            };

            struct Domain
            {
               mockup::domain::Manager manager;
               local::Broker broker;
               mockup::domain::transaction::Manager tm;
            };


         } // <unnamed>
      } // local


      TEST( casual_broker, admin_services)
      {
         common::unittest::Trace trace;

         broker::State state;

         auto arguments = broker::admin::services( state);

         EXPECT_TRUE( arguments.services.at( 0).origin == ".casual.broker.state");
      }


      TEST( casual_broker, startup_shutdown__expect_no_throw)
      {
         common::unittest::Trace trace;

         EXPECT_NO_THROW({
            local::Domain domain;
         });

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


         {
            auto service = service::Lookup{ "service2"}();
            EXPECT_TRUE( service.service.name == "service2");

            //
            // echo server has unadvertise this service. The service is
            // still "present" in broker with no instances. Hence it's absent
            //
            EXPECT_TRUE( service.state == decltype( service)::State::absent) << "service: " << service;
         }
      }



      TEST( casual_broker, service_lookup_non_existent__expect_absent_reply)
      {
         common::unittest::Trace trace;

         local::Domain domain;

         mockup::domain::echo::Server server{ { { "service1"}, { "service2"}}};

         {
            auto service = service::Lookup{ "non-existent-service"}();
            EXPECT_TRUE( service.service.name == "non-existent-service");
            EXPECT_TRUE( service.state == decltype( service)::State::absent);
         }
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
            server.send_ack( "service1");

            // get next pending reply
            auto service = lookup();

            EXPECT_TRUE( service.state == decltype( service)::State::idle);
         }
      }


      /*
       *
      TEST( casual_broker, service_lookup_service1__forward_context___expect__forward_reply)
      {
         local::domain_4 domain;
         domain.instance1().state = state::Server::Instance::State::busy;

         {
            local::Broker broker{ domain.state};

            common::message::service::lookup::Request request;
            request.process = domain.server2.process();
            request.requested = "service1";
            request.context =  common::message::service::lookup::Request::Context::no_reply;

            auto correlation = communication::ipc::blocking::send( broker.queue_id, request);


            common::message::service::lookup::Reply reply;
            communication::ipc::blocking::receive( domain.server2.output(), reply);

            EXPECT_TRUE( correlation == reply.correlation);
            EXPECT_TRUE( reply.process == domain.state.forward) << "process: " <<  reply.process;
            EXPECT_TRUE( reply.state == common::message::service::lookup::Reply::State::idle);
         }
         EXPECT_TRUE( domain.state.pending.requests.empty());
      }


      TEST( casual_broker, forward_connect)
      {
         local::domain_3 domain;

         {
            local::Broker broker{ domain.state};

            common::message::forward::connect::Request connect;
            connect.process = domain.server2.process();

            communication::ipc::blocking::send( broker.queue_id, connect);
         }
         EXPECT_TRUE( domain.state.forward == domain.server2.process());
      }


		TEST( casual_broker, traffic_connect)
      {
         local::domain_6 domain;

         {
            local::Broker broker{ domain.state};

            common::message::traffic::monitor::connect::Request connect;
            connect.process = domain.traffic1.process();

            communication::ipc::blocking::send( broker.queue_id, connect);
         }
         EXPECT_TRUE( domain.state.traffic.monitors.at( 0) == domain.traffic1.process().queue);
      }

      TEST( casual_broker, traffic_connect_x2__expect_2_traffic_monitors)
      {
         local::domain_6 domain;

         {
            local::Broker broker{ domain.state};

            {
               common::message::traffic::monitor::connect::Request connect;
               connect.process = domain.traffic1.process();

               communication::ipc::blocking::send( broker.queue_id, connect);
            }
            {
               common::message::traffic::monitor::connect::Request connect;
               connect.process = domain.traffic2.process();

               communication::ipc::blocking::send( broker.queue_id, connect);
            }

         }
         EXPECT_TRUE( domain.state.traffic.monitors.at( 0) == domain.traffic1.process().queue);
         EXPECT_TRUE( domain.state.traffic.monitors.at( 1) == domain.traffic2.process().queue);
      }

		TEST( casual_broker, traffic_disconnect)
      {
         local::domain_3 domain;
         domain.state.traffic.monitors.push_back( domain.server2.process().queue);

         {
            local::Broker broker{ domain.state};

            common::message::traffic::monitor::Disconnect disconnect;
            disconnect.process = domain.server2.process();

            communication::ipc::blocking::send( broker.queue_id, disconnect);
         }
         EXPECT_TRUE( domain.state.traffic.monitors.empty());
      }
      */
	}
}

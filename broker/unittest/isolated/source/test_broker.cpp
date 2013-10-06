//!
//! casual_isolatedunittest_broker.cpp
//!
//! Created on: May 5, 2012
//!     Author: Lazan
//!




#include <gtest/gtest.h>

#include "broker/broker.h"
#include "broker/broker_implementation.h"


#include "common/mockup.h"


namespace casual
{

	namespace broker
	{

	   namespace local
	   {
	      State initializeState()
	      {
	         State state;

	         state.transactionManager = std::make_shared< broker::Server>();
	         auto transactionManager = std::make_shared< broker::Server::Instance>();
	         transactionManager->pid = 1;
	         transactionManager->queue_id = 1;
	         transactionManager->state = broker::Server::Instance::State::idle;
	         transactionManager->server = state.transactionManager;
	         state.transactionManager->instances.push_back( transactionManager);


	         auto group1 = std::make_shared< broker::Group>();
	         group1->name ="group1";
	         group1->resource.emplace_back( 3, "db", "openinfo string", "closeinfo string");

	         auto server1 = std::make_shared< broker::Server>();
	         server1->memberships.push_back( group1);
	         state.servers[ "server1"] = server1;
	         server1->path = "/a/b/c";

	         auto instance1 = std::make_shared< broker::Server::Instance>();
	         instance1->pid = 10;
	         instance1->queue_id = 10;
	         instance1->state = broker::Server::Instance::State::idle;
	         auto instance2 = std::make_shared< broker::Server::Instance>();
	         instance2->pid = 20;
	         instance2->queue_id = 20;
	         instance2->state = broker::Server::Instance::State::idle;

	         state.instances[ 10] = instance1;
	         state.instances[ 20] = instance2;

	         instance1->server = server1;
	         instance2->server = server1;

	         server1->instances.push_back( instance1);
	         server1->instances.push_back( instance2);

	         auto service1 = std::make_shared< broker::Service>();
	         service1->information.name = "service1";
	         auto service2 = std::make_shared< broker::Service>();
	         service2->information.name = "service2";

	         state.services[ "service1"] = service1;
	         state.services[ "service2"] = service2;

	         service1->instances.push_back( instance1);
	         service1->instances.push_back( instance2);

	         service2->instances.push_back( instance1);
	         service2->instances.push_back( instance2);

	         instance1->services.push_back( service1);
	         instance1->services.push_back( service2);

	         instance2->services.push_back( service1);
	         instance2->services.push_back( service1);

	         return state;
	      }

	   }

	   TEST( casual_broker, internal_remove_instance)
	   {
	      State state = local::initializeState();

	      action::remove::instance( 10, state);

	      ASSERT_TRUE( state.instances.size() == 1);
	      EXPECT_TRUE( state.services.at( "service1")->instances.size() == 1);
	      EXPECT_TRUE( state.services.at( "service1")->instances.at( 0)->pid == 20);

	   }


		TEST( casual_broker, server_disconnect)
		{
		   State state = local::initializeState();

		   message::server::Disconnect message;
		   message.server.pid = 20;

		   handle::Disconnect handler( state);
		   handler.dispatch( message);


		   ASSERT_TRUE( state.instances.size() == 1);
		   auto& instance = state.instances.at( 10);
		   EXPECT_TRUE( instance->pid == 10);
		   ASSERT_TRUE( instance->server != nullptr);
		   EXPECT_TRUE( instance->server->instances.size() == 1);

		   EXPECT_TRUE( state.services.size() == 2);
		   ASSERT_TRUE( state.services.at( "service1")->instances.size() == 1);
		   EXPECT_TRUE( state.services.at( "service1")->instances.front()->pid == 10);
		}

		TEST( casual_broker, advertise_new_services_current_server)
      {
         State state = local::initializeState();

         //
         // Add two new services to instance "10"
         //
         message::service::Advertise message;
         message.server.pid = 10;
         message.services.resize( 2);
         message.services.at( 0).name = "service3";
         message.services.at( 1).name = "service4";


         handle::Advertise handler( state);
         handler.dispatch( message);

         EXPECT_TRUE( state.instances.size() == 2);
         EXPECT_TRUE( state.instances.at( 10)->pid == 10);
         EXPECT_TRUE( state.instances.at( 10)->services.size() == 4);

         EXPECT_TRUE( state.services.size() == 4);
         ASSERT_TRUE( state.services.at( "service3")->instances.size() == 1);
         EXPECT_TRUE( state.services.at( "service3")->instances.front()->pid == 10);

         ASSERT_TRUE( state.services.at( "service4")->instances.size() == 1);
         EXPECT_TRUE( state.services.at( "service4")->instances.front()->pid == 10);
      }

		TEST( casual_broker, connect_server__gives_reply_with_resources)
      {
         State state = local::initializeState();

         typedef queue::blocking::basic_writer<
                        policy::Broker,
                        mockup::queue::blocking::base_writer< message::server::Configuration>> mockup_writer;

         mockup_writer::reset();



         state.transactionManagerQueue = 100;

         //
         // Connect new server, with two services
         //
         message::server::Connect message;
         message.server.pid = 20;
         message.services.resize( 2);
         message.services.at( 0).name = "service1";
         message.services.at( 1).name = "service2";

         typedef handle::basic_connect< queue::ipc_wrapper< mockup_writer>> connect_handler_type;
         connect_handler_type handler( state);
         handler.dispatch( message);

         EXPECT_TRUE( state.instances.size() == 2);
         EXPECT_TRUE( state.instances.at( 20)->pid == 20);

         // Check that the "configuration" has been "sent"
         ASSERT_TRUE( mockup_writer::queue.size() == 1);
         EXPECT_TRUE( mockup_writer::queue.front().transactionManagerQueue == 100);
         ASSERT_TRUE( mockup_writer::queue.front().resourceManagers.size() == 1);
         EXPECT_TRUE( mockup_writer::queue.front().resourceManagers.at( 0).key == "db");
         EXPECT_TRUE( mockup_writer::queue.front().resourceManagers.at( 0).openinfo == "openinfo string");
         EXPECT_TRUE( mockup_writer::queue.front().resourceManagers.at( 0).closeinfo == "closeinfo string");
      }


		TEST( casual_broker, connect_new_server_new_services)
      {
         State state = local::initializeState();

         typedef queue::blocking::basic_writer<
               policy::Broker,
               mockup::queue::blocking::base_writer< message::server::Configuration>> mockup_writer;

         mockup_writer::reset();



         state.transactionManagerQueue = 100;

         //
         // Connect new server, with two services
         //
         message::server::Connect message;
         message.server.pid = 30;
         message.services.resize( 2);
         message.services.at( 0).name = "service3";
         message.services.at( 1).name = "service4";

         typedef handle::basic_connect< queue::ipc_wrapper< mockup_writer>> connect_handler_type;
         connect_handler_type handler( state);
         handler.dispatch( message);

         EXPECT_TRUE( state.instances.size() == 3);
         EXPECT_TRUE( state.instances.at( 30)->pid == 30);

         EXPECT_TRUE( state.services.size() == 4);
         ASSERT_TRUE( state.services.at( "service3")->instances.size() == 1);
         EXPECT_TRUE( state.services.at( "service3")->instances.front()->pid == 30);

         ASSERT_TRUE( state.services.at( "service4")->instances.size() == 1);
         EXPECT_TRUE( state.services.at( "service4")->instances.front()->pid == 30);

         // Check that the "configuration" has been "sent"
         ASSERT_TRUE( mockup_writer::queue.size() == 1);
         EXPECT_TRUE( mockup_writer::queue.front().transactionManagerQueue == 100);
      }


		TEST( casual_broker, connect_new_server_current_services)
      {
         State state = local::initializeState();

         typedef queue::blocking::basic_writer<
                        policy::Broker,
                        mockup::queue::blocking::base_writer< message::server::Configuration>> mockup_writer;
         mockup_writer::reset();

         state.transactionManagerQueue = 100;

         //
         // Add two new services to NEW server 30
         //
         message::server::Connect message;
         message.server.pid = 30;
         message.services.resize( 2);
         message.services.at( 0).name = "service1";
         message.services.at( 1).name = "service2";


         typedef handle::basic_connect< queue::ipc_wrapper< mockup_writer>> connect_handler_type;
         connect_handler_type handler( state);
         handler.dispatch( message);


         EXPECT_TRUE( state.instances.size() == 3);
         EXPECT_TRUE( state.instances.at( 30)->pid == 30);

         EXPECT_TRUE( state.services.size() == 2);
         ASSERT_TRUE( state.services.at( "service1")->instances.size() == 3);
         EXPECT_TRUE( state.services.at( "service1")->instances.at( 2)->pid == 30);

         ASSERT_TRUE( state.services.at( "service2")->instances.size() == 3);
         EXPECT_TRUE( state.services.at( "service2")->instances.at( 2)->pid == 30);

         // Check that the "configuration" has been "sent"
         ASSERT_TRUE( mockup_writer::queue.size() == 1);
         EXPECT_TRUE( mockup_writer::queue.front().transactionManagerQueue == 100);
      }


		TEST( casual_broker, unadvertise_service)
      {
         State state = local::initializeState();

         //
         //
         //
         message::service::Unadvertise message;
         message.server.pid = 20;
         message.services.resize( 2);
         message.services.at( 0).name = "service1";
         message.services.at( 1).name = "service2";


         handle::Unadvertise handler( state);
         handler.dispatch( message);

         //
         // Even if all server "20"'s services are unadvertised, we keep the
         // server.
         //
         EXPECT_TRUE( state.instances.size() == 2);
         EXPECT_TRUE( state.instances.at( 20)->pid == 20);

         EXPECT_TRUE( state.services.size() == 2);
         ASSERT_TRUE( state.services.at( "service1")->instances.size() == 1);
         EXPECT_TRUE( state.services.at( "service1")->instances.at( 0)->pid == 10);

         ASSERT_TRUE( state.services.at( "service2")->instances.size() == 1);
         EXPECT_TRUE( state.services.at( "service2")->instances.at( 0)->pid == 10);
      }


		/*
		TEST( casual_broker, extract_services)
      {
         State state = local::initializeState();

         std::vector< std::string> result = extract::services( 10, state);

         ASSERT_TRUE( result.size() == 2);
         EXPECT_TRUE( result.at( 0) == "service1");
         EXPECT_TRUE( result.at( 1) == "service2");

      }
      */


		TEST( casual_broker, service_request)
      {
         State state = local::initializeState();

         typedef queue::blocking::basic_writer<
               policy::Broker,
               mockup::queue::blocking::base_writer< message::service::name::lookup::Reply>> mockup_writer;

         mockup_writer::reset();


         message::service::name::lookup::Request message;
         message.requested = "service1";
         message.server.pid = 30;
         message.server.queue_id = 30;

         handle::basic_servicelookup< queue::ipc_wrapper< mockup_writer>> handler( state);
         handler.dispatch( message);



         // instance should be busy
         EXPECT_TRUE( state.instances.at( 10)->state == Server::Instance::State::busy);
         // other instance should still be idle
         EXPECT_TRUE( state.instances.at( 20)->state == Server::Instance::State::idle);

         ASSERT_TRUE( mockup_writer::queue.size() == 1);
         EXPECT_TRUE( mockup_writer::queue.front().service.name == "service1");
         ASSERT_TRUE( mockup_writer::queue.front().server.size() == 1);
         EXPECT_TRUE( mockup_writer::queue.front().server.at( 0).pid == 10);
         EXPECT_TRUE( mockup_writer::queue.front().server.at( 0).queue_id == 10);
      }

		TEST( casual_broker, service_request_pending)
      {
         State state = local::initializeState();

         typedef queue::blocking::basic_writer<
                        policy::Broker,
                        mockup::queue::blocking::base_writer< message::service::name::lookup::Reply>> mockup_writer;


         mockup_writer::reset();

         // make servers busy
         state.instances.at( 10)->state = Server::Instance::State::busy;
         state.instances.at( 20)->state = Server::Instance::State::busy;

         message::service::name::lookup::Request message;
         message.requested = "service1";
         message.server.pid = 30;
         message.server.queue_id = 30;

         handle::basic_servicelookup< queue::ipc_wrapper< mockup_writer>> handler( state);
         handler.dispatch( message);


         // no reply should have been sent
         EXPECT_TRUE( mockup_writer::queue.empty());

         // we should have a pending request
         ASSERT_TRUE( state.pending.size() == 1);
         EXPECT_TRUE( state.pending.at( 0).requested == "service1");
         EXPECT_TRUE( state.pending.at( 0).server.pid == 30);
         EXPECT_TRUE( state.pending.at( 0).server.queue_id == 30);

      }

		TEST( casual_broker, service_done)
      {
         State state = local::initializeState();

         typedef queue::blocking::basic_writer<
               policy::Broker,
               mockup::queue::blocking::base_writer< message::service::name::lookup::Reply>> mockup_writer;

         mockup_writer::reset();

         // make server busy
         state.instances.at( 10)->state = Server::Instance::State::busy;

         message::service::ACK message;
         message.service = "service1";
         message.server.pid = 10;
         message.server.queue_id = 10;

         handle::basic_ack< queue::ipc_wrapper< mockup_writer>> handler( state);
         handler.dispatch( message);



         // no reply should have been sent
         EXPECT_TRUE( mockup_writer::queue.empty());
         // server should be idle
         EXPECT_TRUE( state.instances.at( 10)->state == Server::Instance::State::idle);
      }

		TEST( casual_broker, service_done_pending_requests)
      {
         State state = local::initializeState();

         typedef queue::blocking::basic_writer<
               policy::Broker,
               mockup::queue::blocking::base_writer< message::service::name::lookup::Reply>> mockup_writer;

         mockup_writer::reset();

         // make servers busy
         state.instances.at( 10)->state = Server::Instance::State::busy;
         state.instances.at( 20)->state = Server::Instance::State::busy;

         // make sure we have a pending request
         message::service::name::lookup::Request request;
         request.requested = "service1";
         request.server.pid = 30;
         request.server.queue_id = 30;

         state.pending.push_back( request);

         // server "10" is ready for action...
         message::service::ACK message;
         message.service = "service1";
         message.server.pid = 10;
         message.server.queue_id = 10;

         // we should get the pending response
         handle::basic_ack< queue::ipc_wrapper< mockup_writer>> handler( state);
         handler.dispatch( message);

         // The server should still be busy
         EXPECT_TRUE( state.instances.at( 10)->state == Server::Instance::State::busy);

         ASSERT_TRUE( mockup_writer::queue.size() == 1);
         // pending queue response is sent to
         EXPECT_TRUE( mockup_writer::queue_id == 30);
         // response sent to queue "30"
         EXPECT_TRUE( mockup_writer::queue.front().service.name == "service1");
         ASSERT_TRUE( mockup_writer::queue.front().server.size() == 1);
         EXPECT_TRUE( mockup_writer::queue.front().server.front().pid == 10);
         EXPECT_TRUE( mockup_writer::queue.front().server.front().queue_id == 10);


      }


		TEST( casual_broker, monitor_connect)
      {
		   State state = local::initializeState();
		   state.monitorQueue = 0;

		   //
         // Connect
         //
         message::monitor::Connect message;
         message.server.pid = 50;
         message.server.queue_id = 50;

         handle::MonitorConnect handler( state);
         handler.dispatch( message);


         EXPECT_TRUE( state.monitorQueue == 50);
      }

		TEST( casual_broker, monitor_disconnect)
      {
         State state = local::initializeState();
         state.monitorQueue = 0;

         //
         // Add two new services to NEW server 30
         //
         message::monitor::Disconnect message;
         message.server.pid = 50;
         message.server.queue_id = 50;

         handle::MonitorDisconnect handler( state);
         handler.dispatch( message);


         EXPECT_TRUE( state.monitorQueue == 0);
      }

/*
		TEST( casual_broker, transaction_manager_connect)
      {
         State state = local::initializeState();
         state.transactionManagerQueue = 0;

         //
         // Connect
         //
         message::transaction::Connect message;
         message.server.pid = 50;
         message.server.queue_id = 50;

         typedef common::mockup::queue::blocking::base_writer< message::transaction::Configuration> tm_queue_type;

         handle::transaction::basic_manager_connect< tm_queue_type> handler( state);
         handler.dispatch( message);


         EXPECT_TRUE( state.transactionManagerQueue == 50);
         EXPECT_TRUE( tm_queue_type::queue.size() == 1);
      }
      */

	}
}

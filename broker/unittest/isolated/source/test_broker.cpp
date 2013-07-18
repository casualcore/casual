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

	         state.servers[ 10].path = "a/b/c";
	         state.servers[ 10].pid = 10;
	         state.servers[ 10].queue_key = 10;

	         state.servers[ 20].path = "d/e/f";
            state.servers[ 20].pid = 20;
            state.servers[ 20].queue_key = 20;

	         state.services[ "service1"].information.name = "service1";
	         state.services[ "service1"].servers.push_back( &state.servers[ 10]);
	         state.services[ "service1"].servers.push_back( &state.servers[ 20]);

	         state.services[ "service2"].information.name = "service2";
            state.services[ "service2"].servers.push_back( &state.servers[ 10]);
            state.services[ "service2"].servers.push_back( &state.servers[ 20]);

	         return state;
	      }

	   }


		TEST( casual_broker, server_disconnect)
		{
		   State state = local::initializeState();

		   message::server::Disconnect message;
		   message.serverId.pid = 20;

		   handle::Disconnect handler( state);
		   handler.dispatch( message);


		   EXPECT_TRUE( state.servers.size() == 1);
		   EXPECT_TRUE( state.servers[ 10].pid == 10);

		   EXPECT_TRUE( state.services.size() == 2);
		   ASSERT_TRUE( state.services[ "service1"].servers.size() == 1);
		   EXPECT_TRUE( state.services[ "service1"].servers.front()->pid == 10);
		}

		TEST( casual_broker, advertise_new_services_current_server)
      {
         State state = local::initializeState();

         //
         // Add two new services to server "10"
         //
         message::service::Advertise message;
         message.serverId.pid = 10;
         message.services.resize( 2);
         message.services.at( 0).name = "service3";
         message.services.at( 1).name = "service4";


         handle::Advertise handler( state);
         handler.dispatch( message);

         EXPECT_TRUE( state.servers.size() == 2);
         EXPECT_TRUE( state.servers[ 10].pid == 10);

         EXPECT_TRUE( state.services.size() == 4);
         ASSERT_TRUE( state.services[ "service3"].servers.size() == 1);
         EXPECT_TRUE( state.services[ "service3"].servers.front()->pid == 10);

         ASSERT_TRUE( state.services[ "service4"].servers.size() == 1);
         EXPECT_TRUE( state.services[ "service4"].servers.front()->pid == 10);
      }


		TEST( casual_broker, connect_new_server_new_services)
      {
         State state = local::initializeState();
         typedef common::mockup::queue::WriteMessage< message::server::Configuration> mockup_writer;
         mockup_writer::reset();

         state.transactionManagerQueue = 100;

         //
         // Connect new server, with two services
         //
         message::server::Connect message;
         message.serverId.pid = 30;
         message.services.resize( 2);
         message.services.at( 0).name = "service3";
         message.services.at( 1).name = "service4";

         typedef handle::basic_connect< queue::ipc_wrapper< mockup_writer>> connect_handler_type;
         connect_handler_type handler( state);
         handler.dispatch( message);

         EXPECT_TRUE( state.servers.size() == 3);
         EXPECT_TRUE( state.servers[ 30].pid == 30);

         EXPECT_TRUE( state.services.size() == 4);
         ASSERT_TRUE( state.services[ "service3"].servers.size() == 1);
         EXPECT_TRUE( state.services[ "service3"].servers.front()->pid == 30);

         ASSERT_TRUE( state.services[ "service4"].servers.size() == 1);
         EXPECT_TRUE( state.services[ "service4"].servers.front()->pid == 30);

         // Check that the "configuration" has been "sent"
         ASSERT_TRUE( mockup_writer::replies.size() == 1);
         EXPECT_TRUE( mockup_writer::replies.front().transactionManagerQueue == 100);
      }


		TEST( casual_broker, connect_new_server_current_services)
      {
         State state = local::initializeState();

         typedef common::mockup::queue::WriteMessage< message::server::Configuration> mockup_writer;
         mockup_writer::reset();

         state.transactionManagerQueue = 100;

         //
         // Add two new services to NEW server 30
         //
         message::server::Connect message;
         message.serverId.pid = 30;
         message.services.resize( 2);
         message.services.at( 0).name = "service1";
         message.services.at( 1).name = "service2";


         typedef handle::basic_connect< queue::ipc_wrapper< mockup_writer>> connect_handler_type;
         connect_handler_type handler( state);
         handler.dispatch( message);


         EXPECT_TRUE( state.servers.size() == 3);
         EXPECT_TRUE( state.servers[ 30].pid == 30);

         EXPECT_TRUE( state.services.size() == 2);
         ASSERT_TRUE( state.services[ "service1"].servers.size() == 3);
         EXPECT_TRUE( state.services[ "service1"].servers.at( 2)->pid == 30);

         ASSERT_TRUE( state.services[ "service2"].servers.size() == 3);
         EXPECT_TRUE( state.services[ "service2"].servers.at( 2)->pid == 30);

         // Check that the "configuration" has been "sent"
         ASSERT_TRUE( mockup_writer::replies.size() == 1);
         EXPECT_TRUE( mockup_writer::replies.front().transactionManagerQueue == 100);
      }


		TEST( casual_broker, unadvertise_service)
      {
         State state = local::initializeState();

         //
         // Add two new services to NEW server 30
         //
         message::service::Unadvertise message;
         message.serverId.pid = 20;
         message.services.resize( 2);
         message.services.at( 0).name = "service1";
         message.services.at( 1).name = "service2";


         handle::Unadvertise handler( state);
         handler.dispatch( message);

         //
         // Even if all server "20"'s services are unadvertised, we keep the
         // server.
         //
         EXPECT_TRUE( state.servers.size() == 2);
         EXPECT_TRUE( state.servers[ 20].pid == 20);

         EXPECT_TRUE( state.services.size() == 2);
         ASSERT_TRUE( state.services[ "service1"].servers.size() == 1);
         EXPECT_TRUE( state.services[ "service1"].servers.at( 0)->pid == 10);

         ASSERT_TRUE( state.services[ "service2"].servers.size() == 1);
         EXPECT_TRUE( state.services[ "service2"].servers.at( 0)->pid == 10);
      }


		TEST( casual_broker, extract_services)
      {
         State state = local::initializeState();

         std::vector< std::string> result = extract::services( 10, state);

         ASSERT_TRUE( result.size() == 2);
         EXPECT_TRUE( result.at( 0) == "service1");
         EXPECT_TRUE( result.at( 1) == "service2");

      }


		TEST( casual_broker, service_request)
      {
         State state = local::initializeState();
         typedef common::mockup::queue::WriteMessage< message::service::name::lookup::Reply> mockup_writer;
         mockup_writer::reset();


         message::service::name::lookup::Request message;
         message.requested = "service1";
         message.server.pid = 30;
         message.server.queue_key = 30;

         handle::basic_servicelookup<  mockup_writer> handler( state);
         handler.dispatch( message);



         // server should not be idle
         EXPECT_TRUE( state.servers[ 10].idle == false);
         // other server should still be idle
         EXPECT_TRUE( state.servers[ 20].idle == true);

         ASSERT_TRUE( mockup_writer::replies.size() == 1);
         EXPECT_TRUE( mockup_writer::replies.front().service.name == "service1");
         ASSERT_TRUE( mockup_writer::replies.front().server.size() == 1);
         EXPECT_TRUE( mockup_writer::replies.front().server.at( 0).pid == 10);
         EXPECT_TRUE( mockup_writer::replies.front().server.at( 0).queue_key == 10);
      }

		TEST( casual_broker, service_request_pending)
      {
         State state = local::initializeState();
         typedef common::mockup::queue::WriteMessage< message::service::name::lookup::Reply> mockup_writer;
         mockup_writer::reset();

         // make servers busy
         state.servers[ 10].idle = false;
         state.servers[ 20].idle = false;

         message::service::name::lookup::Request message;
         message.requested = "service1";
         message.server.pid = 30;
         message.server.queue_key = 30;

         handle::basic_servicelookup< mockup_writer> handler( state);
         handler.dispatch( message);


         // no reply should have been sent
         EXPECT_TRUE( mockup_writer::replies.empty());

         // we should have a pending request
         ASSERT_TRUE( state.pending.size() == 1);
         EXPECT_TRUE( state.pending.at( 0).requested == "service1");
         EXPECT_TRUE( state.pending.at( 0).server.pid == 30);
         EXPECT_TRUE( state.pending.at( 0).server.queue_key == 30);

      }

		TEST( casual_broker, service_done)
      {
         State state = local::initializeState();
         typedef common::mockup::queue::WriteMessage< message::service::name::lookup::Reply> mockup_writer;
         mockup_writer::reset();

         // make server busy
         state.servers[ 10].idle = false;

         message::service::ACK message;
         message.service = "service1";
         message.server.pid = 10;
         message.server.queue_key = 10;

         handle::basic_ack< mockup_writer> handler( state);
         handler.dispatch( message);



         // no reply should have been sent
         EXPECT_TRUE( mockup_writer::replies.empty());
         // server should be busy
         EXPECT_TRUE( state.servers[ 10].idle == true);
      }

		TEST( casual_broker, service_done_pending_requests)
      {
         State state = local::initializeState();
         typedef common::mockup::queue::WriteMessage< message::service::name::lookup::Reply> mockup_writer;
         mockup_writer::reset();

         // make servers busy
         state.servers[ 10].idle = false;
         state.servers[ 20].idle = false;

         // make sure we have a pending request
         message::service::name::lookup::Request request;
         request.requested = "service1";
         request.server.pid = 30;
         request.server.queue_key = 30;

         state.pending.push_back( request);

         // server "10" is ready for action...
         message::service::ACK message;
         message.service = "service1";
         message.server.pid = 10;
         message.server.queue_key = 10;

         // we should get the pending response
         handle::basic_ack< mockup_writer> handler( state);
         handler.dispatch( message);

         // The server should still be busy
         EXPECT_TRUE( state.servers[ 10].idle == false);

         ASSERT_TRUE( mockup_writer::replies.size() == 1);
         // pending queue response is sent to
         EXPECT_TRUE( mockup_writer::queue_key == 30);
         // response sent to queue "30"
         EXPECT_TRUE( mockup_writer::replies.front().service.name == "service1");
         ASSERT_TRUE( mockup_writer::replies.front().server.size() == 1);
         EXPECT_TRUE( mockup_writer::replies.front().server.front().pid == 10);
         EXPECT_TRUE( mockup_writer::replies.front().server.front().queue_key == 10);


      }


		TEST( casual_broker, monitor_connect)
      {
		   State state = local::initializeState();
		   state.monitorQueue = 0;

		   //
         // Connect
         //
         message::monitor::Connect message;
         message.serverId.pid = 50;
         message.serverId.queue_key = 50;

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
         message.serverId.pid = 50;
         message.serverId.queue_key = 50;

         handle::MonitorDisconnect handler( state);
         handler.dispatch( message);


         EXPECT_TRUE( state.monitorQueue == 0);
      }


		TEST( casual_broker, transaction_manager_connect)
      {
         State state = local::initializeState();
         state.transactionManagerQueue = 0;

         //
         // Connect
         //
         message::transaction::Connect message;
         message.serverId.pid = 50;
         message.serverId.queue_key = 50;

         handle::TransactionManagerConnect handler( state);
         handler.dispatch( message);


         EXPECT_TRUE( state.transactionManagerQueue == 50);
      }

	}
}

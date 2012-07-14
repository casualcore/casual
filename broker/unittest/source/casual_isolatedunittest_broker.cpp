//!
//! casual_isolatedunittest_broker.cpp
//!
//! Created on: May 5, 2012
//!     Author: Lazan
//!




#include <gtest/gtest.h>

#include "casual_broker.h"
#include "casual_broker_implementation.h"


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

	         state.services[ "service1"].name = "service1";
	         state.services[ "service1"].servers.push_back( &state.servers[ 10]);
	         state.services[ "service1"].servers.push_back( &state.servers[ 20]);

	         state.services[ "service2"].name = "service2";
            state.services[ "service2"].servers.push_back( &state.servers[ 10]);
            state.services[ "service2"].servers.push_back( &state.servers[ 20]);

	         return state;
	      }

	   }


		TEST( casual_broker, server_disconnect)
		{
		   State state = local::initializeState();

		   message::ServerDisconnect message;
		   message.serverId.pid = 20;

		   state::removeServer( message, state);

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
         message::ServiceAdvertise message;
         message.serverId.pid = 10;
         message.services.resize( 2);
         message.services.at( 0).name = "service3";
         message.services.at( 1).name = "service4";


         state::advertiseService( message, state);

         EXPECT_TRUE( state.servers.size() == 2);
         EXPECT_TRUE( state.servers[ 10].pid == 10);

         EXPECT_TRUE( state.services.size() == 4);
         ASSERT_TRUE( state.services[ "service3"].servers.size() == 1);
         EXPECT_TRUE( state.services[ "service3"].servers.front()->pid == 10);

         ASSERT_TRUE( state.services[ "service4"].servers.size() == 1);
         EXPECT_TRUE( state.services[ "service4"].servers.front()->pid == 10);
      }

		TEST( casual_broker, advertise_new_services_new_server)
      {
         State state = local::initializeState();

         //
         // Add two new services to NEW server 30
         //
         message::ServiceAdvertise message;
         message.serverId.pid = 30;
         message.services.resize( 2);
         message.services.at( 0).name = "service3";
         message.services.at( 1).name = "service4";


         state::advertiseService( message, state);

         EXPECT_TRUE( state.servers.size() == 3);
         EXPECT_TRUE( state.servers[ 30].pid == 30);

         EXPECT_TRUE( state.services.size() == 4);
         ASSERT_TRUE( state.services[ "service3"].servers.size() == 1);
         EXPECT_TRUE( state.services[ "service3"].servers.front()->pid == 30);

         ASSERT_TRUE( state.services[ "service4"].servers.size() == 1);
         EXPECT_TRUE( state.services[ "service4"].servers.front()->pid == 30);
      }


		TEST( casual_broker, advertise_current_services_new_server)
      {
         State state = local::initializeState();

         //
         // Add two new services to NEW server 30
         //
         message::ServiceAdvertise message;
         message.serverId.pid = 30;
         message.services.resize( 2);
         message.services.at( 0).name = "service1";
         message.services.at( 1).name = "service2";


         state::advertiseService( message, state);

         EXPECT_TRUE( state.servers.size() == 3);
         EXPECT_TRUE( state.servers[ 30].pid == 30);

         EXPECT_TRUE( state.services.size() == 2);
         ASSERT_TRUE( state.services[ "service1"].servers.size() == 3);
         EXPECT_TRUE( state.services[ "service1"].servers.at( 2)->pid == 30);

         ASSERT_TRUE( state.services[ "service2"].servers.size() == 3);
         EXPECT_TRUE( state.services[ "service2"].servers.at( 2)->pid == 30);
      }


		TEST( casual_broker, unadvertise_service)
      {
         State state = local::initializeState();

         //
         // Add two new services to NEW server 30
         //
         message::ServiceUnadvertise message;
         message.serverId.pid = 20;
         message.services.resize( 2);
         message.services.at( 0).name = "service1";
         message.services.at( 1).name = "service2";

         state::unadvertiseService( message, state);

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

         message::ServiceRequest message;
         message.requested = "service1";
         message.server.pid = 30;
         message.server.queue_key = 30;

         std::vector< message::ServiceResponse> response = state::requestService( message, state);

         // server should not be idle
         EXPECT_TRUE( state.servers[ 10].idle == false);
         // other server should still be idle
         EXPECT_TRUE( state.servers[ 20].idle == true);

         ASSERT_TRUE( response.size() == 1);
         EXPECT_TRUE( response.at( 0).service.name == "service1");
         ASSERT_TRUE( response.at( 0).server.size() == 1);
         EXPECT_TRUE( response.at( 0).server.at( 0).pid == 10);
         EXPECT_TRUE( response.at( 0).server.at( 0).queue_key == 10);
      }

		TEST( casual_broker, service_request_pending)
      {
         State state = local::initializeState();

         // make servers busy
         state.servers[ 10].idle = false;
         state.servers[ 20].idle = false;

         message::ServiceRequest message;
         message.requested = "service1";
         message.server.pid = 30;
         message.server.queue_key = 30;

         std::vector< message::ServiceResponse> response = state::requestService( message, state);

         EXPECT_TRUE( response.empty());

         // we should have a pending request
         ASSERT_TRUE( state.pending.size() == 1);
         EXPECT_TRUE( state.pending.at( 0).requested == "service1");
         EXPECT_TRUE( state.pending.at( 0).server.pid == 30);
         EXPECT_TRUE( state.pending.at( 0).server.queue_key == 30);

      }

		TEST( casual_broker, service_done)
      {
         State state = local::initializeState();

         // make server busy
         state.servers[ 10].idle = false;

         message::ServiceACK message;
         message.service = "service1";
         message.server.pid = 10;
         message.server.queue_key = 10;

         std::vector< state::PendingResponse> response = state::serviceDone( message, state);

         EXPECT_TRUE( response.empty());
         EXPECT_TRUE( state.servers[ 10].idle == true);
      }

		TEST( casual_broker, service_done_pending_requests)
      {
         State state = local::initializeState();

         // make servers busy
         state.servers[ 10].idle = false;
         state.servers[ 20].idle = false;

         // make sure we have a pending request
         message::ServiceRequest request;
         request.requested = "service1";
         request.server.pid = 30;
         request.server.queue_key = 30;

         state.pending.push_back( request);

         // server "10" is ready for action...
         message::ServiceACK message;
         message.service = "service1";
         message.server.pid = 10;
         message.server.queue_key = 10;

         // we should get the pending response
         std::vector< state::PendingResponse> response = state::serviceDone( message, state);

         // The server should still be busy
         EXPECT_TRUE( state.servers[ 10].idle == false);

         ASSERT_TRUE( response.size() == 1);
         // pending queue response is sent to
         EXPECT_TRUE( response.front().first == 30);
         // response sent to queue "30"
         EXPECT_TRUE( response.front().second.service.name == "service1");
         ASSERT_TRUE( response.front().second.server.size() == 1);
         EXPECT_TRUE( response.front().second.server.front().pid == 10);
         EXPECT_TRUE( response.front().second.server.front().queue_key == 10);


      }


	}
}

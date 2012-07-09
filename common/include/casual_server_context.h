//!
//! casual_server_context.h
//!
//! Created on: Apr 1, 2012
//!     Author: Lazan
//!

#ifndef CASUAL_SERVER_CONTEXT_H_
#define CASUAL_SERVER_CONTEXT_H_

#include "casual_service_context.h"
#include "casual_message.h"

#include "casual_ipc.h"

#include "casual_utility_platform.h"

//
// std
//
#include <map>

namespace casual
{
	namespace server
	{

		class Context
		{
		public:
			typedef std::map< std::string, service::Context> service_mapping_type;

			static Context& instance();

			void add( const service::Context& context);

			int start();

			void longJumpReturn( int rval, long rcode, char* data, long len, long flags);


		private:

			Context();

			void connect();

			void handleServiceCall( message::ServiceCall& context);

			service::Context& getService( const std::string& name);


			void cleanUp();


			service_mapping_type m_services;

			ipc::send::Queue& m_brokerQueue;
			ipc::receive::Queue& m_queue;


			utility::platform::long_jump_buffer_type m_long_jump_buffer;

			message::ServiceReply m_reply;

		};


	}

}



#endif /* CASUAL_SERVER_CONTEXT_H_ */

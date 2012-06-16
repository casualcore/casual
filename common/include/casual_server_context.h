//!
//! casual_server_context.h
//!
//! Created on: Apr 1, 2012
//!     Author: Lazan
//!

#ifndef CASUAL_SERVER_CONTEXT_H_
#define CASUAL_SERVER_CONTEXT_H_

#include "casual_service_context.h"

#include "casual_ipc.h"

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


		private:

			Context();

			void connect();

			service_mapping_type m_services;

			ipc::receive::Queue m_queue;
			ipc::send::Queue m_brokerQueue;



		};


	}

}



#endif /* CASUAL_SERVER_CONTEXT_H_ */

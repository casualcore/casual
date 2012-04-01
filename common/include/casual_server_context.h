//!
//! casual_server_context.h
//!
//! Created on: Apr 1, 2012
//!     Author: Lazan
//!

#ifndef CASUAL_SERVER_CONTEXT_H_
#define CASUAL_SERVER_CONTEXT_H_

#include "casual_service_context.h"

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
			static Context& instance();

			void add( const service::Context& context);


		private:
			Context();

			std::map< std::string, service::Context> m_services;


		};


	}

}



#endif /* CASUAL_SERVER_CONTEXT_H_ */

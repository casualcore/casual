//!
//! casual_server_context.cpp
//!
//! Created on: Apr 1, 2012
//!     Author: Lazan
//!

#include "casual_server_context.h"

namespace casual
{
	namespace server
	{


		Context& Context::instance()
		{
			static Context singleton;
			return singleton;
		}

		void Context::add( const service::Context& context)
		{
			// TODO: add validation
			m_services[ context.m_name] = context;
		}

		Context::Context()
		{

		}


	}

}



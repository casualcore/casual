//!
//! xatmi_server.cpp
//!
//! Created on: Mar 28, 2012
//!     Author: Lazan
//!

#include "xatmi_server.h"

// TODO:
#include <iostream>

#include <algorithm>


#include "casual_service_context.h"
#include "casual_server_context.h"


namespace local
{
	namespace
	{
		struct AddServiceContext
		{
			void operator () ( const casual_service_name_mapping& mapping)
			{
				casual::service::Context context( mapping.m_name, mapping.m_functionPointer);

				casual::server::Context::instance().add( context);

			}
		};
	}

}





int casual_startServer( int argc, char** argv, struct casual_service_name_mapping* mapping, size_t size)
{
	std::for_each(
			mapping,
			mapping + size,
			local::AddServiceContext());


	return 0;
}

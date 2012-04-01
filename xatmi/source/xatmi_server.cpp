//!
//! xatmi_server.cpp
//!
//! Created on: Mar 28, 2012
//!     Author: Lazan
//!

#include "xatmi_server.h"

#include <iostream>

int casual_startServer( int argc, char** argv, struct casual_service_name_mapping* mapping, size_t size)
{
	for( size_t index = 0; index < size; ++index)
	{
		std::cout << "service: " << mapping[ index].m_name << std::endl;
	}

	return 0;
}

//!
//! xatm.cpp
//!
//! Created on: Apr 29, 2012
//!     Author: Lazan
//!

#include "xatmi.h"

#include "casual_buffer.h"
#include "casual_utility_string.h"




char* tpalloc(char* type, char* subtype, long size)
{
	casual::buffer::Buffer& buffer = casual::buffer::Holder::instance().allocate(
			casual::utility::string::fromCString( type),
			casual::utility::string::fromCString( subtype),
			size);

	return static_cast< char*>( buffer.m_memory);
}

char* tprealloc(char * addr, long size)
{

	casual::buffer::Buffer& buffer = casual::buffer::Holder::instance().reallocate(
		addr,
		size);

	return static_cast< char*>( buffer.m_memory);

}



long tptypes(char* ptr, char* type, char* subtype)
{

}


void tpfree(char* ptr)
{
	casual::buffer::Holder::instance().deallocate( ptr);
}




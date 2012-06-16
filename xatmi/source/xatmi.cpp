//!
//! xatm.cpp
//!
//! Created on: Apr 29, 2012
//!     Author: Lazan
//!

#include "xatmi.h"

#include "casual_buffer.h"
#include "casual_utility_string.h"




char* tpalloc( const char* type, const char* subtype, long size)
{
	casual::buffer::Buffer& buffer = casual::buffer::Holder::instance().allocate(
			type,
			subtype,
			size);

	return buffer.raw();
}

char* tprealloc(char * addr, long size)
{

	casual::buffer::Buffer& buffer = casual::buffer::Holder::instance().reallocate(
		addr,
		size);

	return buffer.raw();

}



long tptypes(char* ptr, char* type, char* subtype)
{
	return 0;
}


void tpfree(char* ptr)
{
	casual::buffer::Holder::instance().deallocate( ptr);
}




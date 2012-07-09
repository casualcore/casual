//!
//! xatm.cpp
//!
//! Created on: Apr 29, 2012
//!     Author: Lazan
//!

#include "xatmi.h"

#include "casual_buffer_context.h"
#include "casual_utility_string.h"


#include "casual_calling_context.h"
#include "casual_server_context.h"




char* tpalloc( const char* type, const char* subtype, long size)
{
	casual::buffer::Buffer& buffer = casual::buffer::Context::instance().allocate(
			type ? type : "",
			subtype ? subtype : "",
			size);

	return buffer.raw();
}

char* tprealloc(char * addr, long size)
{

	casual::buffer::Buffer& buffer = casual::buffer::Context::instance().reallocate(
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
	casual::buffer::Context::instance().deallocate( ptr);
}


void tpreturn(int rval, long rcode, char* data, long len, long flags)
{
	casual::server::Context::instance().longJumpReturn( rval, rcode, data, len, flags);
}

int tpcall( const char * svc, char* idata, long ilen, char ** odata, long *olen, long flags)
{
	int result = casual::calling::Context::instance().asyncCall( svc, idata, ilen, flags);

	return tpgetrply( &result, odata, olen, flags);
}

int tpacall( const char * svc, char* idata, long ilen, long flags)
{
	return casual::calling::Context::instance().asyncCall( svc, idata, ilen, flags);
}
int tpgetrply(int *idPtr, char ** odata, long *olen, long flags)
{
	return casual::calling::Context::instance().getReply( idPtr, odata, *olen, flags);
}




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

#include "casual_error.h"


//
// Define globals
//
int tperrno = 0;
long tpurcode = 0;


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
   try
   {
      casual::server::Context::instance().longJumpReturn( rval, rcode, data, len, flags);
   }
   catch( ...)
   {
      casual::error::handler();
   }
}

int tpcall( const char * svc, char* idata, long ilen, char ** odata, long *olen, long flags)
{
   try
   {

      int callDescriptor = casual::calling::Context::instance().asyncCall( svc, idata, ilen, flags);

      return casual::calling::Context::instance().getReply( &callDescriptor, odata, *olen, flags);

   }
   catch( ...)
   {
      return casual::error::handler();
   }
   return 0; // remove warning in eclipse
}

int tpacall( const char * svc, char* idata, long ilen, long flags)
{
   try
   {
      return casual::calling::Context::instance().asyncCall( svc, idata, ilen, flags);
   }
	catch( ...)
   {
      return casual::error::handler();
   }
   return 0; // remove warning in eclipse
}

int tpgetrply(int *idPtr, char ** odata, long *olen, long flags)
{
   try
   {
      return casual::calling::Context::instance().getReply( idPtr, odata, *olen, flags);
   }
   catch( ...)
   {
      return casual::error::handler();
   }
   return 0; // remove warning in eclipse
}

int tpadvertise( const char* svcname, void(*func)(TPSVCINFO *))
{
   try
   {
      casual::server::Context::instance().advertiseService( svcname, func);
   }
   catch( ...)
   {
      return casual::error::handler();
   }
   return 0; // remove warning in eclipse
}

int tpunadvertise( const char* svcname)
{
   try
   {
      casual::server::Context::instance().unadvertiseService( svcname);
   }
   catch( ...)
   {
      return casual::error::handler();
   }
   return 0; // remove warning in eclipse
}

const char* tperrnostring( int error)
{
   return casual::error::tperrnoStringRepresentation( error).c_str();
}




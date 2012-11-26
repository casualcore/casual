//!
//! xatm.cpp
//!
//! Created on: Apr 29, 2012
//!     Author: Lazan
//!

#include "xatmi.h"
#include "transform.h"

#include "common/buffer_context.h"
#include "common/calling_context.h"
#include "common/server_context.h"


#include "utility/string.h"
#include "utility/error.h"


//
// Define globals
//
int tperrno = 0;
long tpurcode = 0;


char* tpalloc( const char* type, const char* subtype, long size)
{
	auto&& buffer = casual::common::buffer::Context::instance().allocate(
			type ? type : "",
			subtype ? subtype : "",
			size);

	return casual::xatmi::transform::buffer( buffer);
}

char* tprealloc(char * addr, long size)
{

   auto&& buffer = casual::common::buffer::Context::instance().reallocate(
		addr,
		size);

	return casual::xatmi::transform::buffer( buffer);

}



long tptypes(char* ptr, char* type, char* subtype)
{
	return 0;
}


void tpfree(char* ptr)
{
	casual::common::buffer::Context::instance().deallocate( ptr);
}


void tpreturn(int rval, long rcode, char* data, long len, long flags)
{
   try
   {
      casual::common::server::Context::instance().longJumpReturn( rval, rcode, data, len, flags);
   }
   catch( ...)
   {
      casual::utility::error::handler();
   }
}

int tpcall( const char * svc, char* idata, long ilen, char ** odata, long *olen, long flags)
{
   try
   {

      int callDescriptor = casual::common::calling::Context::instance().asyncCall( svc, idata, ilen, flags);

      return casual::common::calling::Context::instance().getReply( &callDescriptor, odata, *olen, flags);

   }
   catch( ...)
   {
      return casual::utility::error::handler();
   }
   return 0; // remove warning in eclipse
}

int tpacall( const char * svc, char* idata, long ilen, long flags)
{
   try
   {
      return casual::common::calling::Context::instance().asyncCall( svc, idata, ilen, flags);
   }
	catch( ...)
   {
      return casual::utility::error::handler();
   }
   return 0; // remove warning in eclipse
}

int tpgetrply(int *idPtr, char ** odata, long *olen, long flags)
{
   try
   {
      return casual::common::calling::Context::instance().getReply( idPtr, odata, *olen, flags);
   }
   catch( ...)
   {
      return casual::utility::error::handler();
   }
   return 0; // remove warning in eclipse
}

int tpadvertise( const char* svcname, void(*func)(TPSVCINFO *))
{
   try
   {
      casual::common::server::Context::instance().advertiseService( svcname, func);
   }
   catch( ...)
   {
      return casual::utility::error::handler();
   }
   return 0; // remove warning in eclipse
}

int tpunadvertise( const char* svcname)
{
   try
   {
      casual::common::server::Context::instance().unadvertiseService( svcname);
   }
   catch( ...)
   {
      return casual::utility::error::handler();
   }
   return 0; // remove warning in eclipse
}

const char* tperrnostring( int error)
{
   return casual::utility::error::tperrnoStringRepresentation( error).c_str();
}




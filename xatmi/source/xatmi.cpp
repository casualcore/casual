//!
//! xatm.cpp
//!
//! Created on: Apr 29, 2012
//!     Author: Lazan
//!

#include "xatmi.h"

#include "common/buffer_context.h"
#include "common/calling_context.h"
#include "common/server_context.h"
#include "common/types.h"
#include "common/log.h"


#include "common/string.h"
#include "common/error.h"


//
// Define globals
//
extern "C"
{
   int tperrno = 0;
   long tpurcode = 0;
}

char* tpalloc( const char* type, const char* subtype, long size)
{
	auto&& buffer = casual::common::buffer::Context::instance().allocate(
			type ? type : "",
			subtype ? subtype : "",
			size);

	return casual::common::transform::public_buffer( buffer);
}

char* tprealloc(char * addr, long size)
{

   auto&& buffer = casual::common::buffer::Context::instance().reallocate(
		addr,
		size);

	return casual::common::transform::public_buffer( buffer);

}

namespace local
{
   namespace
   {
      template< typename Iter, typename Out>
      void copy_max( Iter start, Iter end, typename std::iterator_traits< Iter>::difference_type max, Out out)
      {
         if( end - start > max)
            end = start + max;

         std::copy( start, end, out);
      }

   }

}


long tptypes( char* ptr, char* type, char* subtype)
{
   try
   {
      const int type_size = { 8};
      const int subtype_size = { 8};

      memset( type, '\0', type_size);
      memset( subtype, '\0', subtype_size);

      auto& buffer = casual::common::buffer::Context::instance().get( ptr);

      local::copy_max( buffer.type().begin(), buffer.type().end(), type_size, type);
      local::copy_max( buffer.subtype().begin(), buffer.subtype().end(), subtype_size, subtype);
   }
   catch( ...)
   {
      tperrno = casual::common::error::handler();
      return -1;
   }

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
      tperrno = casual::common::error::handler();
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
      tperrno = casual::common::error::handler();
      return -1;
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
	   tperrno = casual::common::error::handler();
	   return -1;
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
      tperrno = casual::common::error::handler();
      return -1;
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
      tperrno = casual::common::error::handler();
      return -1;
   }
   return 0;
}

int tpunadvertise( const char* svcname)
{
   try
   {
      casual::common::server::Context::instance().unadvertiseService( svcname);
   }
   catch( ...)
   {
      tperrno = casual::common::error::handler();
      return -1;
   }
   return 0;
}

const char* tperrnostring( int error)
{
   return casual::common::error::tperrnoStringRepresentation( error).c_str();
}


int tpsvrinit(int argc, char **argv)
{
  casual::common::log::debug << "internal tpsvrinit called" << std::endl;
  return 0;
}


void tpsvrdone()
{
  casual::common::log::debug << "internal tpsvrdone called" << std::endl;
}




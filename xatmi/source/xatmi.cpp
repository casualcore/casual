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
#include "common/platform.h"
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
   try
   {
      auto result = casual::common::buffer::Context::instance().allocate( { type, subtype}, size);

      return casual::common::platform::public_buffer( result);

   }
   catch( ...)
   {
      tperrno = casual::common::error::handler();
      return nullptr;
   }
}

char* tprealloc( const char* ptr, long size)
{

   try
   {
      auto&& buffer = casual::common::buffer::Context::instance().reallocate( ptr, size);
      return casual::common::platform::public_buffer( buffer);
   }
   catch( ...)
   {
      tperrno = casual::common::error::handler();
      return nullptr;
   }

}

namespace local
{
   namespace
   {
      template< typename Iter, typename Out>
      void copy_max( const Iter start, Iter end, typename std::iterator_traits< Iter>::difference_type max, Out out)
      {
         if( end - start > max)
            end = start + max;

         std::copy( start, end, out);
      }

   }

}

long tptypes( const char* const ptr, char* const type, char* const subtype)
{
   try
   {
      const auto& buffer = casual::common::buffer::Context::instance().get( ptr);

      //
      // type is optional
      //
      if( type)
      {
         const int type_size = { 8 };
         memset( type, '\0', type_size);
         local::copy_max( buffer.type().type.begin(), buffer.type().type.end(), type_size, type);
      }

      //
      // subtype is optional
      //
      if( subtype)
      {
         const int subtype_size = { 16 };
         memset( subtype, '\0', subtype_size);
         local::copy_max( buffer.type().subtype.begin(), buffer.type().subtype.end(), subtype_size, subtype);
      }

      return buffer.size();

   }
   catch( ...)
   {
      tperrno = casual::common::error::handler();
      return -1;
   }

}

void tpfree( const char* const ptr)
{
   casual::common::buffer::Context::instance().deallocate( ptr);
}

void tpreturn( const int rval, const long rcode, char* const data, const long len, const long flags)
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

int tpcall( const char* const svc, char* idata, const long ilen, char** odata, long* olen, const long flags)
{
   try
   {
      //
      // TODO: if needed size is less than current size, shall vi reduce it ?
      //


      int callDescriptor = casual::common::calling::Context::instance().asyncCall( svc, idata, ilen, flags);

      return casual::common::calling::Context::instance().getReply( &callDescriptor, odata, *olen, flags);
   }
   catch( ...)
   {
      tperrno = casual::common::error::handler();
      return -1;
   }
}

int tpacall( const char* const svc, char* idata, const long ilen, const long flags)
{
   try
   {
      //
      // TODO: if needed size is less than current size, shall vi reduce it ?
      //

      return casual::common::calling::Context::instance().asyncCall( svc, idata, ilen, flags);
   }
   catch( ...)
   {
      tperrno = casual::common::error::handler();
      return -1;
   }
}

int tpgetrply( int *const idPtr, char ** odata, long *olen, const long flags)
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
}

int tpcancel( int id)
{
   try
   {
      return casual::common::calling::Context::instance().canccel( id);
   }
   catch( ...)
   {
      tperrno = casual::common::error::handler();
      return -1;
   }
}


int tpadvertise( const char* const svcname, void (*func)( TPSVCINFO *))
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

int tpunadvertise( const char* const svcname)
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

int tpsvrinit( int argc, char **argv)
{
  casual::common::log::debug << "internal tpsvrinit called" << std::endl;
  return 0;
}

void tpsvrdone()
{
  casual::common::log::debug << "internal tpsvrdone called" << std::endl;
}


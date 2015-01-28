//!
//! xatm.cpp
//!
//! Created on: Apr 29, 2012
//!     Author: Lazan
//!

#include "xatmi.h"

#include "common/buffer/pool.h"
#include "common/call/context.h"
#include "common/server/context.h"
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
      //
      // TODO: Shall we report size less than zero ?
      //
      return casual::common::buffer::pool::Holder::instance().allocate( { type, subtype}, size < 0 ? 0 : size);
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
      //
      // TODO: Shall we report size less than zero ?
      //
      return casual::common::buffer::pool::Holder::instance().reallocate( ptr, size < 0 ? 0 : size);
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
      auto& buffer = casual::common::buffer::pool::Holder::instance().get( ptr);

      //
      // type is optional
      //
      if( type)
      {
         const std::size_t size{ 8 };
         memset( type, '\0', size);
         casual::common::range::copy_max( buffer.type.type, size, type);
      }

      //
      // subtype is optional
      //
      if( subtype)
      {
         const std::size_t size{ 16 };
         memset( subtype, '\0', size);
         casual::common::range::copy_max( buffer.type.subtype, size, subtype);
      }

      return buffer.memory.size();

   }
   catch( ...)
   {
      tperrno = casual::common::error::handler();
      return -1;
   }

}

void tpfree( const char* const ptr)
{
   casual::common::buffer::pool::Holder::instance().deallocate( ptr);
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
   if( svc == nullptr)
   {
      tperrno = TPEINVAL;
      return -1;
   }

   try
   {
      casual::common::call::Context::instance().sync( svc, idata, ilen, *odata, *olen, flags);
   }
   catch( ...)
   {
      tperrno = casual::common::error::handler();
      return -1;
   }
   return 0;
}

int tpacall( const char* const svc, char* idata, const long ilen, const long flags)
{
   if( svc == nullptr)
   {
      tperrno = TPEINVAL;
      return -1;
   }

   try
   {
      //
      // TODO: if needed size is less than current size, shall vi reduce it ?
      //

      return casual::common::call::Context::instance().async( svc, idata, ilen, flags);
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
      casual::common::call::Context::instance().reply( *idPtr, odata, *olen, flags);
   }
   catch( ...)
   {
      tperrno = casual::common::error::handler();
      return -1;
   }
   return 0;
}

int tpcancel( int id)
{
   try
   {
      casual::common::call::Context::instance().cancel( id);
   }
   catch( ...)
   {
      tperrno = casual::common::error::handler();
      return -1;
   }
   return 0;
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
   return casual::common::error::xatmi::error( error).c_str();
}

int tpsvrinit( int argc, char **argv)
{
  return 0;
}

void tpsvrdone()
{
}


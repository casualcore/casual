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
#include "common/memory.h"

#include "common/string.h"
#include "common/error.h"


#include <array>
#include <stdarg.h>


//
// Define globals
//

namespace local
{
   namespace
   {
      int tperrno_value = 0;
   } // <unnamed>
} // local

int casual_get_tperrno(void)
{
   return local::tperrno_value;
}

void casual_set_tperrno( int value)
{
   local::tperrno_value = value;
}



long casual_get_tpurcode(void)
{
   return casual::common::call::Context::instance().user_code();
}

void casual_set_tpurcode( long value)
{
   casual::common::call::Context::instance().user_code( value);
}





char* tpalloc( const char* type, const char* subtype, long size)
{
   casual_set_tperrno( 0);

   try
   {
      //
      // TODO: Shall we report size less than zero ?
      //
      return casual::common::buffer::pool::Holder::instance().allocate( { type, subtype}, size < 0 ? 0 : size);
   }
   catch( ...)
   {
      casual_set_tperrno( casual::common::error::handler());
      return nullptr;
   }
}

char* tprealloc( const char* ptr, long size)
{
   casual_set_tperrno( 0);

   try
   {
      //
      // TODO: Shall we report size less than zero ?
      //
      return casual::common::buffer::pool::Holder::instance().reallocate( ptr, size < 0 ? 0 : size);
   }
   catch( ...)
   {
      casual_set_tperrno( casual::common::error::handler());
      return nullptr;
   }

}


long tptypes( const char* const ptr, char* const type, char* const subtype)
{
   casual_set_tperrno( 0);

   try
   {
      auto buffer = casual::common::buffer::pool::Holder::instance().get( ptr);

      //
      // type is optional
      //
      if( type)
      {
         auto destination = casual::common::range::make( type, 8);
         casual::common::memory::set( destination, '\0');
         casual::common::memory::copy( casual::common::range::make( buffer.payload.type.name), destination);
      }

      //
      // subtype is optional
      //
      if( subtype)
      {
         auto destination = casual::common::range::make( subtype, 16);
         casual::common::memory::set( destination, '\0');
         casual::common::memory::copy( casual::common::range::make( buffer.payload.type.subname), destination);
      }

      return buffer.reserved;

   }
   catch( ...)
   {
      casual_set_tperrno( casual::common::error::handler());
      return -1;
   }

}

void tpfree( const char* const ptr)
{
   casual::common::buffer::pool::Holder::instance().deallocate( ptr);
}

void tpreturn( const int rval, const long rcode, char* const data, const long len, const long flags)
{
   casual_set_tperrno( 0);


   try
   {

      casual::common::server::Context::instance().long_jump_return( rval, rcode, data, len, flags);
   }
   catch( ...)
   {
      casual_set_tperrno( casual::common::error::handler());
   }
}

int tpcall( const char* const svc, char* idata, const long ilen, char** odata, long* olen, const long flags)
{
   casual_set_tperrno( 0);
   casual_set_tpurcode( 0);

   if( svc == nullptr)
   {
      casual_set_tperrno( TPEINVAL);
      return -1;
   }

   try
   {
      casual::common::call::Context::instance().sync( svc, idata, ilen, *odata, *olen, flags);
   }
   catch( ...)
   {
      casual_set_tperrno( casual::common::error::handler());
      return -1;
   }
   return 0;
}

int tpacall( const char* const svc, char* idata, const long ilen, const long flags)
{
   casual_set_tperrno( 0);
   casual_set_tpurcode( 0);

   if( svc == nullptr)
   {
      casual_set_tperrno( TPEINVAL);
      return -1;
   }

   try
   {
      return casual::common::call::Context::instance().async( svc, idata, ilen, flags);
   }
   catch( ...)
   {
      casual_set_tperrno( casual::common::error::handler());
      return -1;
   }
}

int tpgetrply( int *const idPtr, char ** odata, long *olen, const long flags)
{
   casual_set_tperrno( 0);

   try
   {
      casual::common::call::Context::instance().reply( *idPtr, odata, *olen, flags);
   }
   catch( ...)
   {
      casual_set_tperrno( casual::common::error::handler());
      return -1;
   }
   return 0;
}

int tpcancel( int id)
{
   casual_set_tperrno( 0);

   try
   {
      casual::common::call::Context::instance().cancel( id);
   }
   catch( ...)
   {
      casual_set_tperrno( casual::common::error::handler());
      return -1;
   }
   return 0;
}


int tpadvertise( const char* const svcname, void (*func)( TPSVCINFO *))
{
   casual_set_tperrno( 0);

   try
   {
      casual::common::server::Context::instance().advertise( svcname, func);
   }
   catch( ...)
   {
      casual_set_tperrno( casual::common::error::handler());
      return -1;
   }
   return 0;
}

int tpunadvertise( const char* const svcname)
{
   casual_set_tperrno( 0);

   try
   {
      casual::common::server::Context::instance().unadvertise( svcname);
   }
   catch( ...)
   {
      casual_set_tperrno( casual::common::error::handler());
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
   casual_set_tperrno( 0);
   return tx_open() == TX_OK ? 0 : -1;
}

void tpsvrdone()
{
   casual_set_tperrno( 0);
   tx_close();
}


void casual_service_forward( const char* service, char* data, long size)
{
   casual::common::server::Context::instance().forward( service, data, size);
}

namespace local
{
   namespace
   {
      template< typename L>
      int vlog( L&& logger, const char* const format, va_list arglist)
      {
         std::array< char, 2048> buffer;
         std::vector< char> backup;

         va_list argcopy;
         va_copy( argcopy, arglist);
         auto written = vsnprintf( buffer.data(), buffer.max_size(), format, argcopy);
         va_end( argcopy );

         auto data = buffer.data();

         if( written >= static_cast< decltype( written)>( buffer.max_size()))
         {
            backup.resize( written + 1);
            va_copy( argcopy, arglist);
            written = vsnprintf( backup.data(), backup.size(), format, argcopy);
            va_end( argcopy );
            data = backup.data();
         }

         logger( data);

         return written;
      }

   } // <unnamed>
} // local

int casual_vlog( casual_log_category_t category, const char* const format, va_list arglist)
{

   auto catagory_logger = [=]( const char* data){

      switch( category)
      {
      case casual_log_category_t::c_log_debug:
         casual::common::log::write( casual::common::log::category::Type::debug, data);
         break;
      case casual_log_category_t::c_log_information:
         casual::common::log::write( casual::common::log::category::Type::information, data);
         break;
      case casual_log_category_t::c_log_warning:
         casual::common::log::write( casual::common::log::category::Type::warning, data);
         break;
      default:
         casual::common::log::write( casual::common::log::category::Type::error, data);
         break;
      }
   };

   return local::vlog( catagory_logger, format, arglist);
}

int casual_user_vlog( const char* category, const char* const format, va_list arglist)
{
   auto user_logger = [=]( const char* data){
      casual::common::log::write( category, data);
   };

   return local::vlog( user_logger, format, arglist);
}

int casual_user_log( const char* category, const char* const message)
{
   casual::common::log::write( category, message);

   return 0;
}

int casual_log( casual_log_category_t category, const char* const format, ...)
{
   va_list arglist;
   va_start( arglist, format );
   auto result = casual_vlog( category, format, arglist);
   va_end( arglist );

   return result;
}









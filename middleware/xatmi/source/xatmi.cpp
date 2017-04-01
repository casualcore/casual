//!
//! casual
//!

#include "xatmi.h"

#include "common/buffer/pool.h"
#include "common/server/context.h"
#include "common/platform.h"
#include "common/log.h"
#include "common/memory.h"

#include "common/string.h"
#include "common/error.h"


#include <array>
#include <stdarg.h>


#include "xatmi/internal.h"



int casual_get_tperrno()
{
   return casual::xatmi::internal::error::get();
}

long casual_get_tpurcode()
{
   return casual::xatmi::internal::user::code::get();
}



char* tpalloc( const char* type, const char* subtype, long size)
{
   casual::xatmi::internal::error::set( 0);

   try
   {
      //
      // TODO: Shall we report size less than zero ?
      //
      return casual::common::buffer::pool::Holder::instance().allocate( type, subtype, size < 0 ? 0 : size);
   }
   catch( ...)
   {
      casual::xatmi::internal::error::set( casual::common::error::handler());
      return nullptr;
   }
}

char* tprealloc( const char* ptr, long size)
{
   casual::xatmi::internal::error::set( 0);

   try
   {
      //
      // TODO: Shall we report size less than zero ?
      //
      return casual::common::buffer::pool::Holder::instance().reallocate( ptr, size < 0 ? 0 : size);
   }
   catch( ...)
   {
      casual::xatmi::internal::error::set( casual::common::error::handler());
      return nullptr;
   }

}


long tptypes( const char* const ptr, char* const type, char* const subtype)
{
   casual::xatmi::internal::error::set( 0);

   try
   {
      auto buffer = casual::common::buffer::pool::Holder::instance().get( ptr);

      auto combined = casual::common::buffer::type::dismantle( buffer.payload().type);

      //
      // type is optional
      //
      if( type)
      {
         auto destination = casual::common::range::make( type, 8);
         casual::common::memory::set( destination, '\0');
         casual::common::memory::copy( std::get< 0>( combined), destination);
      }

      //
      // subtype is optional
      //
      if( subtype)
      {
         auto destination = casual::common::range::make( subtype, 16);
         casual::common::memory::set( destination, '\0');
         casual::common::memory::copy( std::get< 1>( combined), destination);
      }

      return buffer.reserved;

   }
   catch( ...)
   {
      casual::xatmi::internal::error::set( casual::common::error::handler());
      return -1;
   }

}

void tpfree( const char* const ptr)
{
   casual::common::buffer::pool::Holder::instance().deallocate( ptr);
}

void tpreturn( const int rval, const long rcode, char* const data, const long len, const long flags)
{
   casual::xatmi::internal::error::wrap( [&](){
      casual::common::server::Context::instance().jump_return( rval, rcode, data, len, flags);
   });
}






int tpadvertise( const char* service, void (*function)( TPSVCINFO *))
{
   return casual::xatmi::internal::error::wrap( [&](){
      casual::common::server::Context::instance().advertise( service, function);
   });
}

int tpunadvertise( const char* const service)
{
   return casual::xatmi::internal::error::wrap( [&](){
      casual::common::server::Context::instance().unadvertise( service);
   });
}





const char* tperrnostring( int error)
{

   return casual::common::error::xatmi::error( error).c_str();
}

int tpsvrinit( int argc, char **argv)
{
   casual::xatmi::internal::error::set( 0);
   return tx_open() == TX_OK ? 0 : -1;
}

void tpsvrdone()
{
   casual::xatmi::internal::error::set( 0);
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
         casual::common::log::stream::write( "debug", data);
         break;
      case casual_log_category_t::c_log_information:
         casual::common::log::stream::write( "information", data);
         break;
      case casual_log_category_t::c_log_warning:
         casual::common::log::stream::write( "warning", data);
         break;
      default:
         casual::common::log::stream::write( "error", data);
         break;
      }
   };

   return local::vlog( catagory_logger, format, arglist);
}

int casual_user_vlog( const char* category, const char* const format, va_list arglist)
{
   auto user_logger = [=]( const char* data){
      casual::common::log::stream::write( category, data);
   };

   return local::vlog( user_logger, format, arglist);
}

int casual_user_log( const char* category, const char* const message)
{
   casual::common::log::stream::write( category, message);

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









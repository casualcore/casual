//!
//! casual
//!

#include "xatmi.h"

#include "common/buffer/pool.h"
#include "common/service/call/context.h"
#include "common/service/conversation/context.h"
#include "common/server/context.h"
#include "common/platform.h"
#include "common/log.h"
#include "common/memory.h"

#include "common/string.h"
#include "common/error.h"


#include <array>
#include <stdarg.h>




namespace local
{
   namespace
   {
      namespace error
      {
         int value = 0;

         void set( int value)
         {
            error::value = value;
         }

         template< typename T>
         int wrap( T&& task)
         {
            try
            {
               error::set( 0);
               task();
            }
            catch( ...)
            {
               error::set( casual::common::error::handler());
            }
            return error::value == 0 ? 0 : -1;
         }

      } // tperrno

      namespace user
      {
         namespace code
         {
            void set( long value)
            {
               casual::common::service::call::Context::instance().user_code( value);
            }
         } // code
      } // user




   } // <unnamed>
} // local

int casual_get_tperrno()
{
   return local::error::value;
}

long casual_get_tpurcode()
{
   return casual::common::service::call::Context::instance().user_code();
}



char* tpalloc( const char* type, const char* subtype, long size)
{
   local::error::set( 0);

   try
   {
      //
      // TODO: Shall we report size less than zero ?
      //
      return casual::common::buffer::pool::Holder::instance().allocate( type, subtype, size < 0 ? 0 : size);
   }
   catch( ...)
   {
      local::error::set( casual::common::error::handler());
      return nullptr;
   }
}

char* tprealloc( const char* ptr, long size)
{
   local::error::set( 0);

   try
   {
      //
      // TODO: Shall we report size less than zero ?
      //
      return casual::common::buffer::pool::Holder::instance().reallocate( ptr, size < 0 ? 0 : size);
   }
   catch( ...)
   {
      local::error::set( casual::common::error::handler());
      return nullptr;
   }

}


long tptypes( const char* const ptr, char* const type, char* const subtype)
{
   local::error::set( 0);

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
      local::error::set( casual::common::error::handler());
      return -1;
   }

}

void tpfree( const char* const ptr)
{
   casual::common::buffer::pool::Holder::instance().deallocate( ptr);
}

void tpreturn( const int rval, const long rcode, char* const data, const long len, const long flags)
{
   local::error::wrap( [&](){
      casual::common::server::Context::instance().jump_return( rval, rcode, data, len, flags);
   });
}

namespace local
{
   namespace
   {
      template< typename R, typename Flags>
      void handle_reply_buffer( R&& result, Flags flags, char** odata, long* olen)
      {
         auto output = casual::common::buffer::pool::Holder::instance().get( *odata);

         using enum_type = typename Flags::enum_type;

         if( flags.exist( enum_type::no_change) && result.buffer.type != output.payload().type)
         {
            throw casual::common::exception::xatmi::buffer::type::Output{};
         }

         casual::common::buffer::pool::Holder::instance().deallocate( *odata);
         std::tie( *odata, *olen) = casual::common::buffer::pool::Holder::instance().insert( std::move( result.buffer));

         local::error::set( casual::common::cast::underlying( result.state));
      }

   } // <unnamed>
} // local

int tpcall( const char* const service, char* idata, const long ilen, char** odata, long* olen, const long bitmap)
{
   local::error::set( 0);

   if( service == nullptr)
   {
      local::error::set( TPEINVAL);
      return -1;
   }

   try
   {
      local::user::code::set( 0);

      using Flag = casual::common::service::call::sync::Flag;

      constexpr casual::common::service::call::sync::Flags valid_flags{
         Flag::no_transaction,
         Flag::no_change,
         Flag::no_block,
         Flag::no_time,
         Flag::signal_restart};

      auto buffer = casual::common::buffer::pool::Holder::instance().get( idata, ilen);

      auto flags = valid_flags.convert( bitmap);

      auto result = casual::common::service::call::Context::instance().sync(
            service,
            buffer,
            flags);

      auto output = casual::common::buffer::pool::Holder::instance().get( *odata);

      if( flags.exist( Flag::no_change) && result.buffer.type != output.payload().type)
      {
         throw casual::common::exception::xatmi::buffer::type::Output{};
      }

      casual::common::buffer::pool::Holder::instance().deallocate( *odata);
      std::tie( *odata, *olen) = casual::common::buffer::pool::Holder::instance().insert( std::move( result.buffer));

      local::error::set( casual::common::cast::underlying( result.state));
   }
   catch( ...)
   {
      local::error::set( casual::common::error::handler());
   }
   return local::error::value == 0 ? 0 : -1;
}

int tpacall( const char* const service, char* idata, const long ilen, const long flags)
{
   local::error::set( 0);

   if( service == nullptr)
   {
      local::error::set( TPEINVAL);
      return -1;
   }

   try
   {
      auto buffer = casual::common::buffer::pool::Holder::instance().get( idata, ilen);

      using Flag = casual::common::service::call::async::Flag;

      constexpr casual::common::service::call::async::Flags valid_flags{
         Flag::no_transaction,
         Flag::no_reply,
         Flag::no_block,
         Flag::no_time,
         Flag::signal_restart};

      return casual::common::service::call::Context::instance().async(
            service,
            buffer,
            valid_flags.convert( flags));
   }
   catch( ...)
   {
      local::error::set( casual::common::error::handler());
   }
   return local::error::value == 0 ? 0 : -1;
}

int tpgetrply( int *const descriptor, char** odata, long* olen, const long bitmap)
{
   return local::error::wrap( [&](){

      using Flag = casual::common::service::call::reply::Flag;

      constexpr casual::common::service::call::reply::Flags valid_flags{
         Flag::any,
         Flag::no_change,
         Flag::no_block,
         Flag::no_time,
         Flag::signal_restart};

      auto flags = valid_flags.convert( bitmap);

      auto result = casual::common::service::call::Context::instance().reply( *descriptor, flags);

      *descriptor = result.descriptor;

      local::handle_reply_buffer( result, flags, odata, olen);

   });
}

int tpcancel( int id)
{
   return local::error::wrap( [id](){
      casual::common::service::call::Context::instance().cancel( id);
   });
}


int tpadvertise( const char* service, void (*function)( TPSVCINFO *))
{
   return local::error::wrap( [&](){
      casual::common::server::Context::instance().advertise( service, function);
   });
}

int tpunadvertise( const char* const service)
{
   return local::error::wrap( [&](){
      casual::common::server::Context::instance().unadvertise( service);
   });
}



int tpconnect( const char* svc, const char* idata, long ilen, long flags)
{
   if( svc == nullptr)
   {
      local::error::set( TPEINVAL);
      return -1;
   }

   return local::error::wrap( [&](){

      auto buffer = casual::common::buffer::pool::Holder::instance().get( idata, ilen);

      using Flag = casual::common::service::conversation::connect::Flag;

      constexpr casual::common::service::conversation::connect::Flags valid_flags{
         Flag::no_block,
         Flag::no_time,
         Flag::no_transaction,
         Flag::receive_only,
         Flag::send_only,
         Flag::signal_restart};

      casual::common::service::conversation::Context::instance().connect(
            svc,
            buffer,
            valid_flags.convert( flags));

   });
}

namespace local
{
   namespace
   {
      namespace conversation
      {
         template< typename T>
         int wrap( long& event, T&& task)
         {
            try
            {
               error::set( 0);
               auto result = task();

               if( result)
               {
                  event = result.underlaying();
                  local::error::set( TPEEVENT);
                  return -1;
               }
            }
            catch( ...)
            {
               error::set( casual::common::error::handler());
               return -1;
            }
            return 0;
         }

      } // error
   } // <unnamed>
} // local

int tpsend( int id, const char* idata, long ilen, long flags, long* event)
{
   return local::conversation::wrap( *event, [&](){

      auto buffer = casual::common::buffer::pool::Holder::instance().get( idata, ilen);

      using Flag = casual::common::service::conversation::send::Flag;

      constexpr casual::common::service::conversation::send::Flags valid_flags{
         Flag::receive_only,
         Flag::no_block,
         Flag::no_time,
         Flag::signal_restart};

      return casual::common::service::conversation::Context::instance().send(
            id,
            std::move( buffer),
            valid_flags.convert( flags));

   });
}

int tprecv( int id, char ** odata, long *olen, long bitmask, long* event)
{
   return local::conversation::wrap( *event, [&](){

      auto buffer = casual::common::buffer::pool::Holder::instance().get( *odata);

      using Flag = casual::common::service::conversation::receive::Flag;

      constexpr casual::common::service::conversation::receive::Flags valid_flags{
         Flag::no_change,
         Flag::no_block,
         Flag::no_time,
         Flag::signal_restart
      };

      auto flag = valid_flags.convert( bitmask);

      auto result = casual::common::service::conversation::Context::instance().receive(
            id,
            flag);

      if( ( flag & Flag::no_change) && buffer.payload().type != result.buffer.type)
      {
         throw casual::common::exception::xatmi::buffer::type::Output{};
      }

      casual::common::buffer::pool::Holder::instance().deallocate( *odata);
      std::tie( *odata, *olen) = casual::common::buffer::pool::Holder::instance().insert( std::move( result.buffer));

      return result.event;

   });
}

int tpdiscon( int id)
{
   return local::error::wrap( [&](){

   });
}



const char* tperrnostring( int error)
{

   return casual::common::error::xatmi::error( error).c_str();
}

int tpsvrinit( int argc, char **argv)
{
   local::error::set( 0);
   return tx_open() == TX_OK ? 0 : -1;
}

void tpsvrdone()
{
   local::error::set( 0);
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









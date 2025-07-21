//! 
//! Copyright (c) 2015, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!


#include "casual/xatmi/explicit.h"
#include "casual/xatmi/internal/header/context.h"
#include "casual/tx.h"

#include "casual/xatmi/internal/code.h"
#include "casual/xatmi/internal/signal.h"
#include "casual/xatmi/internal/server/service.h"

#include "common/buffer/pool.h"
#include "casual/platform.h"
#include "common/log.h"
#include "common/memory.h"
#include "common/exception/capture.h"
#include "common/string.h"

#include "service/call/context.h"
#include "service/call.h"

#include "server/context.h"


#include <array>
#include <cstdarg>


namespace local
{
   namespace
   {
      template< typename R, typename Flag>
      void handle_reply_buffer( R&& result, Flag flags, char** odata, long* olen)
      {
         auto output = casual::common::buffer::pool::holder().get( casual::common::buffer::handle::type{ *odata});

         if( casual::common::flag::contains( flags, Flag::no_change) && result.buffer.type != output.payload().type)
            casual::common::code::raise::error( casual::common::code::xatmi::buffer_output);

         casual::common::buffer::pool::holder().deallocate( casual::common::buffer::handle::type{ *odata});
         auto buffer = casual::common::buffer::pool::holder().insert( std::move( result.buffer));
         *odata = std::get< 0>( buffer).raw();
         *olen = std::get< 1>( buffer);
      }
   } // <unnamed>
} // local


int casual_service_call( const char* const service, char* idata, const long ilen, char** odata, long* olen, const long bitmap)
{
   casual::xatmi::internal::clear();

   if( service == nullptr)
   {
      casual::xatmi::internal::error::set( casual::common::code::xatmi::argument);
      return -1;
   }

   try
   {
      using Flag = casual::service::call::sync::Flag;

      auto flags = Flag{ bitmap};

      constexpr auto valid_flags = Flag::no_transaction
         | Flag::no_change
         | Flag::no_block 
         | Flag::no_time
         | Flag::signal_restart;

      if( ! casual::common::flag::valid( valid_flags, flags))
         casual::common::code::raise::error( casual::common::code::xatmi::argument, "flags: ", flags, " outside of: ", valid_flags);

      auto handle = casual::common::buffer::handle::type{ idata};

      auto buffer = casual::common::buffer::pool::holder().get( handle, ilen);

      auto maybe_block = casual::xatmi::internal::signal::maybe_block( flags);

      auto get_complement = [ &](){
         if( auto header = casual::xatmi::internal::header::context().find( handle))
            return casual::service::call::Complement{ .flags = flags, .header = *header};
         else
            return casual::service::call::Complement{ .flags = flags};
      };

      auto result = casual::service::call::invoke( service, buffer.payload(), get_complement());

      casual::xatmi::internal::user::code::set( result.user);
      local::handle_reply_buffer( result, flags, odata, olen);

      return 0;
   }
   catch( casual::service::call::Fail& fail)
   {
      casual::xatmi::internal::error::set( casual::common::code::xatmi::service_fail);
      casual::xatmi::internal::user::code::set( fail.result.user);

      casual::common::buffer::pool::holder().deallocate( casual::common::buffer::handle::type{ *odata});
      auto result = casual::common::buffer::pool::holder().insert( std::move( fail.result.buffer));
      *odata = std::get< 0>( result).raw();
      *olen = std::get< 1>( result);
   }
   catch( ...)
   {
      casual::xatmi::internal::error::set( casual::xatmi::internal::exception::code());
   }
   return -1;
}

int casual_service_asynchronous_send( const char* const service, char* idata, const long ilen, const long bitmap)
{
   casual::xatmi::internal::clear();

   if( service == nullptr)
   {
      casual::xatmi::internal::error::set( casual::common::code::xatmi::argument);
      return -1;
   }

   try
   {
      using Flag = casual::service::call::async::Flag;

      auto flags = Flag{ bitmap};

      constexpr auto valid_flags = Flag::no_transaction
         | Flag::no_reply
         | Flag::no_block
         | Flag::no_time
         | Flag::signal_restart;

      if( ! casual::common::flag::valid( valid_flags, flags))
         casual::common::code::raise::error( casual::common::code::xatmi::argument, "flags: ", flags, " outside of: ", valid_flags);

      auto handle = casual::common::buffer::handle::type{ idata};

      auto buffer = casual::common::buffer::pool::holder().get( handle, ilen);

      auto maybe_block = casual::xatmi::internal::signal::maybe_block( flags);

      auto get_complement = [ &](){
         if( auto header = casual::xatmi::internal::header::context().find( handle))
            return casual::service::send::Complement{ .flags = flags, .header = *header};
         else
            return casual::service::send::Complement{ .flags = flags};
      };

      return casual::service::send::invoke(
            service,
            buffer.payload(),
            get_complement());
   }
   catch( ...)
   {
      casual::xatmi::internal::error::set( casual::xatmi::internal::exception::code());
   }
   return -1;
}

int casual_service_asynchronous_receive( int *const descriptor, char** odata, long* olen, const long bitmap)
{
   casual::xatmi::internal::clear();

   try 
   {
      using Flag = casual::service::call::reply::Flag;

      auto flags = Flag{ bitmap};
      
      constexpr auto valid_flags = Flag::any
         | Flag::no_change
         | Flag::no_block
         | Flag::no_time
         | Flag::signal_restart;

      if( ! casual::common::flag::valid( valid_flags, flags))
         casual::common::code::raise::error( casual::common::code::xatmi::argument, "flags: ", flags, " outside of: ", valid_flags);

      auto maybe_block = casual::xatmi::internal::signal::maybe_block( flags);

      auto result = casual::service::receive::invoke( *descriptor, flags);

      *descriptor = result.descriptor;
      casual::xatmi::internal::user::code::set( result.user);

      local::handle_reply_buffer( result, flags, odata, olen);

      return 0;
   }
   catch( casual::service::call::Fail& fail)
   {
      casual::xatmi::internal::error::set( casual::common::code::xatmi::service_fail);
      casual::xatmi::internal::user::code::set( fail.result.user);

      *descriptor = fail.result.descriptor;
      casual::common::buffer::pool::holder().deallocate( casual::common::buffer::handle::type{ *odata});
      auto result = casual::common::buffer::pool::holder().insert( std::move( fail.result.buffer));
      *odata = std::get< 0>( result).raw();
      *olen = std::get< 1>( result);  
   }
   catch( ...)
   {
      auto error = casual::common::exception::capture();
      
      // we "need" to treat no_entry as service_error to conform to xatmi-spec.
      if( error.code() == casual::common::code::xatmi::no_entry)
         casual::xatmi::internal::error::set( casual::common::code::xatmi::service_error);
      else 
         casual::xatmi::internal::error::set( casual::xatmi::internal::exception::code( error.code()));
   }
   return -1;
}

int casual_service_asynchronous_cancel( int id)
{
   return casual::xatmi::internal::error::wrap( [id](){
      casual::service::call::context().cancel( id);
   });
}

void casual_service_return( const int rval, const long rcode, char* const data, const long len, const long /* flags for future use */)
{
   casual::xatmi::internal::error::wrap( [&](){
      casual::server::context().jump_return( 
         static_cast< casual::common::flag::xatmi::Return>( rval), rcode, data, len);
   });
}


int casual_service_advertise( const char* service, void (*function)( TPSVCINFO *))
{
   return casual::xatmi::internal::error::wrap( [&]()
   {
      auto name = std::string{ service};
      if( name.size() >= XATMI_SERVICE_NAME_LENGTH)
      {
         name.resize( XATMI_SERVICE_NAME_LENGTH - 1);
         casual::common::log::error( casual::common::code::xatmi::argument, "service name '", service, "' truncated to '", name, "'");
      }

      casual::server::context().advertise( 
         casual::xatmi::internal::server::service::create( 
            std::move( name), 
            function));
   });
}

int casual_service_unadvertise( const char* const service)
{
   return casual::xatmi::internal::error::wrap( [&](){
      casual::server::context().unadvertise( service);
   });
}



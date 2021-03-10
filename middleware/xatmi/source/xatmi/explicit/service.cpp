//! 
//! Copyright (c) 2015, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!


#include "casual/xatmi/explicit.h"
#include "casual/tx.h"

#include "casual/xatmi/internal/code.h"

#include "common/buffer/pool.h"
#include "common/server/context.h"
#include "common/service/call/context.h"
#include "casual/platform.h"
#include "common/log.h"
#include "common/memory.h"
#include "common/exception/handle.h"

#include "common/string.h"

#include <array>
#include <cstdarg>


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
            casual::common::code::raise::log( casual::common::code::xatmi::buffer_output);

         casual::common::buffer::pool::Holder::instance().deallocate( *odata);
         std::tie( *odata, *olen) = casual::common::buffer::pool::Holder::instance().insert( std::move( result.buffer));
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

      casual::xatmi::internal::user::code::set( result.user);
      local::handle_reply_buffer( result, flags, odata, olen);

      return 0;
   }
   catch( casual::common::service::call::Fail& fail)
   {
      casual::xatmi::internal::error::set( casual::common::code::xatmi::service_fail);
      casual::xatmi::internal::user::code::set( fail.result.user);

      casual::common::buffer::pool::Holder::instance().deallocate( *odata);
      std::tie( *odata, *olen) = casual::common::buffer::pool::Holder::instance().insert( std::move( fail.result.buffer));      
   }
   catch( ...)
   {
      casual::xatmi::internal::error::set( casual::xatmi::internal::exception::code());
   }
   return -1;
}

int casual_service_asynchronous_send( const char* const service, char* idata, const long ilen, const long flags)
{
   casual::xatmi::internal::clear();

   if( service == nullptr)
   {
      casual::xatmi::internal::error::set( casual::common::code::xatmi::argument);
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
      casual::xatmi::internal::error::set( casual::xatmi::internal::exception::code());
   }
   return -1;
}

int casual_service_asynchronous_receive( int *const descriptor, char** odata, long* olen, const long bitmap)
{
   casual::xatmi::internal::clear();

   try 
   {
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
      casual::xatmi::internal::user::code::set( result.user);

      local::handle_reply_buffer( result, flags, odata, olen);

      return 0;
   }
   catch( casual::common::service::call::Fail& fail)
   {
      casual::xatmi::internal::error::set( casual::common::code::xatmi::service_fail);
      casual::xatmi::internal::user::code::set( fail.result.user);

      *descriptor = fail.result.descriptor;
      casual::common::buffer::pool::Holder::instance().deallocate( *odata);
      std::tie( *odata, *olen) = casual::common::buffer::pool::Holder::instance().insert( std::move( fail.result.buffer));      
   }
   catch( ...)
   {
      auto error = casual::common::exception::error();
      
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
      casual::common::service::call::Context::instance().cancel( id);
   });
}

void casual_service_return( const int rval, const long rcode, char* const data, const long len, const long /* flags for future use */)
{
   casual::xatmi::internal::error::wrap( [&](){
      casual::common::server::context().jump_return( 
         static_cast< casual::common::flag::xatmi::Return>( rval), rcode, data, len);
   });
}

int casual_service_advertise( const char* service, void (*function)( TPSVCINFO *))
{
   return casual::xatmi::internal::error::wrap( [&](){
      casual::common::server::context().advertise( service, function);
   });
}

int casual_service_unadvertise( const char* const service)
{
   return casual::xatmi::internal::error::wrap( [&](){
      casual::common::server::context().unadvertise( service);
   });
}
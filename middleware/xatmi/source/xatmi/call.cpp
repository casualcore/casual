//!
//! casual 
//!

#include "xatmi.h"
#include "xatmi/internal.h"

#include "common/buffer/pool.h"
#include "common/service/call/context.h"


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

         casual::xatmi::internal::error::set( casual::common::cast::underlying( result.state));
      }

   } // <unnamed>
} // local

int tpcall( const char* const service, char* idata, const long ilen, char** odata, long* olen, const long bitmap)
{
   casual::xatmi::internal::error::set( 0);

   if( service == nullptr)
   {
      casual::xatmi::internal::error::set( TPEINVAL);
      return -1;
   }

   try
   {
      casual::xatmi::internal::user::code::set( 0);

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

      casual::xatmi::internal::error::set( casual::common::cast::underlying( result.state));
   }
   catch( ...)
   {
      casual::xatmi::internal::error::set( casual::common::error::handler());
   }
   return casual::xatmi::internal::error::get() == 0 ? 0 : -1;
}

int tpacall( const char* const service, char* idata, const long ilen, const long flags)
{
   casual::xatmi::internal::error::set( 0);

   if( service == nullptr)
   {
      casual::xatmi::internal::error::set( TPEINVAL);
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
      casual::xatmi::internal::error::set( casual::common::error::handler());
   }
   return casual::xatmi::internal::error::get() == 0 ? 0 : -1;
}

int tpgetrply( int *const descriptor, char** odata, long* olen, const long bitmap)
{
   return casual::xatmi::internal::error::wrap( [&](){

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
   return casual::xatmi::internal::error::wrap( [id](){
      casual::common::service::call::Context::instance().cancel( id);
   });
}

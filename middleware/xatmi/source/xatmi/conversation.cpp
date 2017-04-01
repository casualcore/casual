//!
//! casual 
//!

#include "xatmi.h"
#include "xatmi/internal.h"

#include "common/service/conversation/context.h"
#include "common/buffer/pool.h"

int tpconnect( const char* svc, const char* idata, long ilen, long flags)
{
   casual::xatmi::internal::error::set( 0);

   if( svc == nullptr)
   {
      casual::xatmi::internal::error::set( TPEINVAL);
      return -1;
   }

   try
   {
      auto buffer = casual::common::buffer::pool::Holder::instance().get( idata, ilen);

      using Flag = casual::common::service::conversation::connect::Flag;

      constexpr casual::common::service::conversation::connect::Flags valid_flags{
         Flag::no_block,
         Flag::no_time,
         Flag::no_transaction,
         Flag::receive_only,
         Flag::send_only,
         Flag::signal_restart};

      return casual::common::service::conversation::Context::instance().connect(
            svc,
            buffer,
            valid_flags.convert( flags));

   }
   catch( ...)
   {
      casual::xatmi::internal::error::set( casual::common::error::handler());
   }

   return -1;
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
               casual::xatmi::internal::error::set( 0);
               auto result = task();

               if( result)
               {
                  event = result.underlaying();
                  casual::xatmi::internal::error::set( TPEEVENT);
                  return -1;
               }
            }
            catch( const casual::common::exception::conversation::Event& exception)
            {
               event = exception.event.underlaying();
               casual::xatmi::internal::error::set( TPEEVENT);
               return -1;
            }
            catch( ...)
            {
               casual::xatmi::internal::error::set( casual::common::error::handler());
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
   return casual::xatmi::internal::error::wrap( [&](){
      casual::common::service::conversation::Context::instance().disconnect( id);
   });
}

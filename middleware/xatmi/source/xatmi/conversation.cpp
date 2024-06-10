//! 
//! Copyright (c) 2015, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!


#include "casual/xatmi.h"
#include "casual/xatmi/internal/code.h"
#include "casual/xatmi/internal/signal.h"

#include "common/service/conversation/context.h"
#include "common/buffer/pool.h"

int tpconnect( const char* svc, const char* idata, long ilen, long bitmap)
{
   casual::xatmi::internal::clear();

   if( svc == nullptr)
   {
      casual::xatmi::internal::error::set( casual::common::code::xatmi::argument);
      return -1;
   }

   try
   {
      auto buffer = casual::common::buffer::pool::holder().get( casual::common::buffer::handle::type{ idata}, ilen);

      using Flag = casual::common::service::conversation::connect::Flag;

      auto flags = Flag{ bitmap};

      constexpr auto valid_flags = Flag::no_block
         | Flag::no_time
         | Flag::no_transaction
         | Flag::receive_only
         | Flag::send_only
         | Flag::signal_restart;

      if( ! casual::common::flag::valid( valid_flags, flags))
         casual::common::code::raise::error( casual::common::code::xatmi::argument, "flags: ", flags, " outside of: ", valid_flags);

      auto maybe_block = casual::xatmi::internal::signal::maybe_block( flags);

      return casual::common::service::conversation::context().connect(
            svc,
            buffer,
            flags).value();

   }
   catch( ...)
   {
      casual::xatmi::internal::error::set( casual::xatmi::internal::exception::code());
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
               casual::xatmi::internal::clear();
               auto result = task();

               if( ! casual::common::flag::empty( result))
               {
                  event = std::to_underlying( result);
                  casual::xatmi::internal::error::set( casual::common::code::xatmi::event);
                  return -1;
               }
            }
            catch( const casual::common::exception::conversation::Event& exception)
            {
               event = std::to_underlying( exception);
               casual::xatmi::internal::error::set( casual::common::code::xatmi::event);
               return -1;
            }
            catch( ...)
            {
               casual::xatmi::internal::error::set( casual::xatmi::internal::exception::code());
               return -1;
            }
            return 0;
         }

      } // error
   } // <unnamed>
} // local

int tpsend( int id, const char* idata, long ilen, long bitmap, long* event)
{
   return local::conversation::wrap( *event, [&]()
   {
      auto buffer = casual::common::buffer::pool::holder().get( casual::common::buffer::handle::type{ idata}, ilen);

      using Flag = casual::common::service::conversation::send::Flag;

      auto flags = Flag{ bitmap};

      constexpr auto valid_flags = Flag::receive_only
         | Flag::no_block
         | Flag::no_time
         | Flag::signal_restart;

      if( ! casual::common::flag::valid( valid_flags, flags))
         casual::common::code::raise::error( casual::common::code::xatmi::argument, "flags: ", flags, " outside of: ", valid_flags);

      auto maybe_block = casual::xatmi::internal::signal::maybe_block( flags);

      auto result = casual::common::service::conversation::context().send(
            casual::common::strong::conversation::descriptor::id{ id},
            std::move( buffer),
            flags);
      // There may be a user return code. It need to be set.
      // Occurs if/when callee called tpreturn (and thereby
      // terminated the conversation). It is a bit unusaual
      // for a (conversational) service to call tpreturn when
      // not in control of the conversation (as we are in
      // tpsend, this side is in control of the conversation).
      // It is a kind of "service side abort" of the converation.
      // A call to tpreturn WITHOUT data shall according to the
      // XATMI spec result in a TPEV_SVCFAIL event, and this can
      // have a user return code. If the tpreturn call have data
      // the result will be a TPEV_SVCERR reported to the initiator...
      // A tpsend in the conversational service will never return
      // TPEV_SVCFAIL. An "initator side abort" of a conversation,
      // is done with tpdiscon() resulting in a TPEV_DISCONIMM.
      // "Abnormal" terminations of caller/initiator also
      // result in TPEV_DISCONIMM.
      //
      // The XATMI spec does not say if the global tpurcode
      // should be left unchanged on calls that do not return
      // a user return code (all cases except TPEV_SVCFAIL).
      // For now I always set it to whatever happens to be in
      // result.user (defaults to 0).
      casual::xatmi::internal::user::code::set( result.user);

      return result.event;

   });
}

int tprecv( int id, char ** odata, long *olen, long bitmap, long* event)
{
   return local::conversation::wrap( *event, [&](){

      auto buffer = casual::common::buffer::pool::holder().get( casual::common::buffer::handle::type{ *odata});

      using Flag = casual::common::service::conversation::receive::Flag;

      auto flags = Flag{ bitmap};

      constexpr auto valid_flags = Flag::no_change
         | Flag::no_block
         | Flag::no_time
         | Flag::signal_restart;

      if( ! casual::common::flag::valid( valid_flags, flags))
         casual::common::code::raise::error( casual::common::code::xatmi::argument, "flags: ", flags, " outside of: ", valid_flags);

      auto maybe_block = casual::xatmi::internal::signal::maybe_block( flags);

      auto result = casual::common::service::conversation::context().receive(
            casual::common::strong::conversation::descriptor::id{ id},
            flags);

      if( casual::common::flag::contains( flags, Flag::no_change) && buffer.payload().type != result.buffer.type)
         casual::common::code::raise::error( casual::common::code::xatmi::buffer_output);

      casual::common::buffer::pool::holder().deallocate( casual::common::buffer::handle::type{ *odata});

      auto output_buffer = casual::common::buffer::pool::holder().insert( std::move( result.buffer));
      *odata = std::get< 0>( output_buffer).underlying();
      *olen = std::get< 1>( output_buffer);

      // We also need to set any user return code that may be present. Happens when/if
      // the callee called tpreturn. A user return code is available on SVCSUCC or SVCFAIL.
      // Not in on SVCERR, but even i that case there is a value (==0) in result.user.
      casual::xatmi::internal::user::code::set( result.user);

      return result.event;

   });
}

int tpdiscon( int id)
{
   return casual::xatmi::internal::error::wrap( [&](){
      casual::common::service::conversation::context().disconnect( casual::common::strong::conversation::descriptor::id{ id});
   });
}

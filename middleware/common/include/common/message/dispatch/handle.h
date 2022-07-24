//! 
//! Copyright (c) 2015, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!


#pragma once


#include "common/message/server.h"
#include "common/message/domain.h"
#include "common/message/internal.h"
#include "common/message/dispatch.h"
#include "common/communication/ipc.h"
#include "common/log.h"


namespace casual
{
   namespace common::message::dispatch::handle
   {

      namespace dump
      {
         //! dumps the state to casual log.
         template< typename State>
         auto state( const State& state) noexcept
         {
            return [&state]( const message::internal::dump::State&)
            {
               std::ostringstream out;
               log::write( out, "state: ", state);
               log::stream::write( "casual.internal", std::move( out).str());
            };
         }
      } // dump


      using handler_type = decltype( message::dispatch::handler( communication::ipc::inbound::device()));

      //! @return a handler with all the default handlers:
      //! * Ping - Replies to a ping message
      //! * Shutdown - throws exception::casual::Shutdown if message::shutdown::Request is dispatched
      //! * global::State gather - global state_ from the process and reply the information to caller
      handler_type defaults() noexcept;

      //! @returns all defaults + state dump.
      template< typename State>
      handler_type defaults( const State& state) noexcept
      {
         auto result = defaults();
         result.insert( dump::state( state));
         return result;
      }

      //! Handles and discard a given message type
      template< typename Message> 
      auto discard()
      {
         return []( const Message& message)
         {
            log::line( log::debug, "discard message: ", message);
         };
      }

      //! Dispatch and assigns a given message
      template< typename M>
      auto assign( M& target)
      {
         return [&target]( M& message)
         {
            target = message;
         };
      }

   } // common::message::dispatch::handle
} // casual

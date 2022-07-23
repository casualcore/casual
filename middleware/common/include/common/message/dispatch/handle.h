//! 
//! Copyright (c) 2015, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!


#pragma once


#include "common/message/server.h"
#include "common/message/domain.h"
#include "common/message/dispatch.h"
#include "common/communication/ipc.h"
#include "common/log.h"

namespace casual
{
   namespace common::message::dispatch::handle
   {

      //! Replies to a ping message
      struct Ping
      {
         void operator () ( const server::ping::Request& message);
      };

      //! @throws exception::casual::Shutdown if message::shutdown::Request is dispatched
      struct Shutdown
      {
         void operator () ( const message::shutdown::Request& message);
      };

      namespace global
      {
         //! gather _global state_ from the process and reply the information to caller
         struct State 
         {
            void operator () ( const message::domain::instance::global::state::Request& message);
         };
      } // global

      using handler_type = decltype( message::dispatch::handler( communication::ipc::inbound::device()));

      //! @return a handler with all the default handlers
      handler_type defaults() noexcept;

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

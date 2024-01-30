//!
//! Copyright (c) 2020, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!

#pragma once

#include "gateway/group/inbound/state.h"
#include "gateway/group/ipc.h"

#include "common/message/dispatch.h"


namespace casual
{
   namespace gateway::group::inbound::handle
   {
      using internal_handler = decltype( common::message::dispatch::handler( ipc::inbound()));
      internal_handler internal( State& state);

      using external_handler = common::message::dispatch::basic_handler< common::communication::tcp::message::Complete, common::strong::socket::id>;
      external_handler external( State& state);

      namespace connection
      {
         //! tries to compensate for the lost connection.
         //! Removes all state associated with the connection.
         message::inbound::connection::Lost lost( State& state, common::strong::socket::id descriptor);

         //! will try to disconnect the socket, depending on the protocol version it's either 'smooth' or 'abrupt'
         void disconnect( State& state, common::strong::socket::id descriptor);
         
      } // connection

      //! take care of pending tasks, when message dispatch is idle.
      void idle( State& state);
      
      //! soft shutdown - tries to ask politely to outbounds, and then disconnect.
      void shutdown( State& state);

      //! hard shutdown - try to cancel stuff directly with best effort.
      void abort( State& state);

   } // gateway::group::inbound::handle
} // casual
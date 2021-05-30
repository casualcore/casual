//!
//! Copyright (c) 2020, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!

#pragma once

#include "gateway/group/outbound/state.h"

#include "common/message/dispatch.h"
#include "common/communication/ipc.h"


namespace casual
{
   namespace gateway::group::outbound::handle
   {
      using internal_handler = decltype( common::message::dispatch::handler( common::communication::ipc::inbound::device()));
      internal_handler internal( State& state);

      using external_handler = decltype( common::message::dispatch::handler( std::declval< common::communication::tcp::Duplex&>()));
      external_handler external( State& state);
      

      void unadvertise( state::Lookup::Resources keys);

      namespace connection
      {
         //! send error replies to all pending in-flight messages that is associated with the connection
         //! removes all state associated with the connection.
         std::optional< configuration::model::gateway::outbound::Connection> lost( State& state, common::strong::file::descriptor::id descriptor);

         //! unadvertise all associated resources to descriptor, mark the connection as 'disconnecting'
         void disconnect( State& state, common::strong::file::descriptor::id descriptor);
         
      } // connection

      //! take care of pending tasks, when message dispatch is idle.
      std::vector< configuration::model::gateway::outbound::Connection> idle( State& state);

      //! soft shutdown - tries to disconnect all connections
      void shutdown( State& state);

      //! hard shutdown - try to cancel stuff directly with best effort.
      void abort( State& state);


   
   } // gateway::group::outbound::handle

} // casual

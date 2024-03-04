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
      using internal_handler = common::message::dispatch::basic_handler< common::communication::ipc::message::Complete, common::strong::ipc::descriptor::id>;
      internal_handler internal( State& state);

      using external_handler = common::message::dispatch::basic_handler< common::communication::tcp::message::Complete, common::strong::socket::id>;
      external_handler external( State& state);

      using management_handler = common::message::dispatch::basic_handler< common::communication::ipc::message::Complete>;
      management_handler management( State& state);
      

      namespace connection
      {
         //! send error replies to all pending in-flight messages that is associated with the connection
         //! removes all state associated with the connection.
         message::outbound::connection::Lost lost( State& state, common::strong::socket::id descriptor);

         //! unadvertise all associated resources to descriptor, mark the connection as 'disconnecting'
         void disconnect( State& state, common::strong::socket::id descriptor);
         
      } // connection

      //! take care of pending tasks, when message dispatch is idle.
      void idle( State& state);

      //! soft shutdown - tries to disconnect all connections
      void shutdown( State& state);

      //! hard shutdown - try to cancel stuff directly with best effort.
      void abort( State& state);

      namespace metric
      {
         void send( State& state, const common::message::event::service::Calls& metric);
      } // metric


   
   } // gateway::group::outbound::handle

} // casual

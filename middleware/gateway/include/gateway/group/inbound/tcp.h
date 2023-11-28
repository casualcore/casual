//!
//! Copyright (c) 2023, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!

#pragma once

#include "gateway/group/inbound/state.h"
#include "gateway/group/inbound/handle.h"

#include "gateway/group/tcp.h"

namespace casual
{
   namespace gateway::group::inbound::tcp
   {
      template< typename M>
      common::strong::correlation::id send( State& state, common::strong::file::descriptor::id descriptor, M&& message)
      {
         return group::tcp::send( state, &handle::connection::lost, descriptor, std::forward< M>( message));
      }

      template< typename M>
      common::strong::file::descriptor::id send( State& state, M&& message)
      {
         if( auto descriptor = state.consume( correlation( message)))
         {
            if( tcp::send( state, descriptor, std::forward< M>( message)))
               return descriptor;
         }
      
         common::log::error( common::code::casual::communication_unavailable, "connection absent when trying to send reply - ", message.type());
         common::log::line( common::log::category::verbose::error, "state: ", state);
      
         return {};
      }

   } // gateway::group::inbound::tcp
} // casual

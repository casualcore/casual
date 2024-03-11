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
      common::strong::correlation::id send( State& state, common::strong::socket::id descriptor, M&& message)
      {
         return group::tcp::send( state, &handle::connection::lost, descriptor, std::forward< M>( message));
      }

   } // gateway::group::inbound::tcp
} // casual

//!
//! Copyright (c) 2020, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!

#pragma once

#include "gateway/reverse/outbound/state.h"

#include "common/message/dispatch.h"
#include "common/communication/ipc.h"


namespace casual
{
   namespace gateway::reverse::outbound::handle
   {
      using internal_handler = decltype( common::message::dispatch::handler( common::communication::ipc::inbound::device()));
      internal_handler internal( State& state);

      using external_handler = decltype( common::message::dispatch::handler( std::declval< common::communication::tcp::Duplex&>()));
      external_handler external( State& state);
   
   } // gateway::reverse::outbound::handle

} // casual

//!
//! Copyright (c) 2020, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!

#pragma once

#include "queue/forward/state.h"

#include "common/message/dispatch.h"
#include "common/communication/ipc.h"

namespace casual
{
   namespace queue::forward
   {
      using handler_type = decltype( common::message::dispatch::handler( common::communication::ipc::inbound::device()));

      handler_type handlers( State& state);
         
   } // queue::forward
} // casual
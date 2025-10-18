//!
//! Copyright (c) 2025, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!

#pragma once

#include "queue/fanout/state.h"

#include "common/message/dispatch.h"
#include "common/communication/ipc/message.h"

namespace casual
{
   namespace queue::fanout::handle
   {
      using handler_type = common::message::dispatch::basic_handler< common::communication::ipc::message::Complete>;

      void abort( State& state);

      handler_type create( State& state);

      

   } // queue::fanout::handle
} // casual

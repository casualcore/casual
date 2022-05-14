//!
//! Copyright (c) 2021, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!

#pragma once

#include "domain/discovery/state.h"

#include "common/communication/ipc.h"
#include "common/message/dispatch.h"

namespace casual
{
   namespace domain::discovery::handle
   {
      using dispatch_type = decltype( common::message::dispatch::handler( common::communication::ipc::inbound::device()));

      void idle( State& state);

      dispatch_type create( State& state);

   } // domain::discovery::handle
} // casual
//! 
//! Copyright (c) 2015, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!


#pragma once

#include "service/forward/state.h"

#include "common/communication/ipc.h"
#include "common/message/dispatch.h"

namespace casual
{
   namespace service::forward::handle
   {
      using dispatch_type = decltype( common::message::dispatch::handler( common::communication::ipc::inbound::device()));

      dispatch_type create( State& state);

   } // service::forward::handle
} // casual



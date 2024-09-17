//!
//! Copyright (c) 2024, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!

#pragma once

#include "file/manager/state.h"

#include "common/message/dispatch.h"
#include "common/communication/ipc.h"

namespace casual
{
   namespace file::manager::handle
   {

      using dispatch_type = decltype( common::message::dispatch::handler( common::communication::ipc::inbound::device()));

      //! @returns all the handlers for service manager
      dispatch_type create( State& state);

   } // file::manager::handle

} // casual

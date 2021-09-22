//!
//! Copyright (c) 2021, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!

#pragma once

#include "http/outbound/state.h"

#include "common/message/dispatch.h"
#include "common/communication/ipc.h"
#include "common/functional.h"

namespace casual
{
   namespace http::outbound::handle
   {
      namespace internal
      {
         using handler_type = decltype( common::message::dispatch::handler( common::communication::ipc::inbound::device()));

         handler_type create( State& state);
      } // internal

      namespace external
      {
         common::function< void( state::pending::Request&&, curl::type::code::easy)> reply( State& state);
      } // external

      
   } // http::outbound::handle
} // casual
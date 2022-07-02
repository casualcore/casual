//!
//! Copyright (c) 2021, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!

#pragma once

#include "common/communication/ipc.h"
#include "common/communication/ipc/flush/send.h"
#include "common/communication/instance.h"
#include "common/communication/select.h"

namespace casual
{
   namespace gateway::group::ipc
   {
      
      namespace manager
      {
         inline auto& service() { return common::communication::instance::outbound::service::manager::device();}
         inline auto& transaction() { return common::communication::instance::outbound::transaction::manager::device();}
         inline auto& gateway() { return common::communication::instance::outbound::gateway::manager::device();}
         namespace optional
         {
            inline auto& queue() { return common::communication::instance::outbound::queue::manager::optional::device();}
         } // optional

      } // manager

      inline auto& inbound() { return common::communication::ipc::inbound::device();}

      namespace flush
      {
         using namespace common::communication::ipc::flush;

      } // flush


   } // gateway::group::ipc
} // casual
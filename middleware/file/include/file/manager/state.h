//!
//! Copyright (c) 2024, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!

#pragma once

#include "common/communication/select.h"
#include "common/communication/ipc/send.h"
#include "common/state/machine.h"

#include "file/message.h"

#include <chrono>
#include "casual/manager/service/context.h"
#include "casual/manager/service/policy.h"

#include <vector>

namespace casual
{
   namespace file::manager
   {
      struct State
      {
         enum struct Runlevel : short
         {
            running,
            shutdown,
         };

         struct Request : file::message::reserve::Request
         {
            std::chrono::system_clock::time_point time;
         };

         common::communication::select::Directive directive;
         common::communication::ipc::send::Coordinator multiplex{ directive};

         common::state::Machine< Runlevel, Runlevel::running> runlevel;

         std::vector< Request> working;
         std::vector< Request> pending;

         casual::manager::service::Context< casual::manager::service::policy::Default> services;

         bool done() const;

         CASUAL_LOG_SERIALIZE(
            CASUAL_SERIALIZE( directive);
            CASUAL_SERIALIZE( multiplex);
            CASUAL_SERIALIZE( runlevel);
            CASUAL_SERIALIZE( working);
            CASUAL_SERIALIZE( pending);
            CASUAL_SERIALIZE( services);
         )
      };

      std::string_view description( State::Runlevel value);

   }
} // casual

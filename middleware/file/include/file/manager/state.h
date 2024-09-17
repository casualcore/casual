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

         common::communication::select::Directive directive;
         common::communication::ipc::send::Coordinator multiplex{ directive};

         common::state::Machine< Runlevel, Runlevel::running> runlevel;

         std::vector< file::message::reserve::Request> working;
         std::vector< file::message::reserve::Request> pending;

         bool done() const;

         CASUAL_LOG_SERIALIZE(
            CASUAL_SERIALIZE( directive);
            CASUAL_SERIALIZE( multiplex);
            CASUAL_SERIALIZE( runlevel);
            CASUAL_SERIALIZE( working);
            CASUAL_SERIALIZE( pending);
         )
      };

      std::string_view description( State::Runlevel value);

   }
} // casual

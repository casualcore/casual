//!
//! Copyright (c) 2021, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!

#pragma once

#include "common/message/service.h"
#include "common/state/machine.h"
#include "common/communication/select.h"
#include "common/communication/ipc/send.h"

#include <vector>
#include <string>


namespace casual
{
   namespace service::forward
   {
      namespace state
      {
         enum struct Runlevel : short
         {
            running,
            shutdown,
         };
         constexpr std::string_view description( Runlevel value)
         {
            switch( value)
            {
               case Runlevel::running: return "running";
               case Runlevel::shutdown: return "shutdown";
            }
            return "<unknown>";
         }
      } // state
      
      struct State
      {
         inline State()
         {
            directive.read.add( common::communication::ipc::inbound::device().descriptor());
         }

         common::state::Machine< state::Runlevel> runlevel;
         common::communication::select::Directive directive;

         std::vector< common::message::service::call::callee::Request> pending;

         common::communication::ipc::send::Coordinator multiplex{ directive};

         inline bool done() const noexcept
         {
            return runlevel > state::Runlevel::running;
         }

         CASUAL_LOG_SERIALIZE(
            CASUAL_SERIALIZE( runlevel);
            CASUAL_SERIALIZE( pending);
            CASUAL_SERIALIZE( multiplex);
         )
      };

   } // service::forward
} // casual
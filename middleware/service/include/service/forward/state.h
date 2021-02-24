//!
//! Copyright (c) 2021, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!

#pragma once

#include "common/message/service.h"
#include "common/state/machine.h"

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

         inline std::ostream& operator << ( std::ostream& out, Runlevel value)
         {
            switch( value)
            {
               case Runlevel::running: return out << "running";
               case Runlevel::shutdown: return out << "shutdown";
            }
            return out << "<unknown>";
         }

      } // state
      
      struct State
      {
         common::state::Machine< state::Runlevel> runlevel;

         std::vector< common::message::service::call::callee::Request> pending;

         inline bool done() const noexcept
         {
            return runlevel > state::Runlevel::running;
         }

         CASUAL_LOG_SERIALIZE(
            CASUAL_SERIALIZE( runlevel);
            CASUAL_SERIALIZE( pending);
         )
      };

   } // service::forward
} // casual
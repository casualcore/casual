//!
//! Copyright (c) 2024, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!

#include "file/manager/state.h"

namespace casual
{
   namespace file::manager
   {
      bool State::done() const
      {
         return runlevel == Runlevel::shutdown && working.empty();
      }

      std::string_view description( const State::Runlevel value)
      {
         switch( value)
         {
         case State::Runlevel::running:
            return "running";
         case State::Runlevel::shutdown:
            return "shutdown";
         default:
            std::unreachable();
         }
      }

   } // file::manager
   
} // casual

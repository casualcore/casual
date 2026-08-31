//!
//! Copyright (c) 2020, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!

#include "queue/forward/state.h"
#include "queue/common/log.h"

#include <ostream>

namespace casual
{
   using namespace common;
   namespace queue::forward
   {
      namespace state
      {
         std::string_view description( Runlevel value)
         {
            switch( value)
            {
               case Runlevel::startup: return "startup";
               case Runlevel::running: return "running";
               case Runlevel::shutdown: return "shutdown";
            }
            return "<unknown>";
         }

         namespace forward
         {
            Instances& Instances::operator++()
            {
               ++running;
               return *this;
            }

            Instances& Instances::operator--()
            {
               assert( running > 0);
               --running;

               return *this;
            }

         } // forward
      } // state

      bool State::done() const noexcept
      {
         if( runlevel <= state::Runlevel::running)
            return false;

         auto absent = []( auto& forward){ return forward.instances.absent();};

         return algorithm::all_of( forward.services, absent)
            && algorithm::all_of( forward.queues, absent);
      }

      state::forward::Service* State::forward_service( state::forward::id id) noexcept
      {
         if( auto found = common::algorithm::find( forward.services, id))
            return found.data();
         return nullptr;
      }

      state::forward::Queue* State::forward_queue( state::forward::id id) noexcept
      {
         if( auto found = common::algorithm::find( forward.queues, id))
            return found.data();
         return nullptr;
      }

   } // queue::forward
} // casual
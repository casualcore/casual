//!
//! Copyright (c) 2020, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!

#include "queue/forward/state.h"

#include <ostream>

namespace casual
{
   using namespace common;
   namespace queue::forward
   {
      namespace state
      {

         std::ostream& operator << ( std::ostream& out, Runlevel value)
         {
            switch( value)
            {
               case Runlevel::startup: return out << "startup";
               case Runlevel::running: return out << "running";
               case Runlevel::shutdown: return out << "shutdown";
            }
            return out << "<unknown>";
         }

         namespace forward
         {
            Service& Service::operator++()
            {
               ++instances.running;
               return *this;
            }

            Service& Service::operator--()
            {
               assert( instances.running > 0);
               --instances.running;

               if( instances.absent())
               {
                  source.process = {};
                  if( reply)
                     reply.value().process = {};
               }

               return *this;
            }

            Queue& Queue::operator++()
            {
               ++instances.running;
               return *this;
            }

            Queue& Queue::operator--()
            {
               assert( instances.running > 0);
               --instances.running;

               if( instances.absent())
               {
                  source.process = {};
                  target.process = {};
               }

               return *this;
            }

         } // forward

      } // state

      bool State::done() const noexcept
      {
         auto absent = []( auto& forward){ return forward.instances.absent();};

         return runlevel == state::Runlevel::shutdown 
            && algorithm::all_of( forward.services, absent)
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
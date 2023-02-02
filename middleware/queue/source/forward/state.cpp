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
      namespace local
      {
         namespace
         {
            namespace extract
            {
               template< typename P>
               auto pending( P& pending, state::forward::id id)
               {
                  return algorithm::container::extract( pending, 
                     std::get< 1>( algorithm::partition( pending, predicate::negate( predicate::value::equal( id)))));
               }
            } // extract
            
         } // <unnamed>
      } // local

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

            void Service::invalidate() noexcept
            {
               log::line( verbose::log, "invalidate: ", *this);

               instances.running = 0;
               source.process = {};

               if( reply)
                  reply.value().process = {};
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

            void Queue::invalidate() noexcept
            {
               log::line( verbose::log, "invalidate: ", *this);

               instances.running = 0;
               source.process = {};
             
               target.process = {};
            }

         } // forward

         std::vector< forward::id> Forward::ids( common::strong::process::id pid) const noexcept
         {
            auto transform_id = []( auto& forward){ return forward.id;};
            auto has_pid = [ pid]( auto& forward){ return forward == pid;};

            auto ids = algorithm::transform_if( queues, transform_id, has_pid);
            algorithm::transform_if( services, std::back_inserter( ids), transform_id, has_pid);

            return ids;
         }

      } // state

      bool State::done() const noexcept
      {
         auto absent = []( auto& forward){ return forward.instances.absent();};

         return runlevel > state::Runlevel::running
            && algorithm::all_of( forward.services, absent)
            && algorithm::all_of( forward.queues, absent);
      }

      void State::invalidate( const std::vector< state::forward::id>& ids) noexcept
      {
         Trace trace{ "queue::forward::State::remove"};
         log::line( verbose::log, "ids: ", ids);

         for( auto id : ids)
            forward_apply( id, []( auto& forward){ forward.invalidate();});
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
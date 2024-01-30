//! 
//! Copyright (c) 2015, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!


#include "queue/manager/state.h"
#include "queue/common/log.h"
#include "queue/common/queue.h"
#include "common/algorithm/container.h"

#include "common/algorithm.h"

namespace casual
{
   using namespace common;

   namespace queue::manager
   {
      namespace state
      {
         namespace entity
         {
            std::string_view description( Lifetime value)
            {
               switch( value)
               {
                  case Lifetime::absent: return "absent";
                  case Lifetime::spawned: return "spawned";
                  case Lifetime::connected: return "connected";
                  case Lifetime::running: return "running";
                  case Lifetime::shutdown: return "shutdown";
               }
               return "<unknown>";
            }  
         } // entity

         std::string_view description( Runlevel value)
         {
            switch( value)
            {
               case Runlevel::configuring: return "configuring";
               case Runlevel::running: return "running";
               case Runlevel::shutdown: return "shutdown";
               case Runlevel::error: return "error";
            }
            return "<unknown>";
         }
      } // state

      const state::Queue* State::queue( const std::string& name) const noexcept
      {
         if( auto found = algorithm::find( queues, name))
            if( ! found->second.empty())
               return &range::front( found->second);

         return nullptr;
      }

      const state::Queue* State::local_queue( const std::string& name) const noexcept
      {
         if( auto found = algorithm::find( queues, name))
            if( ! found->second.empty() && range::front( found->second).local())
               return &range::front( found->second);

         return nullptr;
      }

      void State::update( queue::ipc::message::group::configuration::update::Reply reply)
      {
         Trace trace{ "queue::manager::State::update"};

         // we'll make it easy - first we remove all queues associated with the pid, then we'll add 
         // the 'configured' ones

         remove_queues( reply.process.pid);

         if( auto found = algorithm::find( groups, reply.process.pid))
            found->state = decltype( found->state())::running;
         else
            log::line( log::category::error, "failed to correlate group", reply.process, " - action: discard");
            

         for( auto& queue : reply.queues)
         {
            auto& instances = queues[ queue.name];
            instances.emplace_back( reply.process, queue.id);
            algorithm::sort( instances);
         }
      }

      void State::remove_queues( common::strong::process::id pid)
      {
         Trace trace{ "queue::manager::State::remove_queues"};

         common::algorithm::container::erase_if( queues, common::predicate::adapter::second( [pid]( auto& instances){
            return common::algorithm::container::trim( instances, common::algorithm::remove( instances, pid)).empty();
         }));

         log::line( log, "queues: ", queues);
      }

      void State::remove( common::strong::process::id pid)
      {
         Trace trace{ "queue::manager::State::remove"};

         remove_queues( pid);

         algorithm::container::erase( groups, pid);
         algorithm::container::erase( forward.groups, pid);
         algorithm::container::erase( pending.lookups, pid);
         algorithm::container::erase( remotes, pid);
      }

      void State::update( queue::ipc::message::Advertise& message)
      {
         Trace trace{ "queue::manager::State::update"};

         // we only get queue::ipc::message::Advertise from _outbounds_, never
         // from this domains queue-groups. Hence, the advertised queues are
         // remote queues.
         
         // outbound order is zero-based, we add 1 to give it lower prio.
         auto order = message.order + 1;

         // make sure we've got the instance
         if( ! common::algorithm::find( remotes, message.process))
            remotes.push_back( state::Remote{ message.process, order});

         auto add_queue = [&]( auto& queue)
         {
            auto& instances = queues[ queue.name];

            instances.emplace_back( message.process, queue::remote::queue::id, order);

            // Make sure we prioritize local queue
            common::algorithm::stable_sort( instances);
         };

         algorithm::for_each( message.queues.add, add_queue);


         auto remove_queue = [&]( auto& name)
         {
            if( auto found = algorithm::find( queues, name))
               if( common::algorithm::container::erase( found->second, message.process.ipc).empty())
                  queues.erase( name);
         };

         algorithm::for_each( message.queues.remove, remove_queue);
      }

      bool State::done() const
      {
         if( runlevel <= decltype( runlevel())::running)
            return false;

         return groups.empty() && forward.groups.empty();
      }

      bool State::ready() const
      {
         auto is_running = []( auto& group){ return group.state >= decltype( group.state())::running;};
         
         return algorithm::all_of( groups, is_running) && algorithm::all_of( forward.groups, is_running);
      }

   } // queue::manager
} // casual

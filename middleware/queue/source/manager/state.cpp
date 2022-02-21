//! 
//! Copyright (c) 2015, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!


#include "queue/manager/state.h"
#include "queue/common/log.h"
#include "queue/common/queue.h"

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
            std::ostream& operator << ( std::ostream& out, Lifetime value)
            {
               switch( value)
               {
                  case Lifetime::absent: return out << "absent";
                  case Lifetime::spawned: return out << "spawned";
                  case Lifetime::connected: return out << "connected";
                  case Lifetime::running: return out << "running";
                  case Lifetime::shutdown: return out << "shutdown";
               }
               return out << "<unknown>";
            }  
         } // entity

         std::ostream& operator << ( std::ostream& out, Runlevel value)
         {
            switch( value)
            {
               case Runlevel::running: return out << "running";
               case Runlevel::shutdown:  return out << "shutdown";
               case Runlevel::error: return out << "error";
            }
            return out << "<unknown>";
         }
      } // state

      const state::Queue* State::queue( const std::string& name) const noexcept
      {
         if( auto found = algorithm::find( queues, name))
            if( ! found->second.empty())
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

      std::vector< common::strong::process::id> State::processes() const noexcept
      {
         return algorithm::transform( groups, []( auto& group){ return group.process.pid;});
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

         common::algorithm::container::trim( groups, common::algorithm::remove( groups, pid));
         common::algorithm::container::trim( forward.groups, common::algorithm::remove( forward.groups, pid));
         common::algorithm::container::trim( pending.lookups, common::algorithm::remove( pending.lookups, pid));
         common::algorithm::container::trim( remotes, common::algorithm::remove( remotes, pid));
      }

      void State::update( queue::ipc::message::Advertise& message)
      {
         Trace trace{ "queue::manager::State::update"};

         // we only get queue::ipc::message::Advertise from _outbounds_, never
         // from this domains queue-groups. Hence, the advertised queues are
         // remote queues.

         if( message.reset)
            remove_queues( message.process.pid);
         
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
            auto& instances = queues[ name];

            common::algorithm::container::trim( instances, common::algorithm::remove_if( instances, [&]( auto& queue)
            {
               return message.process.pid == queue.process.pid;
            }));

            if( instances.empty())
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
         auto is_running = []( auto& entity){ return entity.state >= decltype( entity.state())::running;};
         
         return algorithm::all_of( groups, is_running) && algorithm::all_of( forward.groups, is_running);
      }

   } // queue::manager
} // casual

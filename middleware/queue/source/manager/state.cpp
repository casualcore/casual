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

      const state::Queue* State::queue( const std::string& name, queue::ipc::message::lookup::request::context::Action action) const noexcept
      {
         auto enabled_queue = [ action]( auto& queue)
         {
            if( action == decltype( action)::enqueue)
               return queue.enable.enqueue;
            if( action == decltype( action)::dequeue)
               return queue.enable.dequeue;

            return true;
         };

         if( auto found = algorithm::find( queues, name))
            if( auto queue = algorithm::find_if( found->second, enabled_queue))
               return queue.data();

         return nullptr;
      }

      const state::Queue* State::local_queue( const std::string& name, queue::ipc::message::lookup::request::context::Action action) const noexcept
      {
         // we don't know the action from a remote discovery (absent), if the queue has one
         // action enabled we have to reply with the queue... I think. This will lead to possible
         // enqueue/dequeue errors later on. I'm not sure of what effect this has in practice.
         auto local_enabled_queue = [ action]( auto& queue)
         {
            if( action == decltype( action)::enqueue && ! queue.enable.enqueue)
               return false;
            if( action == decltype( action)::dequeue && ! queue.enable.dequeue)
               return false;
               
            return queue.local();
         };

         if( auto found = algorithm::find( queues, name))
            if( auto queue = algorithm::find_if( found->second, local_enabled_queue))
               return queue.data();

         return nullptr;
      }

      void State::update( queue::ipc::message::group::configuration::update::Reply reply)
      {
         Trace trace{ "queue::manager::State::update"};

         // we'll make it easy - first we remove all queues associated with the pid, then we'll add 
         // the 'configured' ones

         remove_queues( reply.process.pid);

         auto update_group = []( auto& state, auto& group, auto& reply)
         {
            group.state = decltype( group.state())::running;

            for( auto& queue : reply.queues)
            {
               auto& instances = state.queues[ queue.name];
               {
                  auto& instance = instances.emplace_back();
                  instance.process = group.process;
                  instance.queue = queue.id;
                  
                  // get the enable from configuration
                  if( auto found = algorithm::find( group.configuration.queues, queue.name))
                     instance.enable = found->enable;
               }
               
               algorithm::sort( instances);
            }
         };

         if( auto found = algorithm::find( groups, reply.process.pid))
            update_group( *this, *found, reply);
         else
            log::line( log::category::error, "failed to correlate group", reply.process, " - action: discard");
            
      }

      namespace local
      {
         namespace
         {
            void remove_queues( State& state, common::process::compare_equal_to_handle auto id)
            {
               common::algorithm::container::erase_if( state.queues, [ id]( auto& pair)
               {
                  return common::algorithm::container::erase( pair.second, id).empty();
               });

               log::line( log, "state.queues: ", state.queues);
            }

         } // <unnamed>
      } // local

      void State::remove_queues( common::strong::process::id pid)
      {
         Trace trace{ "queue::manager::State::remove_queues"};
         log::line( log, "pid: ", pid);

         local::remove_queues( *this, pid);
      }

      void State::remove_queues( common::strong::ipc::id ipc)
      {
         Trace trace{ "queue::manager::State::remove_queues"};
         log::line( log, "ipc: ", ipc);

         local::remove_queues( *this, ipc);
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
         
         if( message.reset)         
            return remove_queues( message.process.ipc);


         // outbound order is zero-based, we add 1 to give it lower prio.
         auto order = message.order + 1;

         // make sure we've got the instance
         if( ! common::algorithm::find( remotes, message.process))
            remotes.push_back( state::Remote{ message.process, order, message.alias, message.description});

         auto add_queue = [&]( auto& queue)
         {
            auto& instances = queues[ queue.name];
            auto& instance = instances.emplace_back();// message.process, queue::remote::queue::id, order);
            instance.process = message.process;
            instance.queue = queue::remote::queue::id;
            instance.order = order;
            instance.enable.enqueue = queue.enable.enqueue;
            instance.enable.dequeue = queue.enable.dequeue;

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

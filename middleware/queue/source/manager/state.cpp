//! 
//! Copyright (c) 2015, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!


#include "queue/manager/state.h"
#include "queue/common/log.h"

#include "common/algorithm.h"

namespace casual
{
   using namespace common;

   namespace queue
   {
      namespace manager
      {

         State::State() = default;

         State::Group::Group() = default;
         State::Group::Group( std::string name, common::process::Handle process) 
            : name( std::move( name)), process( std::move( process)) {}

         bool operator == ( const State::Group& lhs, common::strong::process::id pid) { return lhs.process.pid == pid;}

         static_assert( common::traits::is_movable< State::Group>::value, "not movable");

         State::Remote::Remote() = default;
         State::Remote::Remote( common::process::Handle process)
            : process{ std::move( process)} {}


         bool operator == ( const State::Remote& lhs, const common::process::Handle& rhs)
         {
            return lhs.process == rhs;
         }

         bool operator == ( const State::Remote& lhs, common::strong::process::id pid)
         {
            return lhs.process == pid;
         }

         static_assert( common::traits::is_movable< State::Remote>::value, "not movable");


         bool operator < ( const State::Queue& lhs, const State::Queue& rhs)
         {
            return lhs.order < rhs.order;
         }

         std::vector< common::strong::process::id> State::processes() const noexcept
         {
            return algorithm::transform( groups, []( auto& group){ return group.process.pid;});
         }


         void State::remove_queues( common::strong::process::id pid)
         {
            Trace trace{ "queue::manager::State::remove_queues"};

            common::algorithm::erase_if( queues, common::predicate::adapter::second( [pid]( auto& instances){
               return common::algorithm::trim( instances, common::algorithm::remove( instances, pid)).empty();
            }));

            log::line( log, "queues: ", queues);
         }

         void State::remove( common::strong::process::id pid)
         {
            Trace trace{ "queue::manager::State::remove"};

            remove_queues( pid);

            common::algorithm::trim( groups, common::algorithm::remove( groups, pid));
            common::algorithm::trim( remotes, common::algorithm::remove( remotes, pid));
            common::algorithm::trim( forwards, common::algorithm::remove( forwards, pid));
            common::algorithm::trim( pending.lookups, common::algorithm::remove( pending.lookups, pid));
         }

         void State::update( common::message::queue::concurrent::Advertise& message)
         {
            Trace trace{ "queue::manager::State::update"};

            if( message.reset)
               remove_queues( message.process.pid);

            // make sure we've got the instance
            if( ! common::algorithm::find( remotes, message.process))
               remotes.emplace_back( message.process);

            auto add_queue = [&]( auto& queue)
            {
               auto& instances = queues[ queue.name];

               // outbound order is zero-based, we add 1 to distinguish local from remote
               instances.emplace_back( message.process, common::strong::queue::id{}, message.order + 1);

               // Make sure we prioritize local queue
               common::algorithm::stable_sort( instances);
            };

            algorithm::for_each( message.queues.add, add_queue);


            auto remove_queue = [&]( auto& name)
            {
               auto& instances = queues[ name];

               common::algorithm::trim( instances, common::algorithm::remove_if( instances, [&]( auto& queue)
               {
                  return message.process.pid == queue.process.pid;
               }));

               if( instances.empty())
                  queues.erase( name);
            };

            algorithm::for_each( message.queues.remove, remove_queue);
         }

      } // manager
   } // queue
} // casual

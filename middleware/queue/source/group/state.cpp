//!
//! Copyright (c) 2019, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!

#include "queue/group/state.h"
#include "queue/common/log.h"

#include "common/algorithm.h"
#include "common/algorithm/container.h"

namespace casual
{
   using namespace common;
   namespace queue::group
   {
      namespace state
      {
         std::string_view description( Runlevel value)
         {
            switch( value)
            {
               case Runlevel::running: return "running";
               case Runlevel::shutdown:  return "shutdown";
               case Runlevel::error: return "error";
            }
            return "<unknown>";
         }

         
         void Pending::add( ipc::message::group::dequeue::Request&& message)
         {
            dequeues.push_back( std::move( message));
         }

         ipc::message::group::dequeue::forget::Reply Pending::forget( const ipc::message::group::dequeue::forget::Request& message)
         {
            queue::Trace trace{ "queue::group::state::Pending::forget"};
            log::line( verbose::log, "message: ", message);

            auto result = common::message::reverse::type( message);

            auto [ keep, remove] = algorithm::partition( dequeues, [&message]( auto& m){ return m.correlation != message.correlation;});

            log::line( verbose::log, "found: ", remove);
            
            result.found = ! remove.empty();

            algorithm::container::trim( dequeues, keep);

            return result;
         }

         common::communication::ipc::pending::Holder Pending::forget()
         {
            queue::Trace trace{ "queue::group::state::Pending::forget"};
            return algorithm::transform( std::exchange( dequeues, {}), []( auto&& request)
            {
               ipc::message::group::dequeue::forget::Request result{ process::handle()};
               result.correlation = request.correlation;
               result.queue = request.queue;
               result.name = request.name;

               return common::communication::ipc::pending::Message{ request.process, std::move( result)};
            });
         }

         std::vector< ipc::message::group::dequeue::Request> Pending::extract( std::vector< common::strong::queue::id> queues)
         {
            queue::Trace trace{ "queue::group::state::Pending::extract"};

            auto partition = algorithm::partition( dequeues, [&queues]( auto& v)
            {
               return algorithm::find_if( queues, [&v]( auto q){ return q == v.queue;}).empty();
            });

            auto result = range::to_vector( std::get< 1>( partition));

            // trim away the extracted 
            algorithm::container::trim( dequeues, std::get< 0>( partition));

            return result;
         }

         void Pending::remove( common::strong::process::id pid)
         {
            queue::Trace trace{ "queue::group::state::Pending::remove"};
            log::line( verbose::log, "pid: ", pid);

            algorithm::container::trim( dequeues, algorithm::remove( dequeues, pid));

            replies.remove( pid);
         }
      } // state

      bool State::done() const noexcept
      {
         // likely branch
         if( runlevel == state::Runlevel::running)
            return false;

         return pending.empty() && involved.empty();
      }

   } // queue::group
} // casual
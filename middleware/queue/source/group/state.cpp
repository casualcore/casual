//!
//! Copyright (c) 2019, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!

#include "queue/group/state.h"
#include "queue/common/log.h"

#include "common/algorithm.h"

namespace casual
{
   using namespace common;
   namespace queue
   {
      namespace group
      {
         void State::Pending::add( common::message::queue::dequeue::Request&& message)
         {
            dequeues.push_back( std::move( message));
         }

         common::message::queue::dequeue::forget::Reply State::Pending::forget( const common::message::queue::dequeue::forget::Request& message)
         {
            queue::Trace trace{ "queue::group::State::Pending::forget"};
            log::line( verbose::log, "message: ", message);

            auto result = common::message::reverse::type( message);

            if( auto found = algorithm::find( dequeues, message.correlation))
            {
               result.found = true;
               dequeues.erase( std::begin( found));
            }
            else 
            {
               // we assume that we've sent the dequeue already, but caller hasn't consumed the message yet...
               result.found = false;
               log::line( verbose::log, "failed to correlate pending dequeue from: ", message.process);
               log::line( verbose::log, "message: ", message);
               log::line( verbose::log, "dequeues: ", dequeues);
            }

            return result;
         }

         std::vector< common::message::pending::Message> State::Pending::forget()
         {
            return algorithm::transform( std::exchange( dequeues, {}), []( auto&& request)
            {
               common::message::queue::dequeue::forget::Request result{ process::handle()};
               result.correlation = request.correlation;
               result.queue = request.queue;
               result.name = request.name;

               return common::message::pending::Message{ std::move( result), request.process};
            });
         }

         std::vector< common::message::queue::dequeue::Request> State::Pending::extract( std::vector< common::strong::queue::id> queues)
         {
            queue::Trace trace{ "queue::group::State::Pending::extract"};

            auto partition = algorithm::partition( dequeues, [&queues]( auto& v)
            {
               return algorithm::find_if( queues, [&v]( auto q){ return q == v.queue;}).empty();
            });

            auto result = range::to_vector( std::get< 1>( partition));

            // trim away the extracted 
            algorithm::trim( dequeues, std::get< 0>( partition));

            return result;
         }

         void State::Pending::remove( common::strong::process::id pid)
         {
            queue::Trace trace{ "queue::group::State::Pending::remove"};
            log::line( verbose::log, "pid: ", pid);

            auto remove = []( auto& container, auto predicate)
            {
               algorithm::trim( container, algorithm::remove_if( container, predicate));
            };

            remove( dequeues, [pid]( auto& m) { return m.process.pid == pid;});

            remove( replies, [pid]( auto& m) 
            { 
               m.remove( pid);
               return m.sent();
            });
         }
      } // group
   } // queue
} // casual
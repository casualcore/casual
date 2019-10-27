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

         static_assert( common::traits::is_movable< State::Remote>::value, "not movable");


         bool operator < ( const State::Queue& lhs, const State::Queue& rhs)
         {
            return lhs.order < rhs.order;
         }

         std::vector< common::strong::process::id> State::processes() const
         {
            std::vector< common::strong::process::id> result;

            for( auto& group : groups)
            {
               result.push_back( group.process.pid);
            }

            return result;
         }


         void State::remove_queues( common::strong::process::id pid)
         {
            Trace trace{ "queue::broker::State::remove_queues"};

            common::algorithm::erase_if( queues, [pid]( std::vector< Queue>& instances){
               return common::algorithm::trim( instances, common::algorithm::remove_if( instances, [pid]( const Queue& q){
                  return pid == q.process.pid;
               })).empty();
            });

            log::line( log, "queues: ", queues);
         }

         void State::remove( common::strong::process::id pid)
         {
            Trace trace{ "queue::broker::State::remove"};

            remove_queues( pid);

            common::algorithm::trim( groups, common::algorithm::remove_if( groups, [pid]( const auto& g){
               return pid == g.process.pid;
            }));


            common::algorithm::trim( remotes, common::algorithm::remove_if( remotes, [pid]( const auto& g){
               return pid == g.process.pid;
            }));

         }

         void State::update( common::message::queue::concurrent::Advertise& message)
         {
            Trace trace{ "queue::broker::State::update"};

            using directive_type = decltype(message.directive);

            switch( message.directive)
            {
               case directive_type::replace:
               {
                  remove_queues( message.process.pid);

                  // We fall through and add the queues, if any.
               }
               // no break
               case directive_type::add:
               {
                  if( ! message.queues.empty())
                  {
                     // We only add gateway and queues if there are any.
                     if( ! common::algorithm::find( remotes, message.process))
                     {
                        remotes.emplace_back( message.process);
                     }

                     for( const auto& queue : message.queues)
                     {
                        auto& instances = queues[ queue.name];

                        // outbound order is zero-based, we add 1 to distinguish local from remote
                        instances.emplace_back( message.process, common::strong::queue::id{}, message.order + 1);

                        // Make sure we prioritize local queue
                        common::algorithm::stable_sort( instances);
                     }
                  }
                  break;
               }
               case directive_type::remove:
               {
                  for( const auto& queue : message.queues)
                  {
                     auto& instances = queues[ queue.name];

                     common::algorithm::trim( instances, common::algorithm::remove_if( instances, [&]( const Queue& q){
                           return message.process.pid == q.process.pid;
                        }));

                     if( instances.empty())
                     {
                        queues.erase( queue.name);
                     }
                  }
                  break;
               }
            }
         }
         const common::message::domain::configuration::queue::Group* State::group_configuration( const std::string& name)
         {
            auto found = common::algorithm::find_if( configuration.groups, [&name]( auto& g){
               return g.name == name;
            });

            if( found)
               return found.data();

            return nullptr;
         }

      } // manager
   } // queue
} // casual

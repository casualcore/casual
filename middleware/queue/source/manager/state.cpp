//!
//! casual
//!

#include "queue/manager/state.h"
#include "queue/common/log.h"

#include "common/algorithm.h"

namespace casual
{
   namespace queue
   {
      namespace manager
      {

         State::State() = default;


         State::Group::Group() = default;
         State::Group::Group( std::string name, id_type process) : name( std::move( name)), process( std::move( process)) {}


         bool operator == ( const State::Group& lhs, State::Group::id_type process) { return lhs.process == process;}

         static_assert( common::traits::is_movable< State::Group>::value, "not movable");

         State::Remote::Remote() = default;
         State::Remote::Remote( common::domain::Identity id, common::process::Handle process)
            : id{ std::move( id)}, process{ std::move( process)} {}


         bool operator == ( const State::Remote& lhs, const common::domain::Identity& rhs)
         {
            return lhs.id == rhs;
         }

         static_assert( common::traits::is_movable< State::Remote>::value, "not movable");


         bool operator < ( const State::Queue& lhs, const State::Queue& rhs)
         {
            return lhs.order < rhs.order;
         }

         std::ostream& operator << ( std::ostream& out, const State::Queue& value)
         {
            return out << "{ process:" << value.process
                  << ", qid: " << value.queue
                  << ", order: " << value.order
                  << '}';
         }

         std::vector< common::platform::pid::type> State::processes() const
         {
            std::vector< common::platform::pid::type> result;

            for( auto& group : groups)
            {
               result.push_back( group.process.pid);
            }

            return result;
         }


         void State::remove_queues( common::platform::pid::type pid)
         {
            Trace trace{ "queue::broker::State::remove_queues"};

            common::range::erase_if( queues, [pid]( std::vector< Queue>& instances){
               return common::range::trim( instances, common::range::remove_if( instances, [pid]( const Queue& q){
                  return pid == q.process.pid;
               })).empty();
            });

            if( log)
            {
               log << "queues: [";
               for( auto& value : queues)
               {
                  log << "{ name: " << value.first
                        << ", instancse: " << common::range::make( value.second) << ", ";
               }
               log << "]\n";
            }

         }

         void State::remove( common::platform::pid::type pid)
         {
            Trace trace{ "queue::broker::State::remove"};

            remove_queues( pid);

            common::range::trim( groups, common::range::remove_if( groups, [pid]( const auto& g){
               return pid == g.process.pid;
            }));


            common::range::trim( remotes, common::range::remove_if( remotes, [pid]( const auto& g){
               return pid == g.process.pid;
            }));

         }

         void State::update( common::message::gateway::domain::Advertise& message)
         {
            Trace trace{ "queue::broker::State::update"};

            using directive_type = decltype(message.directive);

            switch( message.directive)
            {
               case directive_type::replace:
               {
                  remove_queues( message.process.pid);

                  //
                  // We fall through and add the queues, if any.
                  //
               }
               // no break
               case directive_type::add:
               {
                  if( ! message.queues.empty())
                  {
                     //
                     // We only add gateway and queues if there are any.
                     //

                     if( ! common::range::find( remotes, message.domain))
                     {
                        remotes.emplace_back( message.domain, message.process);
                     }

                     for( const auto& queue : message.queues)
                     {
                        auto& instances = queues[ queue.name];

                        //
                        // outbound order is zero-based, we add 1 to distinguish local from remote
                        //
                        instances.emplace_back( message.process, 0, message.order + 1);

                        //
                        // Make sure we prioritize local queue
                        //
                        common::range::stable_sort( instances);
                     }
                  }

                  break;
               }
               case directive_type::remove:
               {
                  for( const auto& queue : message.queues)
                  {
                     auto& instances = queues[ queue.name];

                     common::range::trim( instances, common::range::remove_if( instances, [&]( const Queue& q){
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
      } // manager
   } // queue
} // casual

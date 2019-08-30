//! 
//! Copyright (c) 2015, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!


#include "domain/manager/task.h"

#include "domain/manager/ipc.h"
#include "domain/manager/state.h"
#include "domain/common.h"



#include "common/stream.h"
#include "common/algorithm.h"

namespace casual
{
   using namespace common;

   namespace domain
   {
      namespace manager
      {
         namespace task
         {
            namespace event
            {
               bool Dispatch::active( common::message::Type type) const
               {
                  return algorithm::any_of( m_invocables, [ type]( auto& invocable){ return invocable == type;});
               }

               void Dispatch::add( task::id::type id, std::vector< Callback> callbacks)
               {
                  Trace trace{ "domain::manager::task::event::Dispatch::add"};

                  algorithm::transform( callbacks, std::back_inserter( m_invocables), [id]( auto& callback)
                  {
                     return Invocable{ id, std::move( callback)};
                  });

                  log::line( verbose::log, "invocables: ", m_invocables); 
               }

               void Dispatch::remove( task::id::type id)
               {
                  Trace trace{ "domain::manager::task::event::Dispatch::remove"};

                  algorithm::trim( m_invocables, algorithm::remove( m_invocables, id));

                  log::line( verbose::log, "invocables: ", m_invocables); 
               }

            } // event


            namespace local
            {
               namespace
               {
                  namespace property
                  {
                     auto sequential() 
                     {
                        return []( const Task& task)
                        {
                           return task.property().execution == Task::Property::Execution::sequential;
                        };
                     }

                  } // property
               } // <unnamed>
            } // local


            void done( const State& state, task::id::type id)
            {
               Trace trace{ "domain::manager::task::done"};

               log::line( verbose::log, "id: ", id);

               const auto event = []( auto id)
               {
                  common::message::event::domain::task::End result;
                  result.id = id;
                  return result;
               }( id);

               if( state.event.active< common::message::event::domain::task::End>())
                  manager::ipc::send( state, state.event( event));

               // we're interested in this event our self, to remove the task
               ipc::push( event);
            }

         } // task

         std::ostream& operator << ( std::ostream& out, Task::Property::Execution value)
         {
            using Enum = Task::Property::Execution;
            switch( value)
            {
               case Enum::concurrent: return out << "concurrent";
               case Enum::sequential: return out << "sequential";
            }
            return out << "<unknown>";
         }
         
         std::ostream& operator << ( std::ostream& out, Task::Property::Completion value)
         {
            using Enum = Task::Property::Completion;
            switch( value)
            {
               case Enum::abortable: return out << "abortable";
               case Enum::mandatory: return out << "mandatory";
            }
            return out << "<unknown>";
         }

         std::ostream& operator << ( std::ostream& out, Task::Property value)
         {
            return out << "{ execution: " << value.execution
               << ", completion: " << value.completion
               << '}';
         }

         namespace task 
         {
            task::id::type Queue::add( State& state, Task&& task)
            {
               Trace trace{ "domain::manager::task::Queue::add"};
               log::line( verbose::log, "task: ", task);

               auto sequential = local::property::sequential();

               if( ! sequential( task) || algorithm::none_of( m_running, sequential))
                  return start( state, std::move( task));

               m_pending.push_back( std::move( task));
               return m_pending.back().id();
            }

            std::vector< task::id::type> Queue::add( State& state, std::vector< Task>&& tasks)
            {
               return algorithm::transform( tasks, [&]( auto& task)
               {
                  return add( state, std::move( task));
               });
            }

            void Queue::event( State& state, const common::message::event::domain::task::End& event)
            {
               Trace trace{ "domain::manager::task::Queue::event Done"};

               m_events.remove( event.id);

               auto split = algorithm::stable_partition( m_running, [id = event.id]( auto& t){ return t != id;});
               log::line( verbose::log, "done: ", std::get< 1>( split));

               algorithm::trim( m_running, std::get< 0>( split));
               
               // are ther more task to start?
               if( m_running.empty() && ! m_pending.empty())
               {
                  start( state, std::move( m_pending.front()));
                  m_pending.pop_front();
               }
            }

            void Queue::abort()
            {
               Trace trace{ "domain::manager::task::Queue::abort"};

               auto is_mandatory = []( auto& task)
               {
                  return task.property().completion == Task::Property::Completion::mandatory;
               };

               // take care of pending
               {
                  auto split = algorithm::stable_partition( m_pending, is_mandatory);
                  log::line( verbose::log, "pending aborted: ", std::get< 1>( split));
                  algorithm::trim( m_pending, std::get< 0>( split));
               }

               // take care of running
               {
                  auto split = algorithm::partition( m_running, is_mandatory);
                  log::line( verbose::log, "running aborted: ", std::get< 1>( split));
                  algorithm::trim( m_running, std::get< 0>( split));
               }  
            }

            task::id::type Queue::start( State& state, Task&& task)
            {
               Trace trace{ "domain::manager::task::Queue::start"};

               log::line( verbose::log, "task: ", task);

               auto event = []( auto id)
               {
                  common::message::event::domain::task::Begin result;
                  result.id = id;
                  return result;
               };

               m_events.add( task.id(), task( state));

               if( state.event.active< decltype( event( task.id()))>())
                  manager::ipc::send( state, state.event( event( task.id())));

               m_running.push_back( std::move( task));
               return m_running.back().id();
            }


         } // task
      } // manager
   } // domain
} // casual

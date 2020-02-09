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

            void done( const State& state, task::id::type id, common::message::event::domain::task::State outcome)
            {
               Trace trace{ "domain::manager::task::done"};
               log::line( verbose::log, "task id: ", id, " - outcome: ", outcome);

               common::message::event::domain::task::End event;
               {
                  event.id = id;
                  event.state = outcome;
                  if( auto found = algorithm::find( state.tasks.running(), id))
                     event.description = found->description();
               }

               if( state.event.active< common::message::event::domain::task::End>())
                  manager::ipc::send( state, state.event( event));

               // we're interested in this event our self, to remove the task
               ipc::push( event);
            }

            Running::Running( State& state, Task&& task) 
               : Task{ std::move( task)}, m_callbacks{ Task::operator()( state)} 
            {
               Trace trace{ "domain::manager::task::Running::Running"};

            }; 

            bool operator == ( const Running& lhs, common::message::Type rhs) 
            { 
               return ! common::algorithm::find( lhs.m_callbacks, rhs).empty();
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
               case Enum::mandatory: return out << "mandatory";
               case Enum::removable: return out << "removable";
               case Enum::abortable: return out << "abortable";
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
            namespace local
            {
               namespace
               {
                  namespace has
                  {
                     auto completion = []( auto completion)
                     {
                        return [=]( auto& task)
                        {
                           return task.property().completion == completion;
                        };
                     };
                  } // has

                  namespace send
                  {
                     template< typename R> 
                     void abort( State& state, R&& tasks)
                     {
                        Trace trace{ "domain::manager::task::local::send::abort"};
                        log::line( verbose::log, "tasks: ", tasks);

                        if( ! state.event.active< common::message::event::domain::task::End>())
                           return;

                        auto send_task_end = [&state]( auto& task)
                        {
                           common::message::event::domain::task::End event;
                           event.id = task.id();
                           event.description = task.description();
                           event.state = decltype( event.state)::aborted;
                           manager::ipc::send( state, state.event( event));
                        };

                        algorithm::for_each( tasks, send_task_end);
                     }

                  } // send
                  
               } // <unnamed>
            } // local

            void Running::operator() ( const common::message::event::domain::task::End& event)
            {
               Trace trace{ "domain::manager::task::Running::operator() Done"};
               log::line( verbose::log, "event: ", event);

               auto split = algorithm::stable_partition( m_callbacks, [id = event.id]( auto& t){ return t != id;});
               log::line( verbose::log, "removed: ", std::get< 1>( split));

               algorithm::trim( m_callbacks, std::get< 0>( split));

               // ramaining callbacks could be interested
               Running::dispatch( event);
            }

            void Queue::idle( State& state)
            {
               Trace trace{ "domain::manager::task::Queue::idle"};

               // dispatch an 'idle-event'. Could be tasks that want to do stuff
               // when we're 'idle'
               Queue::event( message::event::Idle{});

               if( m_pending.empty())
                  return;

               if( m_running.empty() || algorithm::none_of( m_running, local::property::sequential()))
               {
                  auto task = std::move( m_pending.front());
                  m_pending.pop_front();
                  start( state, std::move( task));
               }
            }

            task::id::type Queue::add( State& state, Task&& task)
            {
               Trace trace{ "domain::manager::task::Queue::add"};
               log::line( verbose::log, "task: ", task);

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

            void Queue::event( const common::message::event::domain::task::End& event)
            {
               Trace trace{ "domain::manager::task::Queue::event Done"};
               log::line( verbose::log, "event: ", event);

               auto split = algorithm::stable_partition( m_running, [id = event.id]( auto& t){ return t != id;});
               log::line( verbose::log, "done: ", std::get< 1>( split));

               algorithm::trim( m_running, std::get< 0>( split));

               // tasks could want this event also
               common::algorithm::for_each( m_running, [&event]( auto& task)
               {
                  task( event);
               });
            }


            void Queue::abort( State& state)
            {
               Trace trace{ "domain::manager::task::Queue::abort"};

               // remove all pending, if any.
               Queue::remove( state);

               // move all that is not abortable first, en remove the complement (abortable)
               auto split = algorithm::partition( m_running, predicate::negate( local::has::completion( Task::Property::Completion::abortable)));
               local::send::abort( state, std::get< 1>( split));
               algorithm::trim( m_running, std::get< 0>( split));
            
            }

            void Queue::remove( State& state)
            {
               Trace trace{ "domain::manager::task::Queue::remove"};

               // move all mandatory first, and remove the complement (not mandatory)
               auto split = algorithm::stable_partition( m_pending, local::has::completion( Task::Property::Completion::mandatory));
               
               local::send::abort( state, std::get< 1>( split));
               algorithm::trim( m_pending, std::get< 0>( split));
            }

            bool Queue::active( common::message::Type type) const
            {
               return ! algorithm::find( m_running, type).empty();
            }

            task::id::type Queue::start( State& state, Task&& task)
            {
               Trace trace{ "domain::manager::task::Queue::start"};
               log::line( verbose::log, "task: ", task);

               using Begin = common::message::event::domain::task::Begin;

               if( state.event.active< Begin>())
               {
                  Begin event;
                  event.id = task.id();
                  event.description = task.description();

                  manager::ipc::send( state, state.event( event));
               }

               task::Running running{ state, std::move( task)};
               log::line( verbose::log, "running: ", running);

               m_running.push_back( std::move( running));
               return m_running.back().id();
            }

         } // task
      } // manager
   } // domain
} // casual

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

            Running::Running( State& state, Task&& task) 
               : Task{ std::move( task)}, m_callbacks( Task::operator()( state))
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

                        if( ! state.event.active< common::message::event::Task>())
                           return;

                        auto send_task_end = [&state]( auto& task)
                        {
                           common::message::event::Task event{ common::process::handle()};
                           event.correlation = task.context().id;
                           event.description = task.context().descripton;
                           event.state = decltype( event.state)::aborted;
                           manager::ipc::send( state, state.event( event));
                        };

                        algorithm::for_each( tasks, send_task_end);
                     }

                     template< typename T> 
                     void done( State& state, T& task)
                     {
                        Trace trace{ "domain::manager::task::local::send::done"};

                        // send event to listeners, if any.
                        using Task = common::message::event::Task;

                        if( state.event.active< Task>())
                        {
                           Task event{ common::process::handle()};
                           event.correlation = task.context().id;
                           event.description = std::move( task.context().descripton);
                           event.state = decltype( event.state)::done;

                           manager::ipc::send( state, state.event( event));
                        }

                     }

                  } // send
                  
               } // <unnamed>
            } // local


            void Queue::idle( State& state)
            {
               Trace trace{ "domain::manager::task::Queue::idle"};

               // dispatch an 'idle-event'. Could be tasks that want to do stuff
               // when we're 'idle'
               Queue::event( state, common::message::event::Idle{});

               if( m_pending.empty())
                  return;

               if( m_running.empty() || algorithm::none_of( m_running, local::property::sequential()))
               {
                  auto task = std::move( m_pending.front());
                  m_pending.pop_front();
                  start( state, std::move( task));
               }
            }

            task::id::type Queue::add( Task&& task)
            {
               Trace trace{ "domain::manager::task::Queue::add"};
               log::line( verbose::log, "task: ", task);

               m_pending.push_back( std::move( task));
               return m_pending.back().context().id;
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
               auto split = algorithm::stable::partition( m_pending, local::has::completion( Task::Property::Completion::mandatory));
               
               local::send::abort( state, std::get< 1>( split));
               algorithm::trim( m_pending, std::get< 0>( split));
            }

            bool Queue::active( common::message::Type type) const
            {
               return ! algorithm::find( m_running, type).empty();
            }

            void Queue::start( State& state, Task&& task)
            {
               Trace trace{ "domain::manager::task::Queue::start"};
               log::line( verbose::log, "task: ", task);

               using Task = common::message::event::Task;
               
               // send the 'start' event
               if( state.event.active< Task>())
               {
                  Task event{ common::process::handle()};
                  event.correlation = task.context().id;
                  event.description = task.context().descripton;
                  event.state = decltype( event.state)::started;

                  manager::ipc::send( state, state.event( event));
               }

               task::Running running{ state, std::move( task)};
               log::line( verbose::log, "running: ", running);
               
               // check if running can do progress, if not, it's 'done'
               if( running.empty())
                  local::send::done( state, running);
               else
                  m_running.push_back( std::move( running));
            }

            void Queue::done( State& state, Task&& task)
            {
               Trace trace{ "domain::manager::task::Queue::done"};
               log::line( verbose::log, "task: ", task);

               local::send::done( state, task);
            }

         } // task
      } // manager
   } // domain
} // casual

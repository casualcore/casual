//!
//! Copyright (c) 2019, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!

#include "domain/manager/task/create.h"
#include "domain/manager/task/event.h"

#include "domain/manager/handle.h"
#include "domain/common.h"

#include "common/algorithm/compare.h"
#include "common/log/stream.h"

namespace casual
{
   using namespace common;

   namespace domain
   {
      namespace manager
      {
         namespace task
         {
            namespace create
            {
               namespace local
               {
                  namespace
                  {
                     namespace detail
                     {
                        template< typename C> 
                        void back( C& container, task::id::type) {}

                        template< typename C, typename T, typename... Ts> 
                        void back( C& container, task::id::type id, T&& t, Ts&&... ts)
                        {
                           container.emplace_back( id, std::forward< T>( t)); 
                           back( container, id, std::forward< Ts>( ts)...);
                        }
                     } // detail

                     template< typename... Ts>
                     auto callbacks( task::id::type id, Ts&&... ts)
                     {
                        std::vector< manager::task::event::Callback> result;
                        detail::back( result, id, std::forward< Ts>( ts)...);
                        return result;
                     }
                  } // <unnamed>
               } // local
               namespace restart
               {
                  manager::Task server( State& state, state::Server::id_type id)
                  {
                     Trace trace{ "domain::manager::task::create::restart::server"};
                     auto& server = state.entity( id);

                     log::line( verbose::log, "server: ", server);

                     // we rely on the restart functionality, but we need to reset to the 
                     // origin when done
                     const auto restart = std::exchange( server.restart, true);

                     auto handles = algorithm::transform_if( 
                        server.instances, 
                        []( auto& i){ return i.handle;},
                        []( auto& i){ return i.state == state::Server::state_type::running;});

                     auto task = [ server_id = id, restart, handles = std::move( handles)]( State& state, manager::task::id::type id) mutable 
                        -> std::vector< manager::task::event::Callback>
                     {
                        Trace trace{ "domain::manager::handle::restart::local::task::server Task"};
                        log::line( verbose::log, "task: ", id, "handles: ", handles, ", server-id: ", server_id, ", restart: ", restart);

                        //auto& server = state.entity( server_id);
                        //algorithm::trim( handles, std::get< 0>( algorithm::intersection( handles, server.instances)));

                        if( handles.empty())
                           return {};

                        // shutdown the last one, we do this in reverse order.
                        handle::scale::shutdown( state, { handles.back()});

                        // create the callback
                        return local::callbacks( id,
                           [ &handles]( const common::message::event::process::Exit& message)
                           {
                              Trace trace{ "domain::manager::handle::restart::local::task::server process::Exit"};
                              log::line( verbose::log, "message: ", message);

                              // we 'tag' the ipc to _empty_ so we can correlate when we get the server connect.
                              // note: even if the calback below is not the one who has triggered the shutdown,
                              //  we still handle the events, and it will work.
                              if( auto found = algorithm::find( handles, message.state.pid))
                                 found->ipc = strong::ipc::id{};

                              log::line( verbose::log, "handles: ", handles);

                           },
                           [ task_id = id, server_id, restart, &state, &handles]( const common::message::event::domain::server::Connect& message)
                           {
                              Trace trace{ "domain::manager::handle::restart::local::task::server server::Connect"};
                              log::line( verbose::log, "message: ", message);

                              auto& server = state.entity( server_id);

                              if( ! algorithm::find( server.instances, message.process.pid))
                                 return;

                              // a server instance that we're interseted in has connected, remove the tagged ones (one)
                              // and scale "down" another one, if any.

                              auto has_tag = []( auto& handle){ return handle.ipc.empty();};
                              
                              algorithm::trim( handles, algorithm::remove_if( handles, has_tag));

                              if( handles.empty())
                              {
                                 // reset the restart
                                 state.entity( server_id).restart = restart;
                                 manager::task::done( state, task_id);
                              }
                              else 
                                 handle::scale::shutdown( state, { handles.back()});
                           });
                     };

                     // this task is concurrent-abortable
                     return manager::Task{ string::compose( "restart server: ", server.alias), std::move( task), 
                        {
                           Task::Property::Execution::concurrent,
                           Task::Property::Completion::abortable
                        }};
                  }

                  manager::Task executable( State& state, state::Executable::id_type id)
                  {
                     Trace trace{ "domain::manager::task::create::restart::executable"};

                     auto& executable = state.entity( id);
                     log::line( verbose::log, "executable: ", executable);

                     // we rely on the restart functionality, but we need to reset to the 
                     // origin when done
                     const auto restart = std::exchange( executable.restart, true);

                     auto pids = algorithm::transform_if( 
                        executable.instances, 
                        []( auto& i){ return i.handle;},
                        []( auto& i){ return i.state == state::Server::state_type::running;});

                     auto task = [executable_id = id, restart = restart, pids = std::move( pids)]( State& state, manager::task::id::type id) mutable
                         -> std::vector< manager::task::event::Callback>
                     {
                        Trace trace{ "domain::manager::task::create::restart::executable start"};
                        log::line( verbose::log, "task: ", id, ", handles: ", pids, ", executable-id: ", executable_id, ", restart: ", restart);

                        auto& executable = state.entity( executable_id);
                        algorithm::trim( pids, std::get< 0>( algorithm::intersection( pids, executable.instances)));

                        if( pids.empty())
                           return {};

                        // start the termination (back to front).
                        common::process::terminate( pids.back());

                        // create the callback
                        return local::callbacks( id,
                           [ restart, task_id = id, executable_id, &state, &pids]( const common::message::event::process::Exit& message)
                           {
                              Trace trace{ "domain::manager::task::create::restart::executable process::Exit"};
                              log::line( verbose::log, "message: ", message);

                              if( auto found = algorithm::find( pids, message.state.pid))
                              {
                                 log::line( verbose::log, "found: ", *found);
                                 pids.erase( std::begin( found));

                                 // terminate the next one.
                                 common::process::terminate( pids.back());
                              }

                              // are we done?
                              if( pids.empty())
                              {
                                 // reset the restart
                                 state.entity( executable_id).restart = restart;
                                 manager::task::done( state, task_id);
                              }
                           });
                     };

                     // this task is concurrent-abortable
                     return manager::Task{ string::compose( "restart executable: ", executable.alias), std::move( task),
                        {
                           Task::Property::Execution::concurrent,
                           Task::Property::Completion::abortable
                        }};
                  }
               } // restart


               namespace scale
               {
                  namespace local
                  {
                     namespace
                     {
                        template< typename ID>
                        auto task( ID id)
                        {
                           return [entity_id = id]( State& state, manager::task::id::type id) mutable 
                              -> std::vector< manager::task::event::Callback>
                           {
                              // start
                              {
                                 Trace trace{ "domain::manager::task::create::scale::local::task start"};

                                 auto& entity = state.entity( entity_id);
                                 log::line( verbose::log, "entity: ", entity);

                                 // scale it
                                 handle::scale::instances( state, entity);
                              };

                              // done when no instances are either scaling out or in
                              auto is_done = [entity_id]( State& state)
                              {
                                 auto& entity = state.entity( entity_id);

                                 return algorithm::none_of( entity.instances, []( auto& i)
                                 {
                                    using Enum = state::Server::state_type;
                                    return algorithm::compare::any( i.state, Enum::scale_out, Enum::scale_in, Enum::spawned);
                                 });
                              };

                              
                              // we could be done directly
                              if( is_done( state))
                              {
                                 manager::task::done( state, id);
                                 return {};
                              }

                              auto check_done = [id, is_done, &state]()
                              {
                                 if( is_done( state))
                                    manager::task::done( state, id);
                              };

                              
                              return create::local::callbacks( id,
                                 [ check_done]( const common::message::event::process::Exit& message)
                                 {
                                    log::line( verbose::log, "process exit: ", message);
                                    check_done();
                                 },
                                 [ check_done]( const common::message::event::process::Spawn& message)
                                 {
                                    log::line( verbose::log, "process spawn: ", message);
                                    check_done();
                                 },
                                 [ check_done]( const message::event::domain::server::Connect& message)
                                 {
                                    log::line( verbose::log, "server connect: ", message);
                                    check_done();
                                 },
                                 [ check_done]( const message::event::Idle& message)
                                 {
                                    log::line( verbose::log, "idle: ", message);
                                    check_done();
                                 }
                              );
                           };

                        }
                     } // <unnamed>
                  } // local

                  manager::Task server( const State& state, state::Server::id_type id)
                  {
                     Trace trace{ "domain::manager::task::create::scale::server"};
                  
                     // this task is concurrent-abortable
                     return manager::Task{ string::compose( "scale server: ", state.entity( id).alias), local::task( id),
                     {
                        Task::Property::Execution::concurrent,
                        Task::Property::Completion::abortable
                     }};
                  }

                  manager::Task executable( const State& state, state::Executable::id_type id)
                  {
                     Trace trace{ "domain::manager::task::create::scale::executable"};

                     // this task is concurrent-abortable
                     return manager::Task{ string::compose( "scale executable: ", state.entity( id).alias), local::task( id),
                     {
                        Task::Property::Execution::concurrent,
                        Task::Property::Completion::abortable
                     }};
                  }

                  std::vector< manager::Task> dependency( const State& state, std::vector< state::dependency::Group> groups)
                  {
                     Trace trace{ "domain::manager::task::create::dependency::scale"};

                     auto transform_group = [&state]( auto& group)
                     {
                        auto result = algorithm::transform( group.servers, [&state]( auto id)
                        {
                           return task::create::scale::server( state, id);
                        });

                        algorithm::transform( group.executables, result, [&state]( auto id)
                        {
                           return task::create::scale::executable( state, id);
                        });

                        return task::create::group( std::move( group.description), std::move( result));
                     };

                     return algorithm::transform( groups, transform_group);

                  }

               } // scale

               manager::Task group( std::string description, std::vector< manager::Task>&& tasks)
               {
                  Trace trace{ "domain::manager::task::create::group"};
                  log::line( verbose::log, "description: ", description, ", tasks: ", tasks);

                  auto restricted_completion = []( auto completion, auto& task)
                  {
                     return std::min( completion, task.property().completion);
                  };
               
                  auto completion = algorithm::accumulate( tasks, Task::Property::Completion::abortable, restricted_completion);

                  auto task = [ tasks = std::move( tasks)]( State& state, manager::task::id::type id) mutable 
                     -> std::vector< manager::task::event::Callback>
                  {
                     auto result = local::callbacks( id,
                        [&tasks, &state, id]( const common::message::event::domain::task::End& message)
                        {
                           Trace trace{ "domain::manager::task::create::group callback task::End"};
                           log::line( verbose::log, "message: ", message);

                           algorithm::trim( tasks, algorithm::remove( tasks, message.id));
                           log::line( verbose::log, "tasks: ", tasks);

                           if( tasks.empty())
                              manager::task::done( state, id);
                          
                        }
                     );

                     auto append_callbacks = [&result, &state]( auto& task)
                     {
                        auto callbacks = task( state);
                        algorithm::move( callbacks, std::back_inserter( result));
                     };

                     algorithm::for_each( tasks, append_callbacks);

                     return result;
                  };

                  // create the group of tasks with sequential execution and the most restricted 
                  // completion
                  return manager::Task{ std::move( description), std::move( task),
                  {
                     Task::Property::Execution::sequential,
                     completion
                  }};


               }

               namespace done
               {                    
                  manager::Task boot()
                  {
                     Trace trace{ "domain::manager::task::create::done::boot"};

                     auto task = []( State& state, manager::task::id::type id) 
                        -> std::vector< manager::task::event::Callback>
                     {
                        Trace trace{ "domain::manager::task::create::done::boot task invoked"};

                        state.runlevel( State::Runlevel::running);

                        manager::task::done( state, id);

                        // we might have listeners to this event
                        task::event::dispatch( state, [&]()
                        {
                           message::event::domain::boot::End event;
                           event.domain = common::domain::identity();
                           event.process = common::process::handle();
                           return event;
                        });

                        return {};

                     };
                     return manager::Task{ "boot done", std::move( task),                        
                     {
                        Task::Property::Execution::sequential,
                        Task::Property::Completion::removable
                     }};
                  }

                  manager::Task shutdown()
                  {
                     Trace trace{ "domain::manager::task::create::done::shutdown"};

                     auto task = []( State& state, manager::task::id::type id) -> std::vector< manager::task::event::Callback>
                     {
                        Trace trace{ "domain::manager::task::create::done::shutdown task invoked"};

                        state.runlevel( State::Runlevel::shutdown);

                        manager::task::done( state, id);

                        // we might have listeners to this event
                        task::event::dispatch( state, [&]()
                        {
                           message::event::domain::shutdown::End event;
                           event.domain = common::domain::identity();
                           event.process = common::process::handle();
                           return event;
                        });

                        return {};

                     };
                     return manager::Task{ "shutdown done", std::move( task),                        
                        {
                           Task::Property::Execution::sequential,
                           Task::Property::Completion::mandatory
                        }};
                  }
                  
               } // done

               manager::Task shutdown()
               {
                  Trace trace{ "domain::manager::task::create::shutdown"};

                  auto task = []( State& state, manager::task::id::type id) -> std::vector< manager::task::event::Callback>
                  {
                     Trace trace{ "domain::manager::task::create::shutdown task invoked"};

                     handle::shutdown( state);
                     manager::task::done( state, id);

                     return {};
                  };

                  return manager::Task{ "shutdown trigger", std::move( task),
                     {
                        Task::Property::Execution::sequential,
                        Task::Property::Completion::mandatory
                     }};
               }

            } // create
         } // task
      } // manager
   } // domain
} // casual
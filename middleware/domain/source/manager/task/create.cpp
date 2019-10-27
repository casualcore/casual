//!
//! Copyright (c) 2019, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!

#include "domain/manager/task/create.h"
#include "domain/manager/task/event.h"

#include "domain/manager/handle.h"
#include "domain/common.h"


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
               namespace restart
               {
                  manager::Task server( State& state, state::Server::id_type id)
                  {
                     Trace trace{ "domain::manager::task::create::restart::server"};
                     auto& server = state.server( id);

                     log::line( verbose::log, "server: ", server);

                     // we rely on the restart functionality, but we need to reset to the 
                     // origin when done
                     const auto restart = std::exchange( server.restart, true);

                     auto handles = algorithm::transform_if( 
                        server.instances, 
                        []( auto& i){ return i.handle;},
                        []( auto& i){ return i.state == state::Server::state_type::running;});

                     auto task = [server_id = id, restart = restart, handles = std::move( handles)]( State& state, manager::task::id::type id) mutable -> std::vector< manager::task::event::Callback>
                     {
                        Trace trace{ "domain::manager::handle::restart::local::task::server Task"};

                        log::line( verbose::log, "task: ", id, "handles: ", handles, ", server-id: ", server_id, ", restart: ", restart);

                        auto is_done = [id, server_id, restart]( auto& state, auto& handles)
                        {
                           if( handles.empty())
                           {
                              // reset the restart
                              state.server( server_id).restart = restart;
                              manager::task::done( state, id);
                              return true;
                           }
                           return false;
                        };

                        if( ! handles.empty())
                           handle::scale::shutdown( state, { handles.back()});

                        // we could be done directly
                        if( is_done( state, handles))
                           return {};

                        // create the callback
                        return algorithm::container::emplace::initialize< std::vector< manager::task::event::Callback>>(
                           [ is_done, &state, &handles]( const common::message::event::process::Exit& message)
                           {
                              Trace trace{ "domain::manager::handle::restart::local::task::server callback"};

                              log::line( verbose::log, "message: ", message);

                              auto split = algorithm::partition( handles, [&message]( auto& h){ return h.pid != message.state.pid;});
                              algorithm::trim( handles, std::get< 0>( split));

                              log::line( verbose::log, "split: ", split);

                              if( is_done( state, handles))
                                 return;
                              
                              // if we removed some (one) we shutdown the next one.
                              if( ! std::get< 1>( split).empty())
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

                     auto& executable = state.executable( id);

                     log::line( verbose::log, "executable: ", executable);

                     // we rely on the restart functionality, but we need to reset to the 
                     // origin when done
                     const auto restart = std::exchange( executable.restart, true);

                     auto pids = algorithm::transform_if( 
                        executable.instances, 
                        []( auto& i){ return i.handle;},
                        []( auto& i){ return i.state == state::Server::state_type::running;});

                     auto task = [executable_id = id, restart = restart, pids = std::move( pids)]( State& state, manager::task::id::type id) mutable -> std::vector< manager::task::event::Callback>
                     {
                        Trace trace{ "domain::manager::handle::restart::local::task::executable Task"};

                        log::line( verbose::log, "task: ", id, "handles: ", pids, ", executable-id: ", executable_id, ", restart: ", restart);

                        auto is_done = [id, executable_id, restart]( auto& state, auto& pids)
                        {
                           if( pids.empty())
                           {
                              // reset the restart
                              state.executable( executable_id).restart = restart;
                              manager::task::done( state, id);
                              return true;
                           }
                           return false;
                        };

                        if( ! pids.empty())
                           common::process::terminate( pids.back());

                        // create the callback
                        return algorithm::container::emplace::initialize< std::vector< manager::task::event::Callback>>(
                           [ is_done, &state, &pids]( const common::message::event::process::Exit& message)
                           {
                              Trace trace{ "domain::manager::handle::restart::local::task::executable callback"};

                              log::line( verbose::log, "message: ", message);

                              auto split = algorithm::partition( pids, [&message]( auto& pid){ return pid != message.state.pid;});
                              algorithm::trim( pids, std::get< 0>( split));

                              log::line( verbose::log, "split: ", split);

                              if( is_done( state, pids))
                                 return;
                              
                              // if we removed some (one) we terminate the next one.
                              if( ! std::get< 1>( split).empty())
                                 common::process::terminate( pids.back());
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

               namespace batch
               {
                  manager::Task boot( state::Batch batch)
                  {
                     Trace trace{ "domain::manager::task::create::batch::boot"};

                     auto task = [batch = std::move( batch)]( State& state, manager::task::id::type id) -> std::vector< manager::task::event::Callback>
                     {
                        Trace trace{ "domain::manager::task::create::batch::boot task invoked"};

                        // we can capture batch and state by reference since
                        // the returned lambda is guaranteed to outlast the callbacks
                        auto is_done = [&state, id, &batch]()
                        {
                           auto check_done = []( State& state, const state::Batch& batch)
                           {  
                              return algorithm::all_of( batch.servers, [&]( auto id){
                                 auto& server = state.server( id);
                                 return algorithm::none_of( server.instances, []( auto& i){
                                    return i.state == state::Server::state_type::scale_out;
                                 });
                              }) && algorithm::all_of( batch.executables, [&]( auto id){
                                 auto& server = state.executable( id);
                                 return algorithm::none_of( server.instances, []( auto& i){
                                    return i.state == state::Server::state_type::scale_out;
                                 });
                              });
                           };

                           if( check_done( state, batch))
                           {
                              log::line( log, "task done: boot batch: ", batch);
                              manager::task::done( state, id);

                              // we might have listeners to this event
                              task::event::dispatch( state, [&]()
                              {
                                 common::message::event::domain::Group event;
                                 event.context = common::message::event::domain::Group::Context::boot_end;
                                 event.id = batch.group.value();
                                 event.name = state.group( batch.group).name;
                                 event.process = common::process::handle();
                                 return event;
                              });

                              return true;
                           }
                           log::line( log, "task NOT done: boot batch: ", batch);
                           return false;
                        };

                        // start the scale out

                        algorithm::for_each( batch.executables, [&]( auto id){
                           handle::scale::instances( state, state.executable( id));
                        });

                        algorithm::for_each( batch.servers, [&]( auto id){
                           handle::scale::instances( state, state.server( id));
                        });

                        // we might have listeners to this event
                        task::event::dispatch( state, [&]()
                        {
                           common::message::event::domain::Group event;
                           event.context = common::message::event::domain::Group::Context::boot_start;
                           event.id = batch.group.value();
                           event.name = state.group( batch.group).name;
                           event.process = common::process::handle();
                           return event;
                        });

                        // we might be done directly
                        if( is_done())
                           return {};

                        // set up the callbacks, we capture is_done by value (which has captured
                        // state and batch by reference)
                        return algorithm::container::emplace::initialize< std::vector< manager::task::event::Callback>>(
                           [ is_done]( const common::message::event::process::Exit& message)
                           {
                              is_done();
                           },
                           [ is_done]( const common::message::event::process::Spawn& message)
                           {
                              is_done();
                           },
                           [ is_done]( const message::event::domain::server::Connect& message)
                           {
                              is_done();
                           }
                        );
                     };

                     // this task sequential-removable
                     return manager::Task{ string::compose( "boot group ", batch.group), std::move( task),                        
                        {
                           Task::Property::Execution::sequential,
                           Task::Property::Completion::removable
                        }};
                  }

                  std::vector< manager::Task> boot( std::vector< state::Batch> batch)
                  {
                     return algorithm::transform( batch, []( auto& batch){ return batch::boot( std::move( batch));});
                  }

                  manager::Task shutdown( state::Batch batch)
                  {
                     Trace trace{ "domain::manager::task::create::batch::shutdown"};

                     auto task = [batch = std::move( batch)]( State& state, manager::task::id::type id) -> std::vector< manager::task::event::Callback>
                     {
                        Trace trace{ "domain::manager::task::create::batch::shutdown task invoked"};

                        // we can capture batch and state by reference since
                        // the returned lambda is guaranteed to outlast the callbacks
                        auto is_done = [&state, id, &batch]()
                        {
                           auto check_done = []( State& state, const state::Batch& batch)
                           {  
                              return algorithm::all_of( batch.servers, [&]( auto id){
                                 auto& server = state.server( id);
                                 return algorithm::all_of( server.instances, []( auto& i){
                                    return ! i.handle.pid;
                                 });
                              }) && algorithm::all_of( batch.executables, [&]( auto id){
                                 auto& server = state.executable( id);
                                 return algorithm::all_of( server.instances, []( auto& i){
                                    return ! i.handle;
                                 });
                              });
                           };

                           if( check_done( state, batch))
                           {
                              log::line( log, "task done: shutdown batch: ", batch);

                              manager::task::done( state, id);

                              // we might have listeners to this event
                              task::event::dispatch( state, [&]()
                              {
                                 common::message::event::domain::Group event;
                                 event.context = common::message::event::domain::Group::Context::shutdown_end;
                                 event.id = batch.group.value();
                                 event.name = state.group( batch.group).name;
                                 event.process = common::process::handle();
                                 return event;
                              });

                              return true;
                           }
                           log::line( log, "task NOT done: shutdown batch: ", batch);
                           return false;
                        };
                        
                        // start scaling in

                        algorithm::for_each( batch.executables, [&]( auto id){
                           state::Executable& e = state.executable( id);
                           e.scale( 0);
                           handle::scale::instances( state, e);
                        });

                        algorithm::for_each( batch.servers, [&]( auto id){
                           state::Server& s = state.server( id);
                           s.scale( 0);
                           handle::scale::instances( state, s);
                        });

                        task::event::dispatch( state, [&]()
                        {
                           common::message::event::domain::Group event;
                           event.context = common::message::event::domain::Group::Context::shutdown_start;
                           event.id = batch.group.value();
                           event.name = state.group( batch.group).name;
                           event.process = common::process::handle();
                           return event;
                        });

                        // we might be done directly
                        if( is_done())
                           return {};

                        // set up the callbacks, we capture is_done by value (which has captured
                        // state and batch by reference)
                        return algorithm::container::emplace::initialize< std::vector< manager::task::event::Callback>>(
                           [ is_done]( const common::message::event::process::Exit& message)
                           {
                              is_done();
                           });
                     };

                     // this task can not be aborted
                     return manager::Task{ string::compose( "shutdown group ", batch.group), std::move( task),                        
                        {
                           Task::Property::Execution::sequential,
                           Task::Property::Completion::mandatory
                        }};
                  }

                  std::vector< manager::Task> shutdown( std::vector< state::Batch> batch)
                  {
                     return algorithm::transform( batch, []( auto& batch){ return batch::shutdown( std::move( batch));});
                  }
                  
               } // batch


               namespace done
               {                    
                  manager::Task boot()
                  {
                     Trace trace{ "domain::manager::task::create::done::boot"};

                     auto task = []( State& state, manager::task::id::type id) -> std::vector< manager::task::event::Callback>
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
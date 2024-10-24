//! 
//! Copyright (c) 2015, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!


#include "domain/manager/handle.h"

#include "domain/manager/state/order.h"
#include "domain/manager/admin/server.h"
#include "domain/manager/task/create.h"
#include "domain/manager/task/event.h"
#include "domain/manager/transform.h"
#include "domain/common.h"

#include "configuration/message.h"

#include "common/instance.h"
#include "common/communication/instance.h" 

#include "common/message/dispatch/handle.h"
#include "common/server/handle/call.h"
#include "common/environment.h"
#include "common/algorithm/compare.h"
#include "common/algorithm.h"
#include "common/string/compose.h"

namespace casual
{
   using namespace common;

   namespace domain::manager::handle
   {

      namespace local
      {
         namespace
         {
            namespace scale
            {
               template< typename E, typename V> 
               auto spawn( const E& executable, const instance::Information& instance, V variables) // note: copy of variables
               {
                  // add casual instance information for the process.
                  variables.push_back( instance::variable( instance));

                  return common::process::spawn( executable.path, executable.arguments, std::move( variables));
               }
               

               template< typename E>
               void out( State& state, E& entity)
               {
                  Trace trace{ "domain::manager::handle::local::scale::out"};

                  auto spawnable = entity.spawnable();

                  if( ! spawnable)
                     return;

                  auto variables = state.variables( entity);

                  instance::Information instance;
                  instance.alias = entity.alias;

                  // we set error state now in case the spawn fails
                  //algorithm::for_each( spawnable, []( auto& entity){ entity.state = decltype( entity.state)::error;});

                  // temporary to enable increment
                  auto range = spawnable;
                  
                  while( range)
                  {
                     instance.index = std::distance( std::begin( entity.instances), std::begin( range));

                     try 
                     {
                        range->spawned( scale::spawn( entity, instance, variables));
                     }
                     catch( ...)
                     {
                        auto error = exception::capture();
                        log::error( error, "failed to spawn: ", entity.path);

                        range->wanted = state::instance::Phase::error;

                        manager::task::event::dispatch( state, [&]()
                        {
                           common::message::event::Error event{ common::process::handle()};
                           event.code = error.code();
                           event.severity = decltype( event.severity)::error;
                           event.message = string::compose( "failed to spawn: ", entity.path);
                           return event;
                        });
                     }
                     
                     ++range;
                  }

                  // we push spawn 'event' and dispatch it to tasks and external listeners 
                  // later on
                  {
                     common::message::event::process::Spawn message{ common::process::handle()};
                     message.path = entity.path;
                     message.alias = entity.alias;

                     algorithm::transform_if( spawnable, message.pids, 
                     []( auto& instance){ return common::process::id( instance.handle);},
                     []( auto& instance){ return instance.state() != state::instance::State::error;});

                     communication::ipc::inbound::device().push( std::move( message));
                  }

                  log::line( verbose::log, "entity: ", entity);
               }

               void in( const State& state, const state::Executable& executable)
               {
                  Trace trace{ "domain::manager::handle::scale::in Executable"};

                  if( auto shutdownable = executable.shutdownable())
                  {
                     // We only want child signals
                     signal::thread::scope::Mask mask{ signal::set::filled( code::signal::child)};

                     auto pids = algorithm::transform( range::reverse( shutdownable), []( const auto& i)
                     {
                        return i.handle;
                     });

                     common::process::terminate( pids);
                  }
               }

               void in( State& state, const state::Server& server)
               {
                  Trace trace{ "domain::manager::handle::local::scale::in Server"};

                  if( auto shutdownable = server.shutdownable())
                  {
                     handle::scale::shutdown( state, algorithm::transform( range::reverse( shutdownable), []( const auto& i)
                     {
                        return i.handle;
                     }));
                  }
               }

            } // scale


         } // <unnamed>
      } // local


      namespace mandatory
      {
         namespace boot
         {
            namespace core
            {
               void prepare( State& state)
               {
                  Trace trace{ "domain::manager::handle::mandatory::boot::core::prepare"};

                  {
                     auto server = state::Server::create();
                     server.alias = "casual-domain-discovery";
                     server.path = common::process::path().parent_path() / "casual-domain-discovery";
                     server.scale( 1);
                     server.memberships.push_back( state.group_id.core);
                     server.note = "handles discovery from/to other domains";
                     server.restart = true;

                     state.servers.push_back( std::move( server));
                  }

                  log::line( verbose::log, "state.servers: ", state.servers);

               }
            }

            void prepare( State& state)
            {
               Trace trace{ "domain::manager::handle::mandatory::boot::prepare"};

               {
                  auto manager = state::Server::create();
                  manager.alias = "casual-service-manager";
                  manager.path = common::process::path().parent_path() / "casual-service-manager";
                  manager.scale( 1);
                  manager.memberships.push_back( state.group_id.master);
                  manager.note = "service lookup and management";
                  manager.restart = true;

                  state.servers.push_back( std::move( manager));
               }

               {
                  auto tm = state::Server::create();
                  tm.alias = "casual-transaction-manager";
                  tm.path = common::process::path().parent_path() / "casual-transaction-manager";
                  tm.scale( 1);
                  tm.memberships.push_back( state.group_id.transaction);
                  tm.note = "manage transaction in this domain";
                  tm.restart = true;

                  state.servers.push_back( std::move( tm));
               }

               {
                  auto queue = state::Server::create();
                  queue.alias = "casual-queue-manager";
                  queue.path = common::process::path().parent_path() / "casual-queue-manager";
                  queue.scale( 1);
                  queue.memberships.push_back( state.group_id.queue);
                  queue.note = "manage queues in this domain";
                  queue.restart = true;

                  state.servers.push_back( std::move( queue));
               }

               {
                  auto gateway = state::Server::create();
                  gateway.alias = "casual-gateway-manager";
                  gateway.path = common::process::path().parent_path() / "casual-gateway-manager";
                  gateway.scale( 1);
                  gateway.memberships.push_back( state.group_id.gateway);
                  gateway.note = "manage connections to and from other domains";
                  gateway.restart = true;

                  state.servers.push_back( std::move( gateway));
               }
            }
         } // boot
      } // mandatory


      void boot( State& state, common::strong::correlation::id correlation)
      {
         Trace trace{ "domain::manager::handle::boot"};

         manager::task::event::dispatch( state, [ &correlation]()
         {
            task::message::domain::Information event{ common::process::handle()};
            event.correlation = correlation;
            event.domain = common::domain::identity();
            return event;
         });

         // we need to send task event with the supplied correlation -> caller able to correlate
         manager::task::event::dispatch( state, [ &correlation]()
         {
               common::message::event::Task event{ common::process::handle()};
               event.correlation = correlation;
               event.description = "boot domain";
               event.state = decltype( event.state)::started;
               return event;
         });

         // make sure we sett runlevel when we're done.
         auto done_callback = [ &state, correlation]( casual::task::unit::id)
         {
            state.runlevel = decltype( state.runlevel())::running;

            // we ignore the actual id of the task unit and use the supplied correlation -> caller able to correlate
            manager::task::event::dispatch( state, [ &correlation]()
            {
                  common::message::event::Task event{ common::process::handle()};
                  event.correlation = correlation;
                  event.description = "boot domain";
                  event.state = decltype( event.state)::done;
                  return event;
            });
            
            return casual::task::unit::action::Outcome::abort;
         };

         state.tasks.then( manager::task::create::scale::groups( state, state::order::boot( state)))
            .then( casual::task::create::action( std::move( done_callback)));
      }

      std::vector< common::strong::correlation::id> shutdown( State& state)
      {
         Trace trace{ "domain::manager::handle::shutdown"};

         state.runlevel = decltype( state.runlevel())::shutdown;

         // abort all abortable running or pending task
         state.tasks.cancel();


         auto done_event = task::create::event::parent( state, string::compose( "shutdown domain: ", common::domain::identity()));

         // prepare scaling to 0
         auto prepare_shutdown = []( auto& entity)
         {
            entity.scale( 0);
         };

         algorithm::for_each( state.executables, prepare_shutdown);
         // Make sure we exclude our self so we don't try to shutdown
         algorithm::for_each_if( state.servers, prepare_shutdown, [&state]( auto& entity){ return entity.id != state.manager_id;});

         auto groups = manager::task::create::scale::groups( state, state::order::shutdown( state));
         auto result = casual::task::ids( groups, done_event);
         state.tasks.then( std::move( groups)).then( std::move( done_event));

         return result;
      }


      namespace scale
      {

         std::vector< common::strong::correlation::id> aliases( State& state, std::vector< admin::model::scale::Alias> aliases)
         {
            Trace trace{ "domain::manager::handle::scale::aliases"};
            log::line( verbose::log, "aliases: ", aliases);

            auto done_event = task::create::event::parent( state, "scale aliases");

            auto scalables = state.scalables( algorithm::transform( aliases, []( auto& alias){ return alias.name;}));

            // prepare the scaling
            {                  
               auto scale_runnable = [&aliases]( auto& runnable)
               {
                  auto is_alias = [&runnable]( auto& alias){ return alias.name == runnable.get().alias;};

                  if( auto found = algorithm::find_if( aliases, is_alias))
                     runnable.get().scale( found->instances);
               };

               algorithm::for_each( scalables.servers, scale_runnable);
               algorithm::for_each( scalables.executables, scale_runnable);
            }

            auto transform_id = []( auto& entity){ return entity.get().id;};

            state::dependency::Group group;
            {
               group.description = "scale aliases";
               algorithm::transform( scalables.servers, group.servers, transform_id);
               algorithm::transform( scalables.executables, group.executables, transform_id);
            }

            auto groups = manager::task::create::scale::groups( state, { std::move( group)}); 

            auto result = casual::task::ids( groups, done_event);
            state.tasks.then( std::move( groups)).then( std::move( done_event));

            return result;
         }

         void shutdown( State& state, std::vector< common::process::Handle> processes)
         {
            Trace trace{ "domain::manager::handle::scale::shutdown"};
            log::line( verbose::log, "processes: ", processes);
            
            // We only want child signals
            signal::thread::scope::Mask mask{ signal::set::filled( code::signal::child)};

            // We need to correlate with the service-manager, if it's up

            common::message::domain::process::prepare::shutdown::Request request{ common::process::handle()};
            request.processes = std::move( processes);

            if( auto service_manager = state.singleton( common::communication::instance::identity::service::manager.id))
            {
               state.multiplex.send( service_manager.ipc, request);
               return;
            }

            // service manager not available ( should only happen in unittest)
            auto reply = common::message::reverse::type( request);
            reply.processes = std::move( request.processes);

            log::line( log, "failed to reach service-manager - action: emulate reply: ", reply);
            communication::ipc::inbound::device().push( std::move( reply));
         }

         void shutdown( State& state, const common::process::Handle& process)
         {
            shutdown( state, std::vector< common::process::Handle>{ process});
         }
         
         void instances( State& state, state::Server& server)
         {
            local::scale::in( state, server);
            local::scale::out( state, server);
         }

         void instances( State& state, state::Executable& executable)
         {
            local::scale::in( state, executable);
            local::scale::out( state, executable);
         }

      } // scale

      namespace restart
      {
         std::vector< common::strong::correlation::id> aliases( State& state, std::vector< std::string> aliases)
         {
            Trace trace{ "domain::manager::handle::restart::aliases"};
            log::line( verbose::log, "aliases: ", aliases);

            auto done_event = task::create::event::parent( state, "restart aliases");

            auto scalables = state.scalables( std::move( aliases));

            auto transform_id = []( auto& entity){ return entity.get().id;};

            state::dependency::Group group;
            algorithm::transform( scalables.servers, group.servers, transform_id);
            algorithm::transform( scalables.executables, group.executables, transform_id);

            auto groups = manager::task::create::restart::groups( state, { std::move( group)});

            auto result = casual::task::ids( groups, done_event);
            state.tasks.then( std::move( groups)).then( std::move( done_event));

            return result;
         }

         std::vector< common::strong::correlation::id> groups( State& state, std::vector< std::string> names)
         {
            Trace trace{ "domain::manager::handle::restart::groups"};
            log::line( verbose::log, "names: ", names);

            auto filter_groups = []( auto groups, const std::vector< std::string>& names)
            {
               if( names.empty())
                  return groups;

               auto has_name = []( auto& l, auto& r){ return l.description == r;};

               return std::get< 0>( algorithm::intersection( groups, names, has_name));
            };

            auto filter_untouchables = []( State& state, auto& groups)
            {
               algorithm::container::erase_if( groups, [ &state]( auto group)
               {
                  auto filter = [ &state]( auto id) { return state.untouchable( id);};

                  return range::empty( algorithm::container::erase_if( group.servers, filter))
                     && range::empty( algorithm::container::erase_if( group.executables, filter));
               });

            };

            auto done_event = task::create::event::parent( state, "restart groups");

            auto groups = state::order::shutdown( state);

            algorithm::container::trim( groups, filter_groups( range::make( groups), names));

            // filter
            filter_untouchables( state, groups);
            log::line( verbose::log, "groups: ", groups);

            auto tasks = manager::task::create::restart::groups( state, std::move( groups));

            auto result = casual::task::ids( tasks, done_event);
            state.tasks.then( std::move( tasks)).then( std::move( done_event));

            return result;
         }
      } // restart

      namespace process
      {
         void exit( const common::process::lifetime::Exit& exit)
         {
            Trace trace{ "domain::manager::handle::process::exit"};

            // We put a dead process event on our own ipc device, that
            // will be handled later on.
            communication::ipc::inbound::device().push( common::message::event::process::Exit{ exit});
         }
      } // process

      namespace local
      {
         namespace
         {
            namespace shutdown
            {
               auto request( State& state)
               {
                  return [&state]( common::message::shutdown::Request& message)
                  {
                     Trace trace{ "domain::manager::handle::shutdown"};
                     log::line( verbose::log, "message: ", message);

                     handle::shutdown( state);
                  };
               }

               namespace manager
               {
                  auto request( State& state)
                  {
                     return [&state]( common::message::domain::manager::shutdown::Request& message)
                     {
                        Trace trace{ "domain::manager::handle::manager::shutdown"};
                        log::line( verbose::log, "message: ", message);

                        state.runlevel = decltype( state.runlevel())::shutdown;

                        // register event subscription for caller
                        {
                           common::message::event::subscription::Begin subscription{ message.process};
                           subscription.types = { 
                              common::message::event::Task::type(),
                              common::message::event::Error::type()
                           };
                           state.event.subscription( subscription);
                        }

                        auto reply = common::message::reverse::type( message);
                        reply.tasks = handle::shutdown( state);

                        state.multiplex.send( message.process, reply);
                     };
                  }  
               } // manager
            } // shutdown

            namespace scale
            {
               namespace prepare
               {
                  auto shutdown( State& state)
                  {
                     return [&state]( common::message::domain::process::prepare::shutdown::Reply& message)
                     {
                        Trace trace{ "domain::manager::handle::scale::prepare::shutdown::Reply"};
                        log::line( verbose::log, "message: ", message);

                        for( auto& process : message.processes)
                        {
                           if( process.ipc)
                              state.multiplex.send( process.ipc, common::message::shutdown::Request{ common::process::handle()});
                           else
                              common::process::terminate( process.pid);
                        }
                     };
                  }
               } // prepare
            } // scale

            namespace event
            {
               namespace subscription
               {
                  auto begin( State& state)
                  {
               
                     return [&state]( const common::message::event::subscription::Begin& message)
                     {
                        Trace trace{ "domain::manager::handle::event::subscription::Begin"};
                        common::log::line( verbose::log, "message: ", message);

                        state.event.subscription( message);

                        common::log::line( log, "event: ", state.event);
                     };
                  }

                  auto end( State& state)
                  {
                     return [&state]( const common::message::event::subscription::End& message)
                     {
                        Trace trace{ "domain::manager::handle::event::subscription::End"};
                        common::log::line( verbose::log, "message: ", message);

                        state.event.subscription( message);
   
                        common::log::line( log, "event: ", state.event);
                     };
                  }

               } // subscription

               namespace process
               {
                  auto get_alias( State& state)
                  {
                     return [ &state]( strong::process::id pid) -> std::string
                     {
                        if( auto server = state.server( pid))
                           return server->alias;
                        else if( auto executable = state.executable( pid))
                           return executable->alias;
                        else if( auto grandchild = state.grandchild( pid))
                           return grandchild->alias;

                        return "<unknown>";
                     };
                  }

                  auto spawn( State& state)
                  {
                     return [&state]( const common::message::event::process::Spawn& message)
                     {
                        Trace trace{ "domain::manager::handle::event::process::spawn"};
                        log::line( verbose::log, "message: ", message);

                        // Are there any listeners to this event?
                        manager::task::event::dispatch( state, [&message]() -> decltype( message)
                        {
                           return message;
                        });
                        
                        // Dispatch to tasks
                        state.tasks( message);
                     };
                  }

                  auto exit( State& state)
                  {
                     return [&state]( const common::message::event::process::Exit& message)
                     {
                        Trace trace{ "domain::manager::handle::event::process::exit"};
                        log::line( verbose::log, "message: ", message);

                        if( message.state.deceased())
                        {
                           // We don't want to handle any signals in this task
                           signal::thread::scope::Block block;

                           auto alias =  get_alias( state)( message.state.pid);

                           if( message.state.reason == decltype( message.state.reason)::core)
                              log::line( log::category::error, "process cored, alias: ", alias, ", details: ", message.state);

                           auto singleton = state.singleton( message.state.pid);

                           auto [ server, executable] = state.remove( message.state.pid);

                           // only log on error if process is a singleton and is spawnable (i.e. not scaled down)
                           if( singleton && (( server && server->spawnable()) || ( executable && executable->spawnable())))
                              log::line( log::category::error, "process exited, alias: ", alias, ", details: ", message.state);
                           else
                              log::line( log::category::information, "process exited, alias: ", alias, ", details: ", message.state);

                           if( server)
                              handle::scale::instances( state, *server);

                           if( executable)
                              handle::scale::instances( state, *executable);

                           // dispatch to tasks
                           state.tasks( message);

                           // Are there any listeners to this event?
                           manager::task::event::dispatch( state, [ &message]() -> decltype( message)
                           {
                              return message;
                           });
                        }
                     };
                  }

                  auto assassination( State& state)
                  {
                     return [&state]( const common::message::event::process::Assassination& message)
                     {
                        Trace trace{ "domain::manager::handle::event::process::assassination"};
                        log::line( verbose::log, "message: ", message);

                        // Are there any subscribers to this event?
                        manager::task::event::dispatch( state, [&message]() -> decltype( message)
                        {
                           return message;
                        });

                        using contract_type = decltype( message.contract);

                        if( message.contract == contract_type::linger)
                        {
                           log::code( log, code::casual::domain_instance_assassinate, "event not severe enough, pid: ", message.target);
                           return;
                        } 

                        // fulfill assassination contract if not untouchable
                        if( ! algorithm::find( state.whitelisted, message.target))
                        {  
                           auto deduce_signal = []( contract_type contract)
                           {
                              if( contract == contract_type::interrupt)
                                 return common::code::signal::interrupt;
                              if( contract == contract_type::kill)
                                 return common::code::signal::kill;
                              return common::code::signal::abort;
                           };

                           common::signal::send( message.target, deduce_signal( message.contract));

                           log::line( log::category::error, code::casual::domain_instance_assassinate, " pid: ", message.target, ", alias: ", get_alias( state)( message.target), ", contract: ", message.contract, ", announcement: ", message.announcement);
                        }
                        else
                           log::information( code::casual::domain_instance_assassinate, "whitelisted process pardoned, pid: ", message.target);
                        
                     };
                  }

               } // process

               auto error( State& state)
               {
                  return [&state]( common::message::event::Error& message)
                  {
                     Trace trace{ "domain::manager::handle::event::error"};
                     log::line( verbose::log, "message: ", message);

                     manager::task::event::dispatch( state, [&message](){ return message;});

                     if( message.severity == decltype( message.severity)::fatal && state.runlevel == state::Runlevel::startup)
                     {
                        // We're in a 'fatal' state, and the only thing we can do is to shutdown
                        state.runlevel = state::Runlevel::error;
                        handle::shutdown( state);
                     }
                  };
               }

               auto notification( State& state)
               {
                  return [&state]( common::message::event::Notification& message)
                  {
                     Trace trace{ "domain::manager::handle::event::notification"};
                     log::line( verbose::log, "message: ", message);

                     manager::task::event::dispatch( state, [&message](){ return message;});
                  };
               }

               auto task( State& state) 
               {
                  return [&state]( common::message::event::Task& message)
                  {
                     Trace trace{ "domain::manager::handle::local::event::task"};
                     log::line( verbose::log, "message: ", message);

                     manager::task::event::dispatch( state, [&message](){ return message;});
                  };
               }

               namespace sub
               {
                  auto task( State& state) 
                  {
                     return [&state]( common::message::event::sub::Task& message)
                     {
                        Trace trace{ "domain::manager::handle::local::event::sub::task"};
                        log::line( verbose::log, "message: ", message);

                        manager::task::event::dispatch( state, [&message](){ return message;});
                     };
                  }
               } // task

               namespace ipc
               {
                  auto destroyed( State& state)
                  {
                     return [ &state]( common::message::event::ipc::Destroyed& message)
                     {
                        Trace trace{ "domain::manager::handle::local::event::ipc::destroyed"};
                        log::line( verbose::log, "message: ", message);

                        manager::task::event::dispatch( state, [ &message](){ return message;});
                     };
                  }
               } // ipc

               namespace transaction
               {
                  auto disassociate( State& state)
                  {
                     return [ &state]( common::message::event::transaction::Disassociate& message)
                     {
                        Trace trace{ "domain::manager::handle::local::event::transaction::disassociate"};
                        log::line( verbose::log, "message: ", message);

                        manager::task::event::dispatch( state, [ &message](){ return message;});
                     };
                  }
                  
                  
               } // transaction

            } // event

            namespace process
            {
               namespace detail
               {
                  namespace lookup
                  {
                     common::process::Handle pid( const State& state, strong::process::id pid)
                     {
                        if( auto server = state.server( pid))
                           return server->instance( pid)->handle;
                        else if ( auto grandchild = state.grandchild( pid))
                           return grandchild->handle;

                        return {};
                     }

                     auto request( State& state)
                     {
                        return [ &state]( const common::message::domain::process::lookup::Request& message)
                        {
                           Trace trace{ "domain::manager::handle::process::local::Lookup"};
                           log::line( verbose::log, "message: ", message);

                           using Directive = decltype( message.directive);

                           auto reply = common::message::reverse::type( message);
                           reply.identification = message.identification;

                           auto reply_direct = []( const State& state, auto& message)
                           {
                              return message.directive == Directive::direct || state.runlevel > decltype( state.runlevel())::running;
                           };

                           if( message.identification)
                           {
                              if( auto found = algorithm::find( state.singletons, message.identification))
                              {
                                 reply.process = found->second;
                                 state.multiplex.send( message.process, reply);
                              }
                              else if( reply_direct( state, message))
                                 state.multiplex.send( message.process, reply);
                              else
                                 return false;
                           }
                           else if( message.pid)
                           {
                              reply.process = detail::lookup::pid( state, message.pid);

                              if( reply.process || reply_direct( state, message))
                                 state.multiplex.send( message.process, reply);
                              else
                                 return false;
                           }
                           else
                           {
                              // invalid
                              log::line( log::category::error, "invalid lookup request");
                           }
                           return true;
                        };
                     }
                  } // lookup



                  namespace send
                  {
                     using Directive = common::message::domain::process::connect::reply::Directive;

                     void reply( State& state, Directive directive, const common::message::domain::process::connect::Request& message)
                     {
                        auto reply = common::message::reverse::type( message);
                        reply.directive = directive;
                        state.multiplex.send( message.information.handle, reply);
                     };
                  } // send

                  void connect( State& state, const common::message::domain::process::connect::Request& message)
                  {
                     Trace trace{ "domain::manager::handle::local::process::detail::connect"};

                     if( auto server = state.server( message.information.handle.pid))
                     {
                        server->connect( message.information.handle);
                        log::line( log, "added process: ", message.information.handle, " to ", *server);
                     }
                     else // we assume it's a grandchild
                        state.grandchildren.emplace_back( message.information.handle, std::move( message.information.alias), std::move( message.information.path));

                     algorithm::container::trim( state.pending.lookup, algorithm::remove_if( state.pending.lookup, process::detail::lookup::request( state)));

                     // tasks might be interested in server-connect
                     state.tasks( message);
                  }


                  namespace singleton
                  {
                     void service( State& state, const common::process::Handle& process)
                     {
                        Trace trace{ "domain::manager::handle::local::process::detail::singleton::service"};
                        
                        auto transform_service = []( const auto& service)
                        {
                           common::message::service::advertise::Service result;
                           result.name = service.name;
                           result.category = service.category;
                           result.transaction = service.transaction;
                           result.visibility = service.visibility;
                           return result;
                        };

                        common::message::service::Advertise message{ common::process::handle()};
                        message.alias = instance::alias();
                        message.services.add = algorithm::transform( manager::admin::services( state).services, transform_service);
                           
                        state.multiplex.send( process, message);
                     }

                     //! @returns true if it's a singleton process that tries to connect and this function takes
                     //! responsibility for all the connection actions
                     bool connect( State& state, const common::message::domain::process::connect::Request& message)
                     {
                        Trace trace{ "domain::manager::handle::local::process::detail::singleton::connect"};

                        if( ! message.singleton)
                           return false; // not a singleton

                        using Directive = decltype( common::message::reverse::type( message).directive);

                        if( auto found = algorithm::find( state.singletons, message.singleton.identification))
                        {
                           // A "singleton" is trying to connect, while we already have one connected

                           log::line( log::category::error, "only one instance is allowed for ", message.singleton.identification);

                           // Adjust configured instances to correspond to reality...

                           if( auto server = state.server( message.information.handle.pid))
                           {
                              server->remove( message.information.handle.pid);
                              server->scale( 1);
                           }
                           else if( auto executable = state.executable( message.information.handle.pid))
                           {
                              executable->remove( message.information.handle.pid);
                              executable->scale( 1);
                           }
                           
                           // deny startup
                           send::reply( state, Directive::denied, message);

                           manager::task::event::dispatch( state, [ &message]()
                           {
                              common::message::event::Error event;
                              event.code = common::code::casual::invalid_semantics;
                              event.message = string::compose( "server connect - only one instance is allowed for ", message.singleton.identification);
                              return event;
                           });

                           // early exit
                           return true;
                        }

                        state.singletons[ message.singleton.identification] = message.information.handle;

                        // possible set environment-state so new spawned processes can use it directly
                        if( ! message.singleton.environment.empty())
                           environment::variable::set( message.singleton.environment, message.information.handle);

                        if( message.whitelist)
                           state.whitelisted.push_back( message.information.handle.pid);

                        // if service-manager, we need to interact.
                        if( message.singleton.identification == communication::instance::identity::service::manager.id)
                           singleton::service( state, message.information.handle);

                        process::detail::connect( state, message);

                        // 'allow' the 'singleton' to start
                        send::reply( state, Directive::approved, message);

                        return true;

                     }
                  } // singleton

               } // detail

               auto connect( State& state)
               {
                  return [&state]( const common::message::domain::process::connect::Request& message)
                  {
                     Trace trace{ "domain::manager::handle::local::process::connect"};
                     common::log::line( verbose::log, "message: ", message);

                     using Directive = decltype( common::message::reverse::type( message).directive);

                     if( state.runlevel > decltype( state.runlevel())::running)
                     {
                        log::line( log::category::information, "refuse connect for pid: ", message.information.handle.pid, " - runlevel: ", state.runlevel);
                        detail::send::reply( state, Directive::denied, message);
                        return;
                     }

                     if( detail::singleton::connect( state, message))
                        return; // singleton, and handled.
                 
                     if( message.whitelist)
                        state.whitelisted.push_back( message.information.handle.pid);

                     detail::connect( state, message);
                     
                     // allow the server to start.
                     detail::send::reply( state, Directive::approved, message);
                  };
               }

               auto lookup( State& state)
               {
                  return [&state]( const common::message::domain::process::lookup::Request& message)
                  {
                     Trace trace{ "domain::manager::handle::process::Lookup"};

                     if( ! detail::lookup::request( state)( message))
                        state.pending.lookup.push_back( message);
                  };
               }

               namespace information
               {
                  auto request( State& state)
                  {
                     return [&state]( const common::message::domain::process::information::Request& message)
                     {
                        Trace trace{ "domain::manager::handle::process::information::request"};

                        auto reply = common::message::reverse::type( message);
                        algorithm::for_each( message.handles, [&state, &reply]( const auto& handle)
                        {
                           if( handle.pid)
                           {
                              if( auto server = state.server( handle.pid))
                                 reply.processes.emplace_back( server->instance( handle.pid)->handle, server->alias, server->path);
                              else if( auto executable = state.executable( handle.pid))
                                 // we make sure to return a handle with pid only, since executables lack ipc
                                 reply.processes.emplace_back( common::process::Handle{ handle.pid}, executable->alias, executable->path);
                              else if( auto grandchild = state.grandchild( handle.pid))
                                 reply.processes.emplace_back( grandchild->handle, grandchild->alias, grandchild->path);
                           }
                        });

                        state.multiplex.send( message.process, reply);
                     };
                  }
               } // information

            } // process

            namespace configuration
            {
               auto request( State& state)
               {
                  return [ &state]( const casual::configuration::message::Request& message)
                  {
                     Trace trace{ "domain::manager::handle::configuration::request"};
                     common::log::line( verbose::log, "message: ", message);

                     auto reply = common::message::reverse::type( message, common::process::handle());
                     reply.model = state.configuration.model;

                     state.multiplex.send( message.process, reply);
                  };
               }

               namespace stakeholder
               {
                  auto registration( State& state)
                  {
                     return [ &state]( const casual::configuration::message::stakeholder::registration::Request& message)
                     {
                        Trace trace{ "domain::manager::handle::configuration::stakeholder::registration"};
                        common::log::line( verbose::log, "message: ", message);

                        state::configuration::Stakeholder stakeholder;
                        stakeholder.process = message.process;
                        stakeholder.contract = message.contract;

                        // make sure we can ask for configuration state later...
                        algorithm::append_unique_value( stakeholder, state.configuration.stakeholders);

                        state.multiplex.send( message.process, common::message::reverse::type( message));
                     };
                  }
               } // stakeholder

               namespace update
               {
                  auto reply( State& state)
                  {
                     return [ &state]( const casual::configuration::message::update::Reply& message)
                     {
                        Trace trace{ "domain::manager::handle::configuration::update::reply"};
                        common::log::line( verbose::log, "message: ", message);

                        state.tasks( message);
                     };
                  }
               } // update

            } // configuration

            namespace server
            {
               using base_type = common::server::handle::policy::call::Admin;
               struct Policy : base_type
               {
                  Policy( manager::State& state)
                     :  m_state( state) {}

                  void configure( common::server::Arguments&& arguments)
                  {
                     // no-op, we'll advertise our services when the broker comes online.
                  }

                  // overload ack so we use domain-manager internal stuff to lookup service-manager
                  void ack( const common::message::service::call::ACK& message)
                  {
                     Trace trace{ "domain::manager::handle::local::server::Policy::ack"};

                     if( auto service_manager = m_state.singleton( common::communication::instance::identity::service::manager.id))
                         m_state.multiplex.send( service_manager.ipc, message);
                     else
                        log::line( log, "failed to reach service-manager - action: discard sending ACK");
                  }

               private: 
                  manager::State& m_state;
               };

               using Handle = common::server::handle::basic_call< Policy>;
            } // server

         } // <unnamed>
      } // local

      dispatch_type create( State& state)
      {
         Trace trace{ "domain::manager::handle::create"};

         return dispatch_type{
            common::message::dispatch::handle::defaults( state),
            handle::local::shutdown::request( state),
            handle::local::shutdown::manager::request( state),
            handle::local::scale::prepare::shutdown( state),
            handle::local::event::process::spawn( state),
            handle::local::event::process::exit( state),
            handle::local::event::process::assassination( state),
            handle::local::event::subscription::begin( state),
            handle::local::event::subscription::end( state),
            handle::local::event::error( state),
            handle::local::event::notification( state),
            handle::local::event::task( state),
            handle::local::event::sub::task( state),
            handle::local::event::ipc::destroyed( state),
            handle::local::event::transaction::disassociate( state),
            handle::local::process::connect( state),
            handle::local::process::lookup( state),
            handle::local::process::information::request( state),
            handle::local::configuration::request( state),
            handle::local::configuration::update::reply( state),
            handle::local::configuration::stakeholder::registration( state),
            handle::local::server::Handle{
               manager::admin::services( state),
               state}
         };

      }


   } // domain::manager::handle
} // casual

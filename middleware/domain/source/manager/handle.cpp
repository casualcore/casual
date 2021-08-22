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
#include "domain/pending/message/message.h"

#include "configuration/message.h"

#include "common/message/handle.h"
#include "common/server/handle/call.h"
#include "common/cast.h"
#include "common/environment.h"
#include "common/algorithm/compare.h"
#include "common/algorithm.h"

#include "common/instance.h"

#include "common/communication/instance.h" 

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
                  algorithm::for_each( spawnable, []( auto& entity){ entity.state = decltype( entity.state)::error;});

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
                        log::line( log::category::error, error, " failed to spawn: ", entity.path);

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
                     common::message::event::process::Spawn message;
                     message.path = entity.path;
                     message.alias = entity.alias;

                     algorithm::transform_if( spawnable, message.pids, 
                     []( auto& instance){ return common::process::id( instance.handle);},
                     []( auto& instance){ return instance.state != decltype( instance.state)::error;});

                     ipc::push( message);
                  }

                  log::line( verbose::log, "entity: ", entity);
               }

               void in( const State& state, const state::Executable& executable)
               {
                  Trace trace{ "domain::manager::handle::scale::in Executable"};

                  auto shutdownable = executable.shutdownable();

                  if( ! shutdownable)
                     return;

                  // We only want child signals
                  signal::thread::scope::Mask mask{ signal::set::filled( code::signal::child)};


                  auto pids = algorithm::transform( range::reverse( shutdownable), []( const auto& i){
                     return i.handle;
                  });

                  common::process::terminate( pids);
               }

               void in( State& state, const state::Server& server)
               {
                  Trace trace{ "domain::manager::handle::local::scale::in Server"};

                  auto shutdownable = server.shutdownable();

                  if( ! shutdownable)
                     return;

                  handle::scale::shutdown( state, algorithm::transform( range::reverse( shutdownable), []( const auto& i){
                     return i.handle;
                  }));
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
                     server.alias = "casual-domain-pending-message";
                     server.path = process::path().parent_path() / "casual-domain-pending-message";
                     server.scale( 1);
                     server.memberships.push_back( state.group_id.core);
                     server.note = "handles pending internal messages";
                     server.restart = true;

                     state.servers.push_back( std::move( server));
                  }

                  {
                     auto server = state::Server::create();
                     server.alias = "casual-domain-discovery";
                     server.path = process::path().parent_path() / "casual-domain-discovery";
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
                  manager.path = process::path().parent_path() / "casual-service-manager";
                  manager.scale( 1);
                  manager.memberships.push_back( state.group_id.master);
                  manager.note = "service lookup and management";

                  state.servers.push_back( std::move( manager));
               }

               {
                  auto tm = state::Server::create();
                  tm.alias = "casual-transaction-manager";
                  tm.path = process::path().parent_path() / "casual-transaction-manager";
                  tm.scale( 1);
                  tm.memberships.push_back( state.group_id.transaction);
                  tm.note = "manage transaction in this domain";

                  state.servers.push_back( std::move( tm));
               }

               {
                  auto queue = state::Server::create();
                  queue.alias = "casual-queue-manager";
                  queue.path = process::path().parent_path() / "casual-queue-manager";
                  queue.scale( 1);
                  queue.memberships.push_back( state.group_id.queue);
                  queue.note = "manage queues in this domain";

                  state.servers.push_back( std::move( queue));
               }

               {
                  auto gateway = state::Server::create();
                  gateway.alias = "casual-gateway-manager";
                  gateway.path = process::path().parent_path() / "casual-gateway-manager";
                  gateway.scale( 1);
                  gateway.memberships.push_back( state.group_id.gateway);
                  gateway.note = "manage connections to and from other domains";

                  state.servers.push_back( std::move( gateway));
               }
            }
         } // boot
      } // mandatory


      void boot( State& state, common::strong::correlation::id correlation)
      {
         Trace trace{ "domain::manager::handle::boot"};
                                          
         // send domain-information if _event-listeners_ wants it.
         manager::task::event::dispatch( state, [&correlation]()
         {
            task::message::domain::Information event{ common::process::handle()};
            event.correlation = correlation;
            event.domain = common::domain::identity();
            return event;
         });

         state.tasks.add( manager::task::create::scale::boot( state::order::boot( state), correlation));
      }

      std::vector< common::strong::correlation::id> shutdown( State& state)
      {
         Trace trace{ "domain::manager::handle::shutdown"};

         state.runlevel = state::Runlevel::shutdown;

         // abort all abortble running or pending task
         state.tasks.abort( state);

         // prepare scaling to 0
         auto prepare_shutdown = []( auto& entity)
         {
            entity.scale( 0);
         };

         algorithm::for_each( state.executables, prepare_shutdown);
         // Make sure we exclude our self so we don't try to shutdown
         algorithm::for_each_if( state.servers, prepare_shutdown, [&state]( auto& entity){ return entity.id != state.manager_id;});

         return { state.tasks.add( manager::task::create::scale::shutdown( state::order::shutdown( state)))};
      }


      namespace scale
      {
         void shutdown( State& state, std::vector< common::process::Handle> processes)
         {
            Trace trace{ "domain::manager::handle::scale::shutdown"};
            log::line( verbose::log, "processes: ", processes);
            
            // We only want child signals
            signal::thread::scope::Mask mask{ signal::set::filled( code::signal::child)};

            // We need to correlate with the service-manager (broker), if it's up

            common::message::domain::process::prepare::shutdown::Request prepare{ common::process::handle()};
            prepare.processes = std::move( processes);

            auto service_manager = state.singleton( common::communication::instance::identity::service::manager.id);

            try
            {
               communication::device::blocking::send( service_manager.ipc, prepare);
            }
            catch( ...)
            {
               auto error = exception::capture();
               log::line( log, error, " failed to reach service-manager - action: emulate reply");

               // service-manager is not online, we emulate the reply from service-manager
               // and send it to our self to ensure that possible tasks are initalized and
               // ready 

               auto reply = common::message::reverse::type( prepare);
               reply.processes = std::move( prepare.processes);
               ipc::push( reply);
            }
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

         std::vector< common::strong::correlation::id> aliases( State& state, std::vector< admin::model::scale::Alias> aliases)
         {
            Trace trace{ "domain::manager::handle::scale::aliases"};
            log::line( verbose::log, "aliases: ", aliases);

            auto runnables = state.runnables( algorithm::transform( aliases, []( auto& a){ return a.name;}));

            auto unrestartable_server = []( auto server)
            {
               // for now, only the domain-manager is unrestartable
               return server.get().instances.size() == 1 && common::process::id( server.get().instances[ 0].handle) == common::process::id();
            };

            algorithm::trim( runnables.servers, algorithm::remove_if( runnables.servers, unrestartable_server));

            // prepare the scaling
            {                  
               auto scale_runnable = [&aliases]( auto& runnable)
               {
                  auto is_alias = [&runnable]( auto& alias){ return alias.name == runnable.get().alias;};

                  if( auto found = algorithm::find_if( aliases, is_alias))
                     runnable.get().scale( found->instances);
               };

               algorithm::for_each( runnables.servers, scale_runnable);
               algorithm::for_each( runnables.executables, scale_runnable);
            }

            auto transform_id = []( auto& entity){ return entity.get().id;};

            state::dependency::Group group;
            algorithm::transform( runnables.servers, group.servers, transform_id);
            algorithm::transform( runnables.executables, group.executables, transform_id);

            return { state.tasks.add( manager::task::create::scale::aliases( { std::move( group)}))};
         }


      } // scale

      namespace restart
      {
         namespace local
         {
            namespace
            {
                  // for now, only the domain-manager is unrestartable
               auto filter_unrestartable( State& state)
               {
                  return [untouchables = state.untouchables()]( state::dependency::Group& group)
                  {
                     auto filter = []( auto& ids, auto& untouchables)
                     {
                        algorithm::trim( ids, algorithm::remove_if( ids, [&untouchables]( auto& id)
                        { 
                           return ! algorithm::find( untouchables, id).empty();
                        }));
                     };

                     filter( group.servers, std::get< 0>( untouchables));
                     filter( group.executables, std::get< 1>( untouchables));
                  };
               }
               
            } // <unnamed>
         } // local
         std::vector< common::strong::correlation::id> aliases( State& state, std::vector< std::string> aliases)
         {
            Trace trace{ "domain::manager::handle::restart::aliases"};
            log::line( verbose::log, "aliases: ", aliases);

            auto runnables = state.runnables( std::move( aliases));

            auto transform_id = []( auto& entity){ return entity.get().id;};

            state::dependency::Group group;
            algorithm::transform( runnables.servers, group.servers, transform_id);
            algorithm::transform( runnables.executables, group.executables, transform_id);

            local::filter_unrestartable( state)( group);

            return { state.tasks.add( manager::task::create::restart::aliases( { std::move( group)}))};
         }

         std::vector< common::strong::correlation::id> groups( State& state, std::vector< std::string> names)
         {
            Trace trace{ "domain::manager::handle::restart::groups"};
            log::line( verbose::log, "names: ", names);

            auto groups = state::order::shutdown( state);

            auto filter_groups = []( auto groups, const std::vector< std::string>& names)
            {
               if( names.empty())
                  return groups;

               auto has_name = []( auto& l, auto& r){ return l.description == r;};

               return std::get< 0>( algorithm::intersection( groups, names, has_name));
            };

            algorithm::trim( groups, filter_groups( range::make( groups), names));

            // filter
            algorithm::for_each( groups, local::filter_unrestartable( state));
            log::line( verbose::log, "groups: ", groups);

            return { state.tasks.add( manager::task::create::restart::aliases( std::move( groups)))};
         }
      } // restart

      namespace event
      {
         namespace process
         {
            void exit( const common::process::lifetime::Exit& exit)
            {
               Trace trace{ "domain::manager::handle::event::process::exit"};

               // We put a dead process event on our own ipc device, that
               // will be handled later on.
               ipc::push( common::message::event::process::Exit{ exit});
            }
         } // process
      } // event

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

                        // register event subscripion for caller
                        {
                           common::message::event::subscription::Begin subscripton{ message.process};
                           subscripton.types = { 
                              common::message::event::Task::type(),
                              common::message::event::Error::type()
                           };
                           state.event.subscription( subscripton);
                        }

                        auto reply = common::message::reverse::type( message);
                        reply.tasks = handle::shutdown( state);

                        casual::domain::manager::ipc::send( state, message.process, reply);
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

                        algorithm::for_each( message.processes, [&]( auto& process)
                        {
                           if( process)
                           {
                              common::message::shutdown::Request shutdown{ common::process::handle()};

                              // Just to make each shutdown easy to follow in log.
                              shutdown.execution = decltype( shutdown.execution)::emplace( uuid::make());

                              manager::ipc::send( state, process, shutdown);
                           }
                           else
                              common::process::terminate( process.pid);
                        });
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
                        state.tasks.event( state, message);
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

                           if( message.state.reason == common::process::lifetime::Exit::Reason::core)
                              log::line( log::category::error, "process cored: ", message.state);
                           else
                              log::line( log::category::information, "process exited: ", message.state);

                           auto restarts = state.remove( message.state.pid);

                           if( std::get< 0>( restarts)) handle::scale::instances( state, *std::get< 0>( restarts));
                           if( std::get< 1>( restarts)) handle::scale::instances( state, *std::get< 1>( restarts));

                           // dispatch to tasks
                           state.tasks.event( state, message);

                           // Are there any listeners to this event?
                           manager::task::event::dispatch( state, [&message]() -> decltype( message)
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

                        if( message.contract == decltype( message.contract)::linger)
                        {
                           log::line( log, code::casual::domain_instance_assassinate, " event not severe enough, pid: ", message.target);
                           return;
                        } 

                        if( ! algorithm::find( state.whitelisted, message.target))
                        {                             
                           // fulfill assassination contract if not untouchable
                           auto weapon = common::code::signal::kill;
                           if( message.contract == decltype( message.contract)::terminate) 
                              weapon = common::code::signal::terminate;
                           
                           common::signal::send( message.target, weapon);
                           log::line( log::category::error, code::casual::domain_instance_assassinate, " pid: ", message.target, ", weapon: ", weapon);
                        }
                        else
                           log::line( log::category::information, code::casual::domain_instance_assassinate, " whitelisted process pardoned, pid: ", message.target);
                        
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

               namespace discoverable
               {
                  auto available( State& state)
                  {
                     return [&state]( common::message::event::discoverable::Avaliable& event)
                     {
                        Trace trace{ "domain::manager::handle::local::event::discoverable::available"};
                        log::line( verbose::log, "event: ", event);

                        manager::task::event::dispatch( state, [&event](){ return event;});
                     };
                  }
               } // discoverable
            } // event

            namespace process
            {
               namespace detail
               {
                  namespace lookup
                  {
                     common::process::Handle pid( const State& state, strong::process::id pid)
                     {
                        auto server = state.server( pid);
                        
                        if( server)
                           return server->instance( pid)->handle;
                        else
                           return state.grandchild( pid);
                     }

                     auto request( State& state)
                     {
                        return [&state]( const common::message::domain::process::lookup::Request& message)
                        {
                           Trace trace{ "domain::manager::handle::process::local::Lookup"};

                           log::line( verbose::log, "message: ", message);

                           using Directive = decltype( message.directive);

                           auto reply = common::message::reverse::type( message);
                           reply.identification = message.identification;

                           if( message.identification)
                           {
                              auto found = algorithm::find( state.singletons, message.identification);

                              if( found)
                              {
                                 reply.process = found->second;
                                 manager::ipc::send( state, message.process, reply);
                              }
                              else if( message.directive == Directive::direct)
                                 manager::ipc::send( state, message.process, reply);
                              else
                                 return false;
                           }
                           else if( message.pid)
                           {
                              reply.process = detail::lookup::pid( state, message.pid);

                              if( reply.process)
                                 manager::ipc::send( state, message.process, reply);
                              else if( message.directive == Directive::direct)
                                 manager::ipc::send( state, message.process, reply);
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
                     auto reply = []( State& state, auto directive, const auto& message)
                     {
                        auto reply = common::message::reverse::type( message);
                        reply.directive = directive;
                        manager::ipc::send( state, message.process, reply);
                     };
                  } // send

                  template< typename M>
                  void connect( State& state, const M& message)
                  {
                     if( auto server = state.server( message.process.pid))
                     {
                        server->connect( message.process);
                        log::line( log, "added process: ", message.process, " to ", *server);
                     }
                     else // we assume it's a grandchild
                        state.grandchildren.push_back( message.process);

                     algorithm::trim( state.pending.lookup, algorithm::remove_if( state.pending.lookup, process::detail::lookup::request( state)));

                     // tasks might be interested in server-connect
                     state.tasks.event( state, message);
                  }


                  namespace singleton
                  {
                     void service( State& state, const common::process::Handle& process)
                     {
                        Trace trace{ "domain::manager::handle::local::process::detail::singleton::service"};
                        
                        auto transform_service = []( const auto& s)
                        {
                           common::message::service::advertise::Service result;
                           result.name = s.name;
                           result.category = s.category;
                           result.transaction = s.transaction;
                           return result;
                        };

                        common::message::service::Advertise message{ common::process::handle()};
                        message.alias = instance::alias();
                        message.services.add = algorithm::transform( manager::admin::services( state).services, transform_service);
                           
                        manager::ipc::send( state, process, message);
                     }

                     //! @returns true if it's a singleton process that tries to connect and this function takes
                     //! responsibility for all the connnetion actions
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

                           if( auto server = state.server( message.process.pid))
                           {
                              server->remove( message.process.pid);
                              server->scale( 1);
                           }
                           else if( auto executable = state.executable( message.process.pid))
                           {
                              executable->remove( message.process.pid);
                              executable->scale( 1);
                           }
                           
                           // deny startup
                           send::reply( state, Directive::denied, message);

                           manager::task::event::dispatch( state, [&message]()
                           {
                              common::message::event::Error event;
                              event.code = common::code::casual::invalid_semantics;
                              event.message = string::compose( "server connect - only one instance is allowed for ", message.singleton.identification);
                              return event;
                           });

                           // early exit
                           return true;
                        }

                        state.singletons[ message.singleton.identification] = message.process;

                        // possible set environment-state so new spawned processes can use it directly
                        if( ! message.singleton.environment.empty())
                           environment::variable::process::set( message.singleton.environment, message.process);

                        if( message.whitelist)
                           state.whitelisted.push_back( message.process.pid);

                        // if ervice-manager, we need to interact.
                        if( message.singleton.identification == communication::instance::identity::service::manager.id)
                           singleton::service( state, message.process);

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
                        log::line( log::category::information, "refuse connect for pid: ", message.process.pid, " - runlevel: ", state.runlevel);
                        detail::send::reply( state, Directive::denied, message);
                        return;
                     }

                     if( detail::singleton::connect( state, message))
                        return; // singleton, and handled.
                 
                     if( message.whitelist)
                        state.whitelisted.push_back( message.process.pid);

                     detail::connect( state, message);
                     
                     // alllow the server to start.
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

            } // process

            namespace configuration
            {
               auto request( State& state)
               {
                  return [&state]( const casual::configuration::message::Request& message)
                  {
                     Trace trace{ "domain::manager::handle::configuration::request"};
                     common::log::line( verbose::log, "message: ", message);

                     auto reply = common::message::reverse::type( message, common::process::handle());
                     reply.model = state.configuration.model;

                     manager::ipc::send( state, message.process, reply);
                  };
               }

               namespace update
               {
                  auto reply( State& state)
                  {
                     return [&state]( const casual::configuration::message::update::Reply& message)
                     {
                        Trace trace{ "domain::manager::handle::configuration::update::reply"};
                        common::log::line( verbose::log, "message: ", message);

                        state.tasks.event( state, message);

                     };

                  }
               } // update

               namespace supplier
               {
                  auto registration( State& state)
                  {
                     return [&state]( const casual::configuration::message::supplier::Registration& message)
                     {
                        Trace trace{ "domain::manager::handle::configuration::supplier::registration"};
                        common::log::line( verbose::log, "message: ", message);

                        // make sure we can ask for configuration state later...
                        // TODO semantics: might be implicit on handling casual::configuration::message::Request...
                        algorithm::append_unique_value( message.process, state.configuration.suppliers);
                     };
                  }
               } // supplier

            } // configuration

            namespace server
            {
               using base_type = common::server::handle::policy::call::Admin;
               struct Policy : base_type
               {
                  using common::server::handle::policy::call::Admin::Admin;

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

                     try
                     {
                        auto service_manager = m_state.singleton( common::communication::instance::identity::service::manager.id);
                        communication::device::blocking::send( service_manager.ipc, message);
                     }
                     catch( ...)
                     {
                        common::log::line( log, exception::capture(), " failed to reach service-manager - action: discard sending ACK");
                     }
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

         return {
            common::message::handle::defaults( ipc::device()),
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
            handle::local::event::discoverable::available( state),
            handle::local::process::connect( state),
            handle::local::process::lookup( state),
            handle::local::configuration::request( state),
            handle::local::configuration::update::reply( state),
            handle::local::configuration::supplier::registration( state),
            handle::local::server::Handle{
               manager::admin::services( state),
               state}
         };

      }


   } // domain::manager::handle
} // casual

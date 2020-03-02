//! 
//! Copyright (c) 2015, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!


#include "domain/manager/handle.h"

#include "domain/manager/admin/server.h"
#include "domain/manager/task/create.h"
#include "domain/manager/task/event.h"
#include "domain/manager/persistent.h"
#include "domain/common.h"
#include "domain/transform.h"
#include "domain/pending/message/environment.h"
#include "domain/pending/message/message.h"

#include "configuration/gateway.h"


#include "common/message/handle.h"
#include "common/server/handle/call.h"
#include "common/cast.h"
#include "common/environment.h"
#include "common/algorithm/compare.h"
#include "common/instance.h"

#include "common/communication/instance.h" 



namespace casual
{
   using namespace common;

   namespace domain
   {
      namespace manager
      {
         namespace handle
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
                           catch( const exception::system::invalid::Argument& exception)
                           {
                              log::line( log::category::error, "failed to spawn: ", entity, " - ", exception);

                              manager::task::event::dispatch( state, [&]()
                              {
                                 common::message::event::domain::Error message;
                                 message.severity = decltype( message.severity)::error;
                                 message.message = string::compose( "failed to spawn '", entity.path, "' - ", exception);
                                 return message;
                              });
                           }
                           
                           ++range;
                        }

                        // send event, if any
                        manager::task::event::dispatch( state, [&]()
                        {
                           common::message::event::process::Spawn message;
                           message.path = entity.path;
                           message.alias = entity.alias;

                           algorithm::transform_if( spawnable, message.pids, 
                           []( auto& instance){ return common::process::id( instance.handle);},
                           []( auto& instance){ return instance.state != decltype( instance.state)::error;});

                           return message;
                        });

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

            namespace start
            {
               namespace pending
               {
                  common::Process message()
                  {
                     Trace trace{ "domain::manager::handle::start::pending::message"};

                     auto process = common::Process{ string::compose( common::environment::directory::casual(), "/bin/", casual::domain::pending::message::environment::executable)};

                     // wait for connect
                     casual::domain::pending::message::Connect connect;
                     communication::ipc::blocking::receive( ipc::device(), connect);

                     process.handle( connect.process);
                     environment::variable::process::set( casual::domain::pending::message::environment::variable, process.handle());

                     return process;
                  }
               } // pending
            } // start

            namespace mandatory
            {
               namespace boot
               {
                  void prepare( State& state)
                  {
                     Trace trace{ "domain::manager::handle::mandatory::boot::prepare"};

                     {
                        state::Server manager;
                        manager.alias = "casual-service-manager";
                        manager.path = "${CASUAL_HOME}/bin/casual-service-manager";
                        manager.scale( 1);
                        manager.memberships.push_back( state.group_id.master);
                        manager.note = "service lookup and management";

                        state.servers.push_back( std::move( manager));
                     }

                     {
                        state::Server tm;
                        tm.alias = "casual-transaction-manager";
                        tm.path = "${CASUAL_HOME}/bin/casual-transaction-manager";
                        tm.scale( 1);
                        tm.memberships.push_back( state.group_id.transaction);
                        tm.note = "manage transaction in this domain";
                        tm.arguments = {
                              "--transaction-log",
                              state.configuration.transaction.log
                        };

                        state.servers.push_back( std::move( tm));
                     }

                     {
                        state::Server queue;
                        queue.alias = "casual-queue-manager";
                        queue.path = "${CASUAL_HOME}/bin/casual-queue-manager";
                        queue.scale( 1);
                        queue.memberships.push_back( state.group_id.queue);
                        queue.note = "manage queues in this domain";

                        state.servers.push_back( std::move( queue));
                     }

                     //if( ! state.configuration.gateway.listeners.empty() || ! state.configuration.gateway.connections.empty())
                     {
                        state::Server gateway;
                        gateway.alias = "casual-gateway-manager";
                        gateway.path = "${CASUAL_HOME}/bin/casual-gateway-manager";
                        gateway.scale( 1);
                        gateway.memberships.push_back( state.group_id.gateway);
                        gateway.note = "manage connections to and from other domains";

                        state.servers.push_back( std::move( gateway));
                     }
                  }
               } // boot
            } // mandatory


            void boot( State& state)
            {
               Trace trace{ "domain::manager::handle::boot"};

               manager::task::event::dispatch( state, [&]()
               {
                  common::message::event::domain::boot::Begin event;
                  event.domain = common::domain::identity();
                  return event;
               });

               state.tasks.add( state, manager::task::create::scale::dependency( state, state.bootorder()));

               // add a "sentinel" to fire an event when the boot is done
               state.tasks.add( state, manager::task::create::done::boot());
            }

            void shutdown( State& state)
            {
               Trace trace{ "domain::manager::handle::shutdown"};

               state.runlevel( State::Runlevel::shutdown);

               // TODO: collect state from sub-managers...
               if( state.persist)
                  persistent::state::save( state);

               manager::task::event::dispatch( state, [&]()
               {
                  common::message::event::domain::shutdown::Begin event;
                  event.domain = common::domain::identity();
                  return event;
               });

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

               state.tasks.add( state, manager::task::create::scale::dependency( state, state.shutdownorder()));

               // add a "sentinel" to fire an event when the shutdown is done
               state.tasks.add( state, manager::task::create::done::shutdown());
            }


            Base::Base( State& state) : m_state{ state} {}

            State& Base::state()
            {
               return m_state.get();
            }

            const State& Base::state() const
            {
               return m_state.get();
            }


            void Shutdown::operator () ( common::message::shutdown::Request& message)
            {
               Trace trace{ "domain::manager::handle::Shutdown"};
               log::line( verbose::log, "message: ", message);

               state().tasks.add( state(), manager::task::create::shutdown());
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

                  auto service_manager = state.singleton( common::communication::instance::identity::service::manager);

                  try
                  {
                     communication::ipc::blocking::send( service_manager.ipc, prepare);
                  }
                  catch( const exception::system::communication::Unavailable&)
                  {
                     log::line( log, "service-manager not online - action: emulate reply");

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

               std::vector< common::strong::task::id> aliases( State& state, std::vector< admin::model::scale::Alias> aliases)
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

                  auto add_task = [&state]( auto callable)
                  {
                     return [&state, callable]( auto entity)
                     {
                        return state.tasks.add( state, callable( state, entity.get().id));
                     };
                  };

                  auto result = algorithm::transform( runnables.servers, add_task( &manager::task::create::scale::server));
                  algorithm::transform( runnables.executables, result, add_task( &manager::task::create::scale::executable));

                  return result;
               }

               namespace prepare
               {
                  void Shutdown::operator () ( common::message::domain::process::prepare::shutdown::Reply& message)
                  {
                     Trace trace{ "domain::manager::handle::scale::prepare::shutdown::Reply"};
                     log::line( verbose::log, "message: ", message);

                     algorithm::for_each( message.processes, [&]( auto& process)
                     {
                        if( process)
                        {
                           common::message::shutdown::Request shutdown{ common::process::handle()};

                           // Just to make each shutdown easy to follow in log.
                           shutdown.execution = uuid::make();

                           manager::ipc::send( state(), process, shutdown);
                        }
                        else
                           common::process::terminate( process.pid);
                     });
                  }
               } // prepare
            } // scale

            namespace restart
            {
               std::vector< common::strong::task::id> instances( State& state, std::vector< std::string> aliases)
               {
                  Trace trace{ "domain::manager::handle::restart::instances"};
                  log::line( verbose::log, "aliases: ", aliases);

                  auto runnables = state.runnables( std::move( aliases));

                  auto add_task = [&state]( auto callable)
                  {
                     return [&state, callable]( auto entity)
                     {
                        return state.tasks.add( state, callable( state, entity.get().id));
                     };
                  };

                  auto unrestartable_server = []( auto server)
                  {
                     // for now, only the domain-manager is unrestartable
                     return server.get().instances.size() == 1 && common::process::id( server.get().instances[ 0].handle) == common::process::id();
                  };

                  algorithm::trim( runnables.servers, algorithm::remove_if( runnables.servers, unrestartable_server));

                  auto result = algorithm::transform( runnables.servers, add_task( &manager::task::create::restart::server));
                  algorithm::transform( runnables.executables, result, add_task( &manager::task::create::restart::executable));

                  return result;

               }
            } // restart

            namespace event
            {
               namespace subscription
               {
                  void Begin::operator () ( const common::message::event::subscription::Begin& message)
                  {
                     Trace trace{ "domain::manager::handle::event::subscription::Begin"};
                     common::log::line( verbose::log, "message: ", message);

                     state().event.subscription( message);

                     common::log::line( log, "event: ", state().event);
                  }

                  void End::operator () ( const common::message::event::subscription::End& message)
                  {
                     Trace trace{ "domain::manager::handle::event::subscription::End"};
                     common::log::line( verbose::log, "message: ", message);

                     state().event.subscription( message);
 
                     common::log::line( log, "event: ", state().event);
                  }

               } // subscription

               namespace process
               {
                  void exit( const common::process::lifetime::Exit& exit)
                  {
                     Trace trace{ "domain::manager::handle::event::process::exit"};

                     // We put a dead process event on our own ipc device, that
                     // will be handled later on.
                     communication::ipc::inbound::device().push( common::message::event::process::Exit{ exit});
                  }

                  void Exit::operator () ( common::message::event::process::Exit& message)
                  {
                     Trace trace{ "domain::manager::handle::event::process::Exit"};

                     log::line( verbose::log, "message: ", message);

                     if( message.state.deceased())
                     {
                        // We don't want to handle any signals in this task
                        signal::thread::scope::Block block;

                        if( message.state.reason == common::process::lifetime::Exit::Reason::core)
                           log::line( log::category::error, "process cored: ", message.state);
                        else
                           log::line( log::category::information, "process exited: ", message.state);

                        auto restarts = state().remove( message.state.pid);

                        if( std::get< 0>( restarts)) scale::instances( state(), *std::get< 0>( restarts));
                        if( std::get< 1>( restarts)) scale::instances( state(), *std::get< 1>( restarts));

                        // Are there any listeners to this event?
                        manager::task::event::dispatch( state(), [&message]() -> decltype( message)
                        {
                           return message;
                        });

                        // check if the process is our own pending-send
                        // should not be possible unless some "human" kills the process
                        if( message.state.pid == state().process.pending.handle())
                        {
                           log::line( log::category::error, "pending send exited: ", message.state);
                           state().process.pending = handle::start::pending::message();
                        }
                     }
                  }
               } // process

               void Error::operator () ( common::message::event::domain::Error& message)
               {
                  Trace trace{ "domain::manager::handle::event::Error"};

                  log::line( verbose::log, "message: ", message);

                  manager::task::event::dispatch( state(), [&message]()
                  {
                     return message;
                  });

                  if( message.severity == decltype( message.severity)::fatal && state().runlevel() == State::Runlevel::startup)
                  {
                     // We're in a 'fatal' state, and the only thing we can do is to shutdown
                     state().runlevel( State::Runlevel::error);
                     handle::shutdown( state());
                  }

               }

            } // event


            namespace process
            {
               namespace local
               {
                  namespace
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
                     } // lookup
                     struct Lookup : Base
                     {
                        using Base::Base;

                        bool operator () ( const common::message::domain::process::lookup::Request& message)
                        {
                           Trace trace{ "domain::manager::handle::process::local::Lookup"};

                           log::line( verbose::log, "message: ", message);

                           using Directive = common::message::domain::process::lookup::Request::Directive;

                           auto reply = common::message::reverse::type( message);
                           reply.identification = message.identification;

                           if( message.identification)
                           {
                              auto found = algorithm::find( state().singletons, message.identification);

                              if( found)
                              {
                                 reply.process = found->second;
                                 manager::ipc::send( state(), message.process, reply);
                              }
                              else if( message.directive == Directive::direct)
                              {
                                 manager::ipc::send( state(), message.process, reply);
                              }
                              else
                              {
                                 return false;
                              }
                           }
                           else if( message.pid)
                           {
                              reply.process = local::lookup::pid( state(), message.pid);

                              if( reply.process)
                                 manager::ipc::send( state(), message.process, reply);
                              else if( message.directive == Directive::direct)
                                 manager::ipc::send( state(), message.process, reply);
                              else
                                 return false;
                           }
                           else
                           {
                              // invalid
                              log::line( log::category::error, "invalid lookup request");
                           }
                           return true;
                        }
                     };

                     namespace singleton
                     {
                        void service( State& state, const common::process::Handle& process)
                        {
                           Trace trace{ "domain::manager::handle::local::singleton::service"};
                           
                           auto transform_service = []( const auto& s)
                           {
                              common::message::service::advertise::Service result;
                              result.name = s.name;
                              result.category = s.category;
                              result.transaction = s.transaction;
                              return result;
                           };

                           common::message::service::Advertise message;
                           message.process = common::process::handle();
                           message.services = algorithm::transform( manager::admin::services( state).services, transform_service);
                              
                           manager::ipc::send( state, process, message);

                           // so new spawned processes get it easier
                           environment::variable::process::set( 
                              environment::variable::name::ipc::service::manager(), process);
                        }

                        void tm( State& state, const common::process::Handle& process)
                        {
                           Trace trace{ "domain::manager::handle::local::singleton::tm"};

                           // so new spawned processes get it easier
                           environment::variable::process::set( 
                              environment::variable::name::ipc::transaction::manager(), process);
                        }

                        void queue( State& state, const common::process::Handle& process)
                        {
                           Trace trace{ "domain::manager::handle::local::singleton::queue"};

                           // so new spawned processes get it easier
                           environment::variable::process::set(
                                 environment::variable::name::ipc::queue::manager(), process);
                        }

                        void connect( State& state, const common::message::domain::process::connect::Request& message)
                        {
                           Trace trace{ "domain::manager::handle::local::singleton::connect"};

                           static const std::map< Uuid, std::function< void(State&, const common::process::Handle&)>> tasks{
                              { common::communication::instance::identity::service::manager, &service},
                              { common::communication::instance::identity::transaction::manager, &tm},
                              { common::communication::instance::identity::queue::manager, &queue}
                           };

                           auto found = algorithm::find( tasks, message.identification);

                           if( found)
                              found->second( state, message.process);
                        }

                     } // singleton
                  } // <unnamed>
               } // local

               void Connect::operator () ( common::message::domain::process::connect::Request& message)
               {
                  Trace trace{ "domain::manager::handle::process::Connect"};

                  common::log::line( verbose::log, "message: ", message);

                  auto reply = common::message::reverse::type( message);

                  auto send_reply = common::execute::scope( [&](){
                     manager::ipc::send( state(), message.process, reply);
                  });

                  if( message.identification)
                  {
                     auto found = algorithm::find( state().singletons, message.identification);

                     if( found)
                     {
                        // A "singleton" is trying to connect, while we already have one connected

                        log::line( log::category::error, "only one instance is allowed for ", message.identification);


                        reply.directive = decltype( reply)::Directive::singleton;

                        // Adjust configured instances to correspond to reality...
                        {
                           auto server = state().server( message.process.pid);

                           if( server)
                           {
                              server->remove( message.process.pid);
                              server->scale( 1);
                           }

                           auto executable = state().executable( message.process.pid);

                           if( executable)
                           {
                              executable->remove( message.process.pid);
                              executable->scale( 1);
                           }
                        }

                        manager::task::event::dispatch( state(), [&message]()
                        {
                           common::message::event::domain::Error event;
                           event.severity = decltype( event.severity)::warning;
                           event.message = string::compose( "server connect - only one instance is allowed for ", message.identification);
                           return event;
                        });
                        return;
                     }

                     state().singletons[ message.identification] = message.process;

                     local::singleton::connect( state(), message);
                  }

                  reply.directive = decltype( reply)::Directive::start;

                  auto server = state().server( message.process.pid);

                  if( server)
                  {
                     server->connect( message.process);
                     log::line( log, "added process: ", message.process, " to ", *server);
                  }
                  else 
                  {
                     // we assume it's a grandchild
                     state().grandchildren.push_back( message.process);
                  }

                  auto& pending = state().pending.lookup;

                  algorithm::trim( pending, algorithm::remove_if( pending, local::Lookup{ state()}));

                  manager::task::event::dispatch( state(), [&message]()
                  {
                     common::message::event::domain::server::Connect event;
                     event.process = message.process;
                     event.identification = message.identification;
                     return event;
                  });

               }

               void Lookup::operator () ( const common::message::domain::process::lookup::Request& message)
               {
                  Trace trace{ "domain::manager::handle::process::Lookup"};

                  if( ! local::Lookup{ state()}( message))
                     state().pending.lookup.push_back( message);
               }

            } // process

            namespace configuration
            {
               void Domain::operator () ( const common::message::domain::configuration::Request& message)
               {
                  Trace trace{ "domain::manager::handle::configuration::Domain"};

                  common::log::line( verbose::log, "message: ", message);

                  auto reply = common::message::reverse::type( message);
                  reply.domain = state().configuration;

                  manager::ipc::send( state(), message.process, reply);
               }

               void Server::operator () ( const common::message::domain::configuration::server::Request& message)
               {
                  Trace trace{ "domain::manager::handle::configuration::Server"};
                  common::log::line( verbose::log, "message: ", message);

                  auto reply = common::message::reverse::type( message);

                  reply.resources = state().resources( message.process.pid);

                  auto server = state().server( message.process.pid);

                  if( server)
                     reply.restrictions = server->restrictions;

                  manager::ipc::send( state(), message.process, reply);
               }
            } // configuration

            namespace local
            {
               namespace
               {
                  namespace server
                  {
                     using base_type = common::server::handle::policy::call::Admin;
                     struct Policy : base_type
                     {
                        using common::server::handle::policy::call::Admin::Admin;

                        Policy( manager::State& state)
                           :  m_state( state) {}

                        void configure( common::server::Arguments& arguments)
                        {
                           // no-op, we'll advertise our services when the broker comes online.
                        }

                        // overload ack so we use domain-manager internal stuff to lookup service-manager
                        void ack( const common::message::service::call::ACK& message)
                        {
                           Trace trace{ "domain::manager::handle::local::server::Policy::ack"};

                           try
                           {
                              auto service_manager = m_state.singleton( common::communication::instance::identity::service::manager);
                              communication::ipc::blocking::send( service_manager.ipc, message);
                           }
                           catch( const exception::system::communication::Unavailable&)
                           {
                              common::log::line( log, "service-manager is not online - action: discard sending ACK");
                           }
                        }

                     private: 
                        manager::State& m_state;
                     };

                     using Handle = common::server::handle::basic_call< Policy>;
                  } // server

                  namespace task
                  {
                     namespace event
                     {
                        auto done( State& state) 
                        {
                           return [&state]( common::message::event::domain::task::End& message)
                           {
                              Trace trace{ "domain::manager::handle::local::task::event::done"};
                              log::line( verbose::log, "message: ", message);

                              state.tasks.event( message);
                           };
                        }
                     } // event
                  } // task
    

               } // <unnamed>
            } // local
         } // handle

         handle::dispatch_type handler( State& state)
         {
            Trace trace{ "domain::manager::handler"};

            return {
               common::message::handle::defaults( ipc::device()),
               manager::handle::Shutdown{ state},
               manager::handle::scale::prepare::Shutdown{ state},
               manager::handle::event::process::Exit{ state},
               manager::handle::event::subscription::Begin{ state},
               manager::handle::event::subscription::End{ state},
               manager::handle::event::Error{ state},
               manager::handle::local::task::event::done( state),
               manager::handle::process::Connect{ state},
               manager::handle::process::Lookup{ state},
               manager::handle::configuration::Domain{ state},
               manager::handle::configuration::Server{ state},
               handle::local::server::Handle{
                  manager::admin::services( state),
                  state}
            };

         }

      } // manager
   } // domain
} // casual

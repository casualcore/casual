//! 
//! Copyright (c) 2015, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!


#include "domain/manager/handle.h"
#include "domain/manager/admin/server.h"
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

#include "common/communication/instance.h" 



namespace casual
{
   using namespace common;

   namespace domain
   {
      namespace manager
      {

         namespace ipc
         {
            const communication::ipc::Helper& device()
            {
               static communication::ipc::Helper singleton{
                  communication::error::handler::callback::on::Terminate{ &handle::event::process::exit}};

               return singleton;
            }

         } // ipc



         namespace local
         {
            namespace
            {
               namespace ipc
               {
                  namespace pending
                  {
                     void send( const State& state, message::pending::Message&& pending)
                     { 
                        manager::ipc::device().blocking_send( 
                           state.process.pending.handle().ipc, 
                           casual::domain::pending::message::Request{ std::move( pending)});
                     }
                     
                  } // pending
                  template< typename M>
                  void send( const State& state, const process::Handle& process, M&& message)
                  {
                     try
                     {
                        if( ! manager::ipc::device().non_blocking_send( process.ipc, message))
                           ipc::pending::send( state, message::pending::Message{ std::forward< M>( message), process});
                     }
                     catch( const exception::system::communication::Unavailable&)
                     {
                        log::line( log, "failed to send message - type: ", common::message::type( message), " to: ", process, " - action: ignore");
                     }
                  }

                  void send( const State& state, message::pending::Message&& pending)
                  {
                     if( ! message::pending::non::blocking::send( pending, manager::ipc::device().error_handler()))
                        ipc::pending::send( state, std::move( pending));
                  }

               } // ipc

               namespace scale
               {
                  template< typename E>
                  void out( State& state, E& executable)
                  {
                     Trace trace{ "domain::manager::handle::scale::out"};

                     auto spawnable = executable.spawnable();

                     if( ! spawnable)
                        return;

                     try
                     {
                        common::algorithm::for_each( spawnable, [&]( auto& i){
                           i.spawned( common::process::spawn(
                                 executable.path, executable.arguments, state.variables( executable)));
                        });

                        if( state.event.active< common::message::event::process::Spawn>())
                        {
                           common::message::event::process::Spawn message;
                           message.path = executable.path;
                           message.alias = executable.alias;

                           common::algorithm::transform( spawnable, message.pids, []( auto& i){
                              return common::process::id( i.handle);
                           });

                           manager::local::ipc::send( state, state.event( message));
                        }

                     }
                     catch( const exception::system::invalid::Argument& e)
                     {
                        log::line( log::category::error, "failed to spawn executable: ", executable, " - ", e);

                        common::algorithm::for_each( executable.spawnable(), []( auto& i){
                           i.state = state::instance::State::exit;
                        });

                        if( state.event.active< common::message::event::domain::Error>())
                        {
                           common::message::event::domain::Error message;
                           message.severity = common::message::event::domain::Error::Severity::error;
                           message.message = string::compose( "failed to spawn '", executable.path, "' - ", e);

                           manager::local::ipc::send( state, state.event( message));
                        }
                     }

                     log::line( verbose::log, "executable: ", executable);

                  }

                  void in( const State& state, const state::Executable& executable)
                  {
                     Trace trace{ "domain::manager::handle::scale::in Executable"};

                     auto shutdownable = executable.shutdownable();

                     if( ! shutdownable)
                        return;

                     // We only want child signals
                     signal::thread::scope::Mask mask{ signal::set::filled( signal::Type::child)};


                     auto pids = algorithm::transform( range::make_reverse( shutdownable), []( const auto& i){
                        return i.handle;
                     });

                     common::process::terminate( pids);
                  }

                  void in( State& state, const state::Server& server)
                  {
                     Trace trace{ "domain::manager::handle::scale::in Server"};

                     auto shutdownable = server.shutdownable();

                     if( ! shutdownable)
                        return;

                     // We only want child signals
                     signal::thread::scope::Mask mask{ signal::set::filled( signal::Type::child)};

                     // We need to correlate with the service-manager (broker), if it's up

                     common::message::domain::process::prepare::shutdown::Request prepare;
                     prepare.process = common::process::handle();

                     prepare.processes = algorithm::transform( range::make_reverse( shutdownable), []( const auto& i){
                        return i.handle;
                     });

                     auto service_manager = state.singleton( common::communication::instance::identity::service::manager);

                     try
                     {
                        manager::ipc::device().blocking_send( service_manager.ipc, prepare);
                     }
                     catch( const exception::system::communication::Unavailable&)
                     {
                        // service-manager is not online, we simulate the reply from the broker

                        auto reply = common::message::reverse::type( prepare);
                        reply.processes = std::move( prepare.processes);

                        handle::scale::prepare::Shutdown handle{ state};
                        handle( reply);
                     }
                   }

               } // scale

               namespace task
               {
                  struct base_batch
                  {
                     base_batch( State& state, state::Batch batch) : m_state( state), m_batch{ std::move( batch)} {}

                  protected:

                     State& state() { return m_state.get();}
                     const State& state() const { return m_state.get();}

                     std::reference_wrapper< State> m_state;
                     state::Batch m_batch;
                  };

                  struct Boot : base_batch
                  {
                     using base_batch::base_batch;

                     void start()
                     {
                        Trace trace{ "domain::manager::handle::task::Boot::start"};

                        algorithm::for_each( m_batch.executables, [&]( auto id){
                           scale::out( this->state(), this->state().executable( id));
                        });

                        algorithm::for_each( m_batch.servers, [&]( auto id){
                           scale::out( this->state(), this->state().server( id));
                        });


                        if( state().event.active< common::message::event::domain::Group>())
                        {
                           common::message::event::domain::Group event;
                           event.context = common::message::event::domain::Group::Context::boot_start;
                           event.id = m_batch.group.value();
                           event.name = state().group( m_batch.group).name;
                           event.process = common::process::handle();

                           manager::local::ipc::send( state(), state().event( event));
                        }
                     }

                     Boot( Boot&&) = default;
                     Boot& operator = ( Boot&&) = default;

                     ~Boot()
                     {
                        try
                        {
                           if( m_active && state().event.active< common::message::event::domain::Group>())
                           {
                              common::message::event::domain::Group event;
                              event.context = common::message::event::domain::Group::Context::boot_end;
                              event.id = m_batch.group.value();
                              event.name = state().group( m_batch.group).name;
                              event.process = common::process::handle();

                              manager::local::ipc::send( state(), state().event( event));
                           }
                        }
                        catch( ...)
                        {
                           exception::handle();
                        }
                     }

                     bool done() const
                     {
                        return algorithm::all_of( m_batch.servers, [&]( auto id){
                           auto& server = this->state().server( id);
                           return algorithm::none_of( server.instances, []( auto& i){
                              return i.state == state::Server::state_type::scale_out;
                           });
                        }) && algorithm::all_of( m_batch.executables, [&]( auto id){
                           auto& server = this->state().executable( id);
                           return algorithm::none_of( server.instances, []( auto& i){
                              return i.state == state::Server::state_type::scale_out;
                           });
                        });
                     }

                     friend std::ostream& operator << ( std::ostream& out, const Boot& value)
                     {
                        return out << "{ done: " << value.done() << ", batch: " << value.m_batch << '}';
                     }

                  private:
                     common::move::Active m_active;
                  };

                  struct Shutdown : base_batch
                  {
                     using base_batch::base_batch;

                     Shutdown( Shutdown&&) = default;
                     Shutdown& operator = ( Shutdown&&) = default;

                     ~Shutdown()
                     {
                        try
                        {
                           if( m_active && state().event.active< common::message::event::domain::Group>())
                           {
                              common::message::event::domain::Group event;
                              event.context = common::message::event::domain::Group::Context::shutdown_end;
                              event.id = m_batch.group.value();
                              event.name = state().group( m_batch.group).name;
                              event.process = common::process::handle();

                              manager::local::ipc::send( state(), state().event( event));
                           }
                        }
                        catch( ...)
                        {
                           exception::handle();
                        }
                     }

                     void start()
                     {
                        Trace trace{ "domain::manager::handle::task::Shutdown::start"};

                        algorithm::for_each( m_batch.executables, [&]( auto id){
                           state::Executable& e = this->state().executable( id);
                           e.scale( 0);
                           scale::in( this->state(), e);
                        });

                        algorithm::for_each( m_batch.servers, [&]( auto id){
                           state::Server& s = this->state().server( id);
                           s.scale( 0);
                           scale::in( this->state(), s);
                        });
                        if( state().event.active< common::message::event::domain::Group>())
                        {
                           common::message::event::domain::Group event;
                           event.context = common::message::event::domain::Group::Context::shutdown_start;
                           event.id = m_batch.group.value();
                           event.name = state().group( m_batch.group).name;
                           event.process = common::process::handle();

                           manager::local::ipc::send( state(), state().event( event));
                        }
                     }

                     bool done() const
                     {
                        return algorithm::all_of( m_batch.servers, [&]( auto id){
                           auto& server = this->state().server( id);
                           return algorithm::all_of( server.instances, []( auto& i){
                              return ! i.handle.pid;
                           });
                        }) && algorithm::all_of( m_batch.executables, [&]( auto id){
                           auto& server = this->state().executable( id);
                           return algorithm::all_of( server.instances, []( auto& i){
                              return ! i.handle;
                           });
                        });
                     }

                     friend std::ostream& operator << ( std::ostream& out, const Shutdown& value)
                     {
                        out << "{ done: " << value.done() << ", batch: ";
                        value.m_batch.log( out, value.state());
                        return out << '}';
                     }

                  private:
                     common::move::Active m_active;
                  };

                  namespace boot
                  {
                     struct Done : handle::Base
                     {
                        using handle::Base::Base;

                        void start()
                        {
                           state().runlevel( State::Runlevel::running);

                           if( state().event.active< message::event::domain::boot::End>())
                           {
                              message::event::domain::boot::End event;
                              event.domain = common::domain::identity();
                              event.process = common::process::handle();
                              manager::local::ipc::send( state(), state().event( event));
                           }
                        }

                        bool done() const { return true;}
                        bool started() const { return true;}

                        friend std::ostream& operator << ( std::ostream& out, const Done& value)
                        {
                           return out << "{ boot event - done}";
                        }
                     };

                  } // boot

                  namespace shutdown
                  {
                     struct Done : handle::Base
                     {
                        using handle::Base::Base;

                        void start()
                        {
                           state().runlevel( State::Runlevel::shutdown);

                           if( state().event.active< message::event::domain::shutdown::End>())
                           {
                              message::event::domain::shutdown::End event;
                              event.domain = common::domain::identity();
                              event.process = common::process::handle();
                              manager::local::ipc::send( state(), state().event( event));
                           }
                        }

                        bool done() const { return true;}
                        bool started() const { return true;}

                        friend std::ostream& operator << ( std::ostream& out, const Done& value)
                        {
                           return out << "{ shutdown event - done}";
                        }
                     };
                  } // shutdown

               } // task

            } // <unnamed>
         } // local
         namespace handle
         {
            namespace start
            {
               namespace pending
               {
                  common::Process message()
                  {
                     Trace trace{ "domain::manager::handle::start::pending::message"};

                     auto process = common::Process{ string::compose( "${CASUAL_HOME}/bin/", casual::domain::pending::message::environment::executable)};

                     // wait for connect
                     casual::domain::pending::message::Connect connect;
                     ipc::device().blocking_receive( connect);

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

               if( state.event.active< message::event::domain::boot::Begin>())
               {
                  message::event::domain::boot::Begin event;
                  event.domain = common::domain::identity();
                  manager::local::ipc::send( state, state.event( event));
               }

               algorithm::for_each( state.bootorder(), [&]( state::Batch& batch){
                     state.tasks.add( manager::local::task::Boot{ state, batch});
                  });

               if( state.event.active< message::event::domain::boot::End>())
               {
                  state.tasks.add( manager::local::task::boot::Done{ state});
               }
            }

            void shutdown( State& state)
            {
               Trace trace{ "domain::manager::handle::shutdown"};

               state.runlevel( State::Runlevel::shutdown);

               // TODO: collect state from sub-managers...
               if( state.persist)
               {
                  persistent::state::save( state);
               }

               if( state.event.active< message::event::domain::shutdown::Begin>())
               {
                  message::event::domain::shutdown::Begin event;
                  event.domain = common::domain::identity();
                  manager::local::ipc::send( state, state.event( event));
               }

               // Make sure we remove our self so we don't try to shutdown
               algorithm::for_each( state.servers, [&]( auto& s){
                  if( s.id == state.manager_id)
                  {
                     s.instances.clear();
                  }
               });

               algorithm::for_each( state.shutdownorder(), [&]( state::Batch& batch){
                     state.tasks.add( manager::local::task::Shutdown{ state, batch});
                  });

               state.tasks.add( manager::local::task::shutdown::Done{ state});

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


            namespace local
            {
               namespace
               {
                  namespace task
                  {
                     struct Shutdown : handle::Base
                     {
                        using handle::Base::Base;
                        void start()
                        {
                           handle::shutdown( state());
                        }

                        bool done() const { return true;}
                        bool started() const { return true;}

                        friend std::ostream& operator << ( std::ostream& out, const Shutdown& value)
                        {
                           return out << "{ shutdown task}";
                        }
                     };

                  } // task
               } // <unnamed>
            } // local

            void Shutdown::operator () ( common::message::shutdown::Request& message)
            {
               Trace trace{ "domain::manager::handle::Shutdown"};

               state().tasks.add( local::task::Shutdown( state()));
            }


            namespace scale
            {
               void instances( State& state, state::Server& server)
               {
                  manager::local::scale::in( state, server);
                  manager::local::scale::out( state, server);
               }

               void instances( State& state, state::Executable& executable)
               {
                  manager::local::scale::in( state, executable);
                  manager::local::scale::out( state, executable);
               }

               namespace prepare
               {
                  void Shutdown::operator () ( common::message::domain::process::prepare::shutdown::Reply& message)
                  {
                     Trace trace{ "domain::manager::handle::scale::prepare::shutdown::Reply"};

                     algorithm::for_each( message.processes, [&]( auto& process){

                        if( process)
                        {
                           message::shutdown::Request shutdown{ common::process::handle()};

                           // Just to make each shutdown easy to follow in log.
                           shutdown.execution = uuid::make();

                           manager::local::ipc::send( state(), process, shutdown);
                        }
                        else
                        {
                           common::process::terminate( process.pid);
                        }
                     });
                  }
               } // prepare
            } // scale

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
                     common::message::event::process::Exit event{ exit};
                     communication::ipc::inbound::device().push( std::move( event));
                  }

                  void Exit::operator () ( common::message::event::process::Exit& message)
                  {
                     Trace trace{ "domain::manager::handle::event::process::Exit"};

                     if( message.state.deceased())
                     {
                        // We don't want to handle any signals in this task
                        signal::thread::scope::Block block;

                        switch( message.state.reason)
                        {
                           case common::process::lifetime::Exit::Reason::core:
                           {
                              log::line( log::category::error, "process cored: ", message.state);
                              break;
                           }
                           default:
                           {
                              log::line( log::category::information, "process exited: ", message.state);
                              break;
                           }
                        }

                        auto restarts = state().remove( message.state.pid);

                        if( std::get< 0>( restarts)) scale::instances( state(), *std::get< 0>( restarts));
                        if( std::get< 1>( restarts)) scale::instances( state(), *std::get< 1>( restarts));

                        // Are there any listeners to this event?
                        if( state().event.active< common::message::event::process::Exit>())
                           manager::local::ipc::send( state(), state().event( message));


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

                  manager::local::ipc::send( state(), state().event( message));
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
                           {
                              return server->instance( pid).handle;
                           }
                           else
                           {
                              return state.grandchild( pid);
                           }
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
                                 manager::local::ipc::send( state(), message.process, reply);
                              }
                              else if( message.directive == Directive::direct)
                              {
                                 manager::local::ipc::send( state(), message.process, reply);
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
                              {
                                 manager::local::ipc::send( state(), message.process, reply);
                              }
                              else if( message.directive == Directive::direct)
                              {
                                 manager::local::ipc::send( state(), message.process, reply);
                              }
                              else
                              {
                                 return false;
                              }
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
                              
                           manager::local::ipc::send( state, process, message);

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
                     manager::local::ipc::send( state(), message.process, reply);
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

                        if( state().event.active< message::event::domain::Error>())
                        {
                           message::event::domain::Error event;
                           event.severity = message::event::domain::Error::Severity::warning;
                           event.message = string::compose( "server connect - only one instance is allowed for ", message.identification);

                           manager::local::ipc::send( state(), state().event( event));
                        }
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

                  if( state().event.active< message::event::domain::server::Connect>())
                  {
                     message::event::domain::server::Connect event;
                     event.process = message.process;
                     event.identification = message.identification;

                     manager::local::ipc::send( state(), state().event( event));
                  }

               }

               void Lookup::operator () ( const common::message::domain::process::lookup::Request& message)
               {
                  Trace trace{ "domain::manager::handle::process::Lookup"};

                  if( ! local::Lookup{ state()}( message))
                  {
                     state().pending.lookup.push_back( message);
                  }
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

                  manager::local::ipc::send( state(), message.process, reply);
               }

               void Server::operator () ( const common::message::domain::configuration::server::Request& message)
               {
                  Trace trace{ "domain::manager::handle::configuration::Server"};

                  common::log::line( verbose::log, "message: ", message);

                  auto reply = common::message::reverse::type( message);

                  reply.resources = state().resources( message.process.pid);

                  auto server = state().server( message.process.pid);

                  if( server)
                     reply.service.restrictions = server->restrictions;

                  auto transform_route = []( const auto& service)
                  {
                     message::domain::configuration::server::Reply::Service::Route result;
                     result.name = service.name;
                     result.routes = service.routes;
                     return result;
                  };
                  
                  algorithm::transform_if(
                     state().configuration.service.services,
                     reply.service.routes,
                     transform_route,
                     []( const auto& s) // predicate
                     { 
                        return ! s.routes.empty();
                     });

                  manager::local::ipc::send( state(), message.process, reply);

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

                        Policy( common::communication::error::type handler, manager::State& state)
                           : base_type( std::move( handler)), m_state( state) {}

                        void configure( common::server::Arguments& arguments)
                        {
                           // no-op, we'll advertise our services when the broker comes online.
                        }

                        // overload ack so we use domain-manager internal stuff to lookup service-manager
                        void ack( const message::service::call::ACK& message)
                        {
                           Trace trace{ "domain::manager::handle::local::server::Policy::ack"};

                           try
                           {
                              auto service_manager = m_state.singleton( common::communication::instance::identity::service::manager);
                              manager::ipc::device().blocking_send( service_manager.ipc, message);
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

               } // <unnamed>
            } // local


         } // handle

         handle::dispatch_type handler( State& state)
         {
            Trace trace{ "domain::manager::handler"};

            return {
               common::message::handle::ping(),
               manager::handle::Shutdown{ state},
               manager::handle::scale::prepare::Shutdown{ state},
               manager::handle::event::process::Exit{ state},
               manager::handle::event::subscription::Begin{ state},
               manager::handle::event::subscription::End{ state},
               manager::handle::event::Error{ state},
               manager::handle::process::Connect{ state},
               manager::handle::process::Lookup{ state},
               manager::handle::configuration::Domain{ state},
               manager::handle::configuration::Server{ state},
               handle::local::server::Handle{
                  manager::admin::services( state),
                  ipc::device().error_handler(), 
                  state}

            };

         }

      } // manager

   } // domain


} // casual

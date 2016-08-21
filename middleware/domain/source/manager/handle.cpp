//!
//! casual 
//!

#include "domain/manager/handle.h"
#include "domain/manager/admin/server.h"
#include "domain/common.h"
#include "domain/transform.h"




#include "common/message/handle.h"
#include "common/server/handle.h"
#include "common/cast.h"
#include "common/environment.h"


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
               static communication::ipc::Helper singleton{ communication::error::handler::callback::on::Terminate{ &handle::process::termination::event}};
               return singleton;
            }

         } // ipc

         namespace local
         {
            namespace
            {

               namespace ipc
               {
                  template< typename M>
                  void send( State& state, const process::Handle& process, M&& message)
                  {
                     try
                     {
                        if( ! manager::ipc::device().non_blocking_send( process.queue, message))
                        {
                           state.pending.replies.emplace_back( message, process.queue);
                        }
                     }
                     catch( const exception::communication::Unavailable&)
                     {
                        // no-op
                     }
                  }

               } // ipc

               namespace task
               {
                  struct base_batch
                  {
                     base_batch( state::Batch batch) : m_batch{ std::move( batch)} {}

                     void start()
                     {
                        Trace trace{ "domain::manager::handle::task::base_batch::start"};

                        message::domain::scale::Executable message;
                        range::transform( m_batch.executables, message.executables, []( const state::Executable& e){ return e.id;});

                        communication::ipc::inbound::device().push( std::move( message));
                     }


                  protected:
                     state::Batch m_batch;
                  };

                  struct Boot : base_batch
                  {
                     using base_batch::base_batch;

                     bool done() const
                     {
                        return m_batch.online();
                     }

                     friend std::ostream& operator << ( std::ostream& out, const Boot& value)
                     {
                        return out << "{ done: " << value.done() << ", batch: " << value.m_batch << '}';
                     }
                  };

                  struct Shutdown : base_batch
                  {
                     using base_batch::base_batch;

                     bool done() const
                     {
                        return m_batch.offline();
                     }

                     friend std::ostream& operator << ( std::ostream& out, const Shutdown& value)
                     {
                        return out << "{ done: " << value.done() << ", batch: " << value.m_batch << '}';
                     }
                  };

               } // task

            } // <unnamed>
         } // local
         namespace handle
         {

            namespace mandatory
            {
               namespace boot
               {
                  void prepare( State& state)
                  {
                     Trace trace{ "domain::manager::handle::mandatory::boot::prepare"};

                     {
                        state::Executable broker;
                        broker.alias = "casual-broker";
                        broker.path = "${CASUAL_HOME}/bin/casual-broker";
                        broker.configured_instances = 1;
                        broker.memberships.push_back( state.global.group);
                        broker.note = "service lookup and management";
                        //broker.restart = true;

                        state.executables.push_back( std::move( broker));
                     }

                     {
                        state::Executable tm;
                        tm.alias = "casual-transaction-manager";
                        tm.path = "${CASUAL_HOME}/bin/casual-transaction-manager";
                        tm.configured_instances = 1;
                        tm.memberships.push_back( state.global.group);
                        tm.note = "manage transaction in this domain";
                        //tm.restart = true;

                        state.executables.push_back( std::move( tm));
                     }

                     if( ! state.configuration.gateway.listeners.empty() || ! state.configuration.gateway.connections.empty())
                     {
                        state::Executable gateway;
                        gateway.alias = "casual-gateway-manager";
                        gateway.path = "${CASUAL_HOME}/bin/casual-gateway-manager";
                        gateway.configured_instances = 1;
                        gateway.memberships.push_back(  state.global.last);
                        gateway.note = "manage connections to and from other domains";

                        state.executables.push_back( std::move( gateway));
                     }
                  }
               } // boot
            } // mandatory


            void boot( State& state)
            {
               Trace trace{ "domain::manager::handle::boot"};


               range::for_each( state.bootorder(), [&]( state::Batch& batch){
                     state.tasks.add( local::task::Boot{ batch});
                  });
            }

            void shutdown( State& state)
            {
               Trace trace{ "domain::manager::handle::shutdown"};

               state.runlevel( State::Runlevel::shutdown);

               //
               // Make sure we remove our self so we don't try to shutdown
               //
               range::for_each( state.executables, []( state::Executable& e){
                  range::trim( e.instances, range::remove( e.instances, common::process::id()));
               });


               range::for_each( state.executables, []( state::Executable& e){
                  e.configured_instances = 0;
               });

               range::for_each( state.shutdownorder(), [&]( state::Batch& batch){
                     state.tasks.add( local::task::Shutdown{ batch});
                  });
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


            namespace scale
            {
               namespace
               {
                  void out( state::Executable& executable)
                  {
                     Trace trace{ "domain::manager::handle::scale::out"};
                     try
                     {
                        auto current = executable.instances.size();

                        while( current++ < executable.configured_instances)
                        {
                           auto pid = common::process::spawn( executable.path, executable.arguments, executable.environment.variables);
                           executable.instances.push_back( pid);
                        }
                     }
                     catch( const exception::invalid::Argument& e)
                     {
                        log::error << "failed to spawn executable: " << executable << " - " << e << '\n';
                     }
                  }

                  void in( const State& state, const state::Executable& executable)
                  {
                     Trace trace{ "domain::manager::handle::scale::in"};

                     assert( executable.configured_instances <= executable.instances.size());

                     //
                     // We only want child signals
                     //
                     signal::thread::scope::Mask mask{ signal::set::filled( { signal::Type::child})};

                     auto pids = range::make_reverse(
                           range::make(
                                 std::begin( executable.instances) + executable.configured_instances,
                                 std::end( executable.instances)));

                     std::vector< platform::pid::type> terminate;

                     range::for_each( pids, [&state, &terminate]( platform::pid::type pid){

                        auto found = range::find( state.processes, pid);

                        if( found)
                        {
                           try
                           {
                              message::shutdown::Request shutdown{ common::process::handle()};

                              //
                              // Just to make each shutdown easy to follow in log.
                              //
                              shutdown.execution = uuid::make();

                              ipc::device().non_blocking_send( found->second.queue, shutdown);
                           }
                           catch( const exception::queue::Unavailable&)
                           {
                              terminate.push_back( pid);
                           }
                        }
                        else
                        {
                           terminate.push_back( pid);
                        }
                     });

                     common::process::terminate( terminate);
                  }
               } //

               void Executable::operator () ( common::message::domain::scale::Executable& executable)
               {
                  Trace trace{ "domain::manager::handle::scale::Executable"};

                  log << "message: " << executable << '\n';

                  for( auto id : executable.executables)
                  {
                     auto found = range::find( state().executables, id);

                     if( found)
                     {
                        if( found->instances.size() < found->configured_instances)
                        {
                           scale::out( *found);
                        }
                        else if( found->instances.size() > found->configured_instances)
                        {
                           scale::in( state(), *found);
                        }
                     }
                     else
                     {
                        log << "failed to locate id: " << id << '\n';
                     }
                  }
               }

            } // scale


            namespace process
            {
               namespace termination
               {
                  void event( const common::process::lifetime::Exit& exit)
                  {
                     Trace trace{ "domain::manager::handle::process::termination::event"};

                     //
                     // We put a dead process event on our own ipc device, that
                     // will be handled later on.
                     //
                     common::message::domain::process::termination::Event event{ exit};
                     communication::ipc::inbound::device().push( std::move( event));
                  }

                  void Registration::operator () ( const common::message::domain::process::termination::Registration& message)
                  {
                     Trace trace{ "domain::manager::handle::process::termination::Registration"};

                     auto& listeners = state().termination.listeners;

                     auto found = range::find_if( listeners, common::process::Handle::equal::pid{ message.process.pid});
                     if( found)
                     {
                        *found = message.process;
                     }
                     else
                     {
                        listeners.push_back( message.process);
                     }

                     log << "termination.listeners: " << range::make( listeners);

                  }

                  void Event::operator () ( common::message::domain::process::termination::Event& message)
                  {
                     Trace trace{ "domain::manager::handle::process::termination::Event"};

                     if( message.death.deceased())
                     {
                        //
                        // We don't want to handle any signals in this task
                        //
                        signal::thread::scope::Block block;

                        switch( message.death.reason)
                        {
                           case common::process::lifetime::Exit::Reason::core:
                           {
                              log::error << "process cored: " << message.death << '\n';
                              break;
                           }
                           default:
                           {
                              log::information << "process exited: " << message.death << '\n';
                              break;
                           }
                        }


                        state().remove_process( message.death.pid);

                        auto& listeners = state().termination.listeners;


                        //
                        // We send event to all listeners
                        //
                        {
                           common::message::pending::Message pending{ message,
                              range::transform( listeners, std::mem_fn( &common::process::Handle::queue))};

                           if( ! common::message::pending::send( pending,
                                 communication::ipc::policy::non::Blocking{}))
                           {
                              state().pending.replies.push_back( std::move( pending));
                           }
                        }
                     }
                  }
               } // termination

               namespace local
               {
                  namespace
                  {
                     struct Lookup : Base
                     {
                        using Base::Base;

                        bool operator () ( const common::message::domain::process::lookup::Request& message)
                        {
                           auto reply = common::message::reverse::type( message);
                           reply.identification = message.identification;

                           if( message.identification)
                           {
                              auto found = range::find( state().singeltons, message.identification);

                              if( found)
                              {
                                 reply.process = found->second;
                                 manager::local::ipc::send( state(), message.process, reply);
                              }
                              else if( message.directive == common::message::domain::process::lookup::Request::Directive::direct)
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
                              auto found = range::find( state().processes, message.pid);

                              if( found)
                              {
                                 reply.process = found->second;
                                 manager::local::ipc::send( state(), message.process, reply);
                              }
                              else if( message.directive == common::message::domain::process::lookup::Request::Directive::direct)
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
                              log::error << "invalid lookup request: " << '\n';
                           }
                           return true;
                        }
                     };

                     namespace singleton
                     {
                        void broker( State& state, const common::process::Handle& process)
                        {
                           Trace trace{ "domain::manager::handle::local::singleton::broker"};

                           common::message::service::Advertise message;
                           message.process = common::process::handle();

                           message.services = range::transform( manager::admin::services( state).services,
                                 []( const common::server::Service& s)
                                 {
                                    common::message::service::advertise::Service result;

                                    result.name = s.origin;
                                    result.type = s.type;
                                    result.transaction = s.transaction;

                                    return result;
                                 });

                           manager::local::ipc::send( state, process, message);
                           environment::variable::set( environment::variable::name::ipc::broker(), process.queue);
                        }

                        void tm( State& state, const common::process::Handle& process)
                        {
                           Trace trace{ "domain::manager::handle::local::singleton::tm"};

                           environment::variable::set( environment::variable::name::ipc::transaction::manager(), process.queue);
                        }

                        void connect( State& state, const common::message::domain::process::connect::Request& message)
                        {
                           static const std::map< Uuid, std::function< void(State&, const common::process::Handle&)>> tasks{
                              { common::process::instance::identity::broker(), &broker},
                              { common::process::instance::identity::transaction::manager(), &tm}
                           };

                           auto found = range::find( tasks, message.identification);

                           if( found)
                           {
                              found->second( state, message.process);
                           }
                        }

                     } // singleton
                  } // <unnamed>
               } // local

               void Connect::operator () ( common::message::domain::process::connect::Request& message)
               {
                  Trace trace{ "domain::manager::handle::process::Connect"};

                  auto reply = common::message::reverse::type( message);

                  if( message.identification)
                  {
                     auto found = range::find( state().singeltons, message.identification);


                     if( found)
                     {
                        log::error << "domain::manager only one instance is allowed for " << message.identification << '\n';
                        //
                        // A "singleton" is trying to connect, while we already have one connected
                        //

                        reply.directive = decltype( reply)::Directive::singleton;
                        manager::local::ipc::send( state(), message.process, reply);
                        return;
                     }


                     state().singeltons[ message.identification] = message.process;

                     local::singleton::connect( state(), message);
                  }

                  reply.directive = decltype( reply)::Directive::start;
                  manager::local::ipc::send( state(), message.process, reply);



                  state().processes[ message.process.pid] = message.process;

                  auto& pending = state().pending.lookup;

                  range::trim( pending, range::remove_if( pending, local::Lookup{ state()}));

               }

               void Lookup::operator () ( const common::message::domain::process::lookup::Request& message)
               {
                  Trace trace{ "domain::manager::handle::process::Lookup"};

                  log << "message: " << message << '\n';

                  if( ! local::Lookup{ state()}( message))
                  {
                     state().pending.lookup.push_back( message);
                  }
               }


            } // process


            namespace configuration
            {
               namespace transaction
               {
                  void Resource::operator () ( const message::domain::configuration::transaction::resource::Request& message)
                  {
                     Trace trace{ "domain::manager::handle::configuration::transaction::Resource"};

                     auto reply = message::reverse::type( message);

                     if( message.scope == message::domain::configuration::transaction::resource::Request::Scope::specific)
                     {
                        auto& executable = state().executable( message.process.pid);

                        for( auto id : executable.memberships)
                        {
                           auto& group = state().group( id);
                           range::transform( group.resources, reply.resources, transform::configuration::transaction::Resource{});
                        }

                     }
                     else
                     {
                        for( auto& group : state().groups)
                        {
                           range::transform( group.resources, reply.resources, transform::configuration::transaction::Resource{});
                        }
                     }

                     log << "reply: " << reply << '\n';

                     manager::local::ipc::send( state(), message.process, reply);
                  }

               } // transaction


               void Gateway::operator () ( const common::message::domain::configuration::gateway::Request& message)
               {
                  Trace trace{ "domain::manager::handle::configuration:::Gateway"};

                  auto reply = config::gateway::transform::gateway( state().configuration.gateway);
                  reply.correlation = message.correlation;

                  log << "reply: " << reply << '\n';

                  manager::local::ipc::send( state(), message.process, reply);

               }


            } // configuration


            namespace local
            {
               namespace
               {
                  namespace server
                  {
                     struct Policy : common::server::handle::policy::Admin
                     {
                        using common::server::handle::policy::Admin::Admin;

                        void connect( std::vector< message::service::advertise::Service> services, const std::vector< transaction::Resource>& resources)
                        {
                           // no-op, we'll advertise our services when the broker comes online.
                        }

                     };

                     using Handle = common::server::handle::basic_call< Policy>;
                  } // server

               } // <unnamed>
            } // local


         } // handle

         common::message::dispatch::Handler handler( State& state)
         {
            Trace trace{ "domain::manager::handler"};

            return {
               common::message::handle::ping(),
               common::message::handle::Shutdown{},
               manager::handle::scale::Executable{ state},
               manager::handle::process::termination::Event{ state},
               manager::handle::process::termination::Registration{ state},
               manager::handle::process::Connect{ state},
               manager::handle::process::Lookup{ state},
               manager::handle::configuration::transaction::Resource{ state},
               manager::handle::configuration::Gateway{ state},
               handle::local::server::Handle{
                  manager::admin::services( state),
                  ipc::device().error_handler()}

            };

         }

      } // manager

   } // domain


} // casual

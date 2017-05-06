//!
//! casual 
//!

#include "domain/manager/handle.h"
#include "domain/manager/admin/server.h"
#include "domain/manager/persistent.h"
#include "domain/common.h"
#include "domain/transform.h"

#include "configuration/gateway.h"

#include "common/message/handle.h"
#include "common/server/handle/call.h"
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
                  template< typename M>
                  void send( State& state, const process::Handle& process, M&& message)
                  {
                     try
                     {
                        if( ! manager::ipc::device().non_blocking_send( process.queue, message))
                        {
                           state.pending.replies.emplace_back( std::forward< M>( message), process.queue);
                        }
                     }
                     catch( const exception::communication::Unavailable&)
                     {
                        log << "failed to send message - type: " << common::message::type( message) << " to: " << process << " - action: ignore\n";
                     }
                  }

                  void send( State& state, message::pending::Message&& pending)
                  {
                     if( ! message::pending::non::blocking::send( pending, manager::ipc::device().error_handler()))
                     {
                        state.pending.replies.push_back( std::move( pending));
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
                        using scale_type = message::domain::scale::Executable::Scale;
                        range::transform( m_batch.servers, message.servers, []( auto& task){ return scale_type{ task.id.underlaying(), task.instances};});
                        range::transform( m_batch.executables, message.executables, []( auto& task){ return scale_type{ task.id.underlaying(), task.instances};});

                        communication::ipc::inbound::device().push( std::move( message));
                     }


                  protected:
                     state::Batch m_batch;
                  };

                  struct Boot : base_batch
                  {
                     using base_batch::base_batch;

                     void start()
                     {
                        base_batch::start();

                        if( m_batch.state().event.active< common::message::event::domain::Group>())
                        {
                           common::message::event::domain::Group event;
                           event.context = common::message::event::domain::Group::Context::boot_start;
                           event.id = m_batch.group.underlaying();
                           event.name = m_batch.state().group( m_batch.group).name;

                           manager::local::ipc::send( m_batch.state(), m_batch.state().event( event));
                        }
                     }

                     Boot( Boot&&) = default;
                     Boot& operator = ( Boot&&) = default;

                     ~Boot()
                     {
                        try
                        {
                           if( ! m_moved && m_batch.state().event.active< common::message::event::domain::Group>())
                           {
                              common::message::event::domain::Group event;
                              event.context = common::message::event::domain::Group::Context::boot_end;
                              event.id = m_batch.group.underlaying();
                              event.name = m_batch.state().group( m_batch.group).name;

                              manager::local::ipc::send( m_batch.state(), m_batch.state().event( event));
                           }
                        }
                        catch( ...)
                        {
                           error::handler();
                        }
                     }

                     bool done() const
                     {
                        return m_batch.online();
                     }

                     friend std::ostream& operator << ( std::ostream& out, const Boot& value)
                     {
                        return out << "{ done: " << value.done() << ", batch: " << value.m_batch << '}';
                     }

                  private:
                     common::move::Moved m_moved;
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
                           if( ! m_moved && m_batch.state().event.active< common::message::event::domain::Group>())
                           {
                              common::message::event::domain::Group event;
                              event.context = common::message::event::domain::Group::Context::shutdown_end;
                              event.id = m_batch.group.underlaying();
                              event.name = m_batch.state().group( m_batch.group).name;

                              manager::local::ipc::send( m_batch.state(), m_batch.state().event( event));
                           }
                        }
                        catch( ...)
                        {
                           error::handler();
                        }
                     }

                     void start()
                     {
                        base_batch::start();

                        if( m_batch.state().event.active< common::message::event::domain::Group>())
                        {
                           common::message::event::domain::Group event;
                           event.context = common::message::event::domain::Group::Context::shutdown_start;
                           event.id = m_batch.group.underlaying();
                           event.name = m_batch.state().group( m_batch.group).name;

                           manager::local::ipc::send( m_batch.state(), m_batch.state().event( event));
                        }
                     }

                     bool done() const
                     {
                        return m_batch.offline();
                     }

                     friend std::ostream& operator << ( std::ostream& out, const Shutdown& value)
                     {
                        return out << "{ done: " << value.done() << ", batch: " << value.m_batch << '}';
                     }
                  private:
                     common::move::Moved m_moved;
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
                           if( state().event.active< message::event::domain::shutdown::End>())
                           {
                              message::event::domain::shutdown::End event;
                              event.domain = common::domain::identity();
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

            namespace mandatory
            {
               namespace boot
               {
                  void prepare( State& state)
                  {
                     Trace trace{ "domain::manager::handle::mandatory::boot::prepare"};

                     {
                        state::Server broker;
                        broker.alias = "casual-broker";
                        broker.path = "${CASUAL_HOME}/bin/casual-broker";
                        broker.configured_instances = 1;
                        broker.memberships.push_back( state.group_id.master);
                        broker.note = "service lookup and management";

                        state.servers.push_back( std::move( broker));
                     }

                     {
                        state::Server tm;
                        tm.alias = "casual-transaction-manager";
                        tm.path = "${CASUAL_HOME}/bin/casual-transaction-manager";
                        tm.configured_instances = 1;
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
                        queue.alias = "casual-queue-broker";
                        queue.path = "${CASUAL_HOME}/bin/casual-queue-broker";
                        queue.configured_instances = 1;
                        queue.memberships.push_back( state.group_id.queue);
                        queue.note = "manage queues in this domain";

                        state.servers.push_back( std::move( queue));
                     }

                     //if( ! state.configuration.gateway.listeners.empty() || ! state.configuration.gateway.connections.empty())
                     {
                        state::Server gateway;
                        gateway.alias = "casual-gateway-manager";
                        gateway.path = "${CASUAL_HOME}/bin/casual-gateway-manager";
                        gateway.configured_instances = 1;
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

               range::for_each( state.bootorder(), [&]( state::Batch& batch){
                     state.tasks.add( manager::local::task::Boot{ batch});
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

               //
               // TODO: collect state from sub-managers...
               //
               if( state.auto_persist)
               {
                  persistent::state::save( state);
               }

               if( state.event.active< message::event::domain::shutdown::Begin>())
               {
                  message::event::domain::shutdown::Begin event;
                  event.domain = common::domain::identity();
                  manager::local::ipc::send( state, state.event( event));
               }

               //
               // Make sure we remove our self so we don't try to shutdown
               //
               range::for_each( state.servers, [&]( auto& s){
                  if( s.id == state.manager_id)
                  {
                     s.instances.clear();
                  }
               });


               range::for_each( state.executables, []( auto& e){
                  e.configured_instances = 0;
               });

               range::for_each( state.servers, []( auto& s){
                  s.configured_instances = 0;
               });

               range::for_each( state.shutdownorder(), [&]( state::Batch& batch){
                     state.tasks.add( manager::local::task::Shutdown{ batch});
                  });

               if( state.event.active< message::event::domain::shutdown::End>())
               {
                  state.tasks.add( manager::local::task::shutdown::Done{ state});
               }
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
                  template< typename E>
                  void out( State& state, E& executable)
                  {
                     Trace trace{ "domain::manager::handle::scale::out"};
                     try
                     {
                        if( state.event.active< common::message::event::process::Spawn>())
                        {
                           common::message::event::process::Spawn message;
                           message.path = executable.path;
                           message.alias = executable.alias;

                           while( executable.instances.size() < executable.configured_instances)
                           {
                              auto pid = common::process::spawn( executable.path, executable.arguments, executable.environment.variables);

                              executable.instances.emplace_back( pid);
                              message.pids.push_back( pid);
                           }

                           manager::local::ipc::send( state, state.event( message));

                        }
                        else
                        {
                           while( executable.instances.size() < executable.configured_instances)
                           {
                              auto pid = common::process::spawn( executable.path, executable.arguments, executable.environment.variables);

                              executable.instances.emplace_back( pid);
                           }
                        }
                     }
                     catch( const exception::invalid::Argument& e)
                     {
                        log::category::error << "failed to spawn executable: " << executable << " - " << e << '\n';

                        //
                        // We can't spawn this executable, so we adjust the configured instances.
                        //
                        executable.configured_instances = executable.instances.size();
                     }
                  }

                  void in( const State& state, const state::Executable& executable)
                  {
                     Trace trace{ "domain::manager::handle::scale::in Executable"};

                     assert( executable.configured_instances <= executable.instances.size());

                     //
                     // We only want child signals
                     //
                     signal::thread::scope::Mask mask{ signal::set::filled( { signal::Type::child})};

                     auto pids = range::make_reverse(
                           range::make(
                                 std::begin( executable.instances) + executable.configured_instances,
                                 std::end( executable.instances)));

                     common::process::terminate( common::range::to_vector( pids));
                  }

                  void in( State& state, const state::Server& server)
                  {
                     Trace trace{ "domain::manager::handle::scale::in Server"};

                     assert( server.configured_instances <= server.instances.size());

                     //
                     // We only want child signals
                     //
                     signal::thread::scope::Mask mask{ signal::set::filled( { signal::Type::child})};


                     common::message::domain::process::prepare::shutdown::Request prepare;
                     prepare.process = common::process::handle();

                     auto handles = range::make_reverse(
                           range::make(
                                 std::begin( server.instances) + server.configured_instances,
                                 std::end( server.instances)));

                     prepare.processes = range::to_vector( handles);

                     auto broker = state.singleton( common::process::instance::identity::broker());

                     try
                     {
                        manager::ipc::device().blocking_send( broker.queue, prepare);
                     }
                     catch( const exception::communication::Unavailable&)
                     {
                        //
                        // broker is not online, we simulate the reply from the broker
                        //

                        auto reply = common::message::reverse::type( prepare);
                        reply.processes = std::move( prepare.processes);

                        scale::prepare::Shutdown handle{ state};
                        handle( reply);
                     }
                   }

               } // scale

               void Executable::operator () ( common::message::domain::scale::Executable& scale)
               {
                  Trace trace{ "domain::manager::handle::scale::Executable"};

                  log << "message: " << scale << '\n';

                  auto scaler = [&]( auto& tasks, auto& entities){
                       for( auto task : tasks)
                       {
                          using id_type = decltype( range::front( entities).id);

                          auto found = range::find( entities, id_type{ task.id});

                          if( found)
                          {
                             found->configured_instances = task.instances;

                             if( found->instances.size() < found->configured_instances)
                             {
                                scale::out( state(), *found);
                             }
                             else if( found->instances.size() > found->configured_instances)
                             {
                                scale::in( state(), *found);
                             }
                          }
                          else
                          {
                             log << "failed to locate id: " << task.id << '\n';
                          }
                       }
                  };

                  scaler( scale.servers, state().servers);
                  scaler( scale.executables, state().executables);
               }

               namespace prepare
               {
                  void Shutdown::operator () ( common::message::domain::process::prepare::shutdown::Reply& message)
                  {
                     Trace trace{ "domain::manager::handle::scale::prepare::shutdown::Reply"};

                     range::for_each( message.processes, [&]( auto& process){

                        if( process)
                        {
                           message::shutdown::Request shutdown{ common::process::handle()};

                           //
                           // Just to make each shutdown easy to follow in log.
                           //
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

                     log << "message: " << message << '\n';

                     state().event.subscription( message);

                     log << "event: " << state().event << '\n';
                  }

                  void End::operator () ( const common::message::event::subscription::End& message)
                  {
                     Trace trace{ "domain::manager::handle::event::subscription::End"};

                     log << "message: " << message << '\n';

                     state().event.subscription( message);

                     log << "event: " << state().event << '\n';
                  }

               } // subscription

               namespace process
               {
                  void exit( const common::process::lifetime::Exit& exit)
                  {
                     Trace trace{ "domain::manager::handle::event::process::exit"};

                     //
                     // We put a dead process event on our own ipc device, that
                     // will be handled later on.
                     //
                     common::message::event::process::Exit event{ exit};
                     communication::ipc::inbound::device().push( std::move( event));
                  }

                  void Exit::operator () ( common::message::event::process::Exit& message)
                  {
                     Trace trace{ "domain::manager::handle::event::process::Exit"};

                     if( message.state.deceased())
                     {
                        //
                        // We don't want to handle any signals in this task
                        //
                        signal::thread::scope::Block block;

                        switch( message.state.reason)
                        {
                           case common::process::lifetime::Exit::Reason::core:
                           {
                              log::category::error << "process cored: " << message.state << '\n';
                              break;
                           }
                           default:
                           {
                              log::category::information << "process exited: " << message.state << '\n';
                              break;
                           }
                        }

                        state().remove_process( message.state.pid);


                        //
                        // Are there any listeners to this event?
                        //
                        if( state().event.active< common::message::event::process::Exit>())
                        {
                           manager::local::ipc::send( state(), state().event( message));
                        }
                     }
                  }
               } // process
            } // event


            namespace process
            {
               namespace local
               {
                  namespace
                  {
                     struct Lookup : Base
                     {
                        using Base::Base;

                        bool operator () ( const common::message::domain::process::lookup::Request& message)
                        {
                           using Directive = common::message::domain::process::lookup::Request::Directive;

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
                              auto server = state().server( message.pid);

                              if( server)
                              {
                                 reply.process = server->process( message.pid);
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
                              log::category::error << "invalid lookup request: " << '\n';
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

                                    result.name = s.name;
                                    result.category = s.category;
                                    result.transaction = s.transaction;

                                    return result;
                                 });

                           manager::local::ipc::send( state, process, message);

                           environment::variable::process::set( environment::variable::name::ipc::broker(), process);
                        }

                        void tm( State& state, const common::process::Handle& process)
                        {
                           Trace trace{ "domain::manager::handle::local::singleton::tm"};

                           environment::variable::process::set( environment::variable::name::ipc::transaction::manager(), process);
                        }

                        void queue( State& state, const common::process::Handle& process)
                        {
                           Trace trace{ "domain::manager::handle::local::singleton::queue"};

                           environment::variable::process::set(
                                 environment::variable::name::ipc::queue::broker(), process);
                        }

                        void connect( State& state, const common::message::domain::process::connect::Request& message)
                        {
                           Trace trace{ "domain::manager::handle::local::singleton::connect"};

                           static const std::map< Uuid, std::function< void(State&, const common::process::Handle&)>> tasks{
                              { common::process::instance::identity::broker(), &broker},
                              { common::process::instance::identity::transaction::manager(), &tm},
                              { common::process::instance::identity::queue::broker(), &queue}
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

                  log << "message: " << message << '\n';

                  auto reply = common::message::reverse::type( message);

                  auto send_reply = common::scope::execute( [&](){
                     manager::local::ipc::send( state(), message.process, reply);
                  });



                  if( message.identification)
                  {
                     auto found = range::find( state().singeltons, message.identification);


                     if( found)
                     {
                        log::category::error << "domain::manager only one instance is allowed for " << message.identification << '\n';
                        //
                        // A "singleton" is trying to connect, while we already have one connected
                        //

                        reply.directive = decltype( reply)::Directive::singleton;

                        //
                        // Adjust configured instances to correspond to reality...
                        //
                        {
                           auto server = state().server( message.process.pid);

                           if( server)
                           {
                              server->remove( message.process.pid);
                              server->configured_instances = server->instances.size();
                           }

                           auto executable = state().executable( message.process.pid);

                           if( executable)
                           {
                              executable->remove( message.process.pid);
                              executable->configured_instances = executable->instances.size();
                           }
                        }

                        if( state().event.active< message::event::domain::Error>())
                        {
                           message::event::domain::Error event;
                           event.severity = message::event::domain::Error::Severity::warning;
                           event.message = "server connect - only one instance is allowed for " + uuid::string( message.identification);

                           manager::local::ipc::send( state(), state().event( event));
                        }
                        return;
                     }


                     state().singeltons[ message.identification] = message.process;

                     local::singleton::connect( state(), message);
                  }

                  reply.directive = decltype( reply)::Directive::start;

                  auto server = state().server( message.process.pid);

                  if( server)
                  {
                     server->connect( message.process);
                     log << "added process: " << message.process << " to " << *server << '\n';
                  }

                  auto& pending = state().pending.lookup;

                  range::trim( pending, range::remove_if( pending, local::Lookup{ state()}));

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

                  log << "message: " << message << '\n';

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

                  auto reply = common::message::reverse::type( message);
                  reply.domain = state().configuration;

                  manager::local::ipc::send( state(), message.process, reply);
               }


               void Server::operator () ( const common::message::domain::configuration::server::Request& message)
               {
                  Trace trace{ "domain::manager::handle::configuration::Server"};

                  auto reply = common::message::reverse::type( message);

                  reply.resources = state().resources( message.process.pid);

                  auto server = state().server( message.process.pid);

                  if( server)
                  {
                     reply.restrictions = server->restrictions;
                  }

                  using service_type = message::domain::configuration::service::Service;
                  range::transform_if(
                        state().configuration.service.services,
                        reply.routes,
                        []( const service_type& s){ // transform

                           message::domain::configuration::server::Reply::Service result;

                           result.name = s.name;
                           result.routes = s.routes;

                           return result;
                        },
                        []( const service_type& s){ // predicate
                           return s.routes.size() != 1 || s.routes[ 0] != s.name;
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
                     struct Policy : common::server::handle::policy::call::Admin
                     {
                        using common::server::handle::policy::call::Admin::Admin;

                        void configure( common::server::Arguments& arguments)
                        {
                           // no-op, we'll advertise our services when the broker comes online.
                        }

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
               common::message::handle::Shutdown{},
               manager::handle::scale::Executable{ state},
               manager::handle::scale::prepare::Shutdown{ state},
               manager::handle::event::process::Exit{ state},
               manager::handle::event::subscription::Begin{ state},
               manager::handle::event::subscription::End{ state},
               manager::handle::process::Connect{ state},
               manager::handle::process::Lookup{ state},
               manager::handle::configuration::Domain{ state},
               manager::handle::configuration::Server{ state},
               handle::local::server::Handle{
                  manager::admin::services( state),
                  ipc::device().error_handler()}

            };

         }

      } // manager

   } // domain


} // casual

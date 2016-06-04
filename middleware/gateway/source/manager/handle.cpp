

#include "gateway/manager/handle.h"
#include "gateway/manager/admin/server.h"
#include "gateway/environment.h"



#include "common/server/handle.h"
#include "common/message/handle.h"

#include "common/trace.h"
#include "common/environment.h"
#include "common/process.h"

namespace casual
{
   using namespace common;

   namespace gateway
   {
      namespace manager
      {
         namespace local
         {
            namespace
            {

            } // <unnamed>
         } // local

         namespace ipc
         {
            const common::communication::ipc::Helper& device()
            {
               static communication::ipc::Helper singleton{ communication::error::handler::callback::on::Terminate{ &handle::process::exit}};
               return singleton;
            }
         } // ipc

         namespace handle
         {

            namespace local
            {
               namespace
               {

                  namespace shutdown
                  {
                     struct Connection
                     {

                        template< typename C>
                        void operator () ( C& connection) const
                        {
                           Trace trace{ "gateway::manager::handle::local::shutdown::Connection", log::internal::gateway};

                           //
                           // We only want to handle terminate during this
                           //
                           common::signal::thread::scope::Mask mask{ signal::set::filled( { signal::Type::terminate})};

                           if( connection.running())
                           {
                              if( connection.process)
                              {
                                 log::internal::gateway << "send shutdown to connection: " << connection << std::endl;

                                 common::message::shutdown::Request request;
                                 request.process = common::process::handle();

                                 try
                                 {
                                    communication::ipc::outbound::Device ipc{ connection.process.queue};
                                    ipc.send( request, communication::ipc::policy::Blocking{});
                                 }
                                 catch( const exception::queue::Unavailable&)
                                 {
                                    connection.runlevel = state::base_connection::Runlevel::error;
                                    // no op, will be removed
                                 }
                              }
                              else if( connection.process.pid)
                              {
                                 log::internal::gateway << "terminate connection: " << connection << std::endl;
                                 common::process::lifetime::terminate( { connection.process.pid});
                                 connection.runlevel = state::base_connection::Runlevel::offline;
                              }
                           }
                        }

                     };


                  } // connection


                  std::string executable( const manager::state::outbound::Connection& connection)
                  {
                     switch( connection.type)
                     {
                        case manager::state::outbound::Connection::Type::ipc:
                        {
                           return common::environment::directory::casual() + "/bin/casual-gateway-outbound-ipc";
                        }
                        case manager::state::outbound::Connection::Type::tcp:
                        {
                           return common::environment::directory::casual() + "/bin/casual-gateway-outbound-tcp";
                        }
                        default:
                        {
                           throw exception::invalid::Argument{ "invalid connection type", CASUAL_NIP( connection)};
                        }
                     }
                  }

                  struct Boot
                  {
                     void operator () ( manager::state::outbound::Connection& connection) const
                     {
                        Trace trace{ "gateway::manager::handle::local::Boot", log::internal::gateway};

                        if( connection.runlevel == manager::state::outbound::Connection::Runlevel::absent)
                        {
                           try
                           {
                              connection.process.pid = common::process::spawn(
                                    local::executable( connection),
                                    { "--address", connection.address});

                              connection.runlevel = manager::state::outbound::Connection::Runlevel::booting;
                           }
                           catch( ...)
                           {
                              error::handler();
                              connection.runlevel = manager::state::outbound::Connection::Runlevel::error;
                           }
                        }
                        else
                        {
                           log::error << "boot connection: " << connection << " - wrong runlevel - action: ignore\n";
                        }
                     }

                  };

               } // <unnamed>
            } // local


            void shutdown( State& state)
            {
               Trace trace{ "gateway::manager::handle::shutdown", log::internal::gateway};

               //
               // We only want to handle child-signals during this stage
               //
               common::signal::thread::scope::Mask mask{ signal::set::filled( { signal::Type::child})};

               state.runlevel = State::Runlevel::shutdown;

               log::internal::gateway << "state: " << state << '\n';

               range::for_each( state.listeners, std::mem_fn( &Listener::shutdown));

               range::for_each( state.connections.inbound, local::shutdown::Connection{});
               range::for_each( state.connections.outbound, local::shutdown::Connection{});

               auto handler = manager::handler( state);

               while( state.running())
               {
                  log::internal::gateway << "state: " << state << '\n';

                  handler( ipc::device().next( communication::ipc::policy::Blocking{}));
               }
            }


            void boot( State& state)
            {
               Trace trace{ "gateway::manager::handle::boot", log::internal::gateway};

               range::for_each( state.connections.outbound, local::Boot{});
               range::for_each( state.listeners, std::mem_fn( &Listener::start));
            }


            Base::Base( State& state) : m_state( state) {}

            State& Base::state() { return m_state;}


            namespace listener
            {

               void Event::operator () ( message_type& message)
               {
                  Trace trace{ "gateway::manager::handle::listener::Event::operator()", log::internal::gateway};
                  log::internal::gateway << "message: " << message << '\n';

                  state().event( message);
               }

            } // listener

            namespace process
            {


               void exit( const common::process::lifetime::Exit& exit)
               {
                  Trace trace{ "gateway::manager::handle::process::exit", log::internal::gateway};

                  //
                  // We put a dead process event on our own ipc device, that
                  // will be handled later on.
                  //
                  common::message::domain::process::termination::Event event{ exit};

                  communication::ipc::inbound::device().push( std::move( event));
               }

               void Exit::operator () ( message_type& message)
               {
                  Trace trace{ "gateway::manager::handle::process::Exit", log::internal::gateway};


                  auto inbound_found = range::find( state().connections.inbound, message.death.pid);
                  auto outbound_found = range::find( state().connections.outbound, message.death.pid);

                  if( inbound_found)
                  {
                     log::information << "inbound connection terminated - connection: " << *inbound_found << std::endl;

                     state().connections.inbound.erase( std::begin( inbound_found));
                  }
                  else if( outbound_found)
                  {
                     log::information << "outbound connection terminated - connection: " << *outbound_found << std::endl;

                     state().connections.outbound.erase( std::begin( outbound_found));

                  }
                  else
                  {
                     log::error << "failed to correlate child termination - death: " << message.death << " - action: discard\n";
                  }
               }

            } // process


            namespace outbound
            {
               void Connect::operator () ( message_type& message)
               {
                  Trace trace{ "gateway::manager::handle::outbound::Connect", log::internal::gateway};

                  log::internal::gateway << "message: " << message << '\n';

                  auto found = range::find( state().connections.outbound, message.process.pid);

                  if( found)
                  {
                     if( found->runlevel == state::outbound::Connection::Runlevel::booting)
                     {
                        found->process = message.process;
                        found->remote = message.remote;
                        found->runlevel = state::outbound::Connection::Runlevel::online;
                     }
                     else
                     {
                        log::internal::gateway << "outbound connected is in wrong state: " << *found << " - action: discard\n";
                     }

                  }
                  else
                  {
                     log::error << "unknown outbound connected " << message << " - action: discard\n";
                  }
               }
            } // outbound

            namespace inbound
            {
               void Connect::operator () ( message_type& message)
               {
                  Trace trace{ "gateway::manager::handle::inbound::Connect", log::internal::gateway};

                  log::internal::gateway << "message: " << message << '\n';

                  auto found = range::find( state().connections.inbound, message.process.pid);

                  if( found)
                  {
                     found->process = message.process;
                     found->remote = message.remote;
                     found->runlevel = state::inbound::Connection::Runlevel::online;

                  }
                  else
                  {
                     log::error << "unknown inbound connected " << message << " - action: discard\n";
                  }
               }

               namespace ipc
               {
                  void Connect::operator () ( message_type& message)
                  {
                     Trace trace{ "gateway::manager::handle::inbound::ipc::Connect", log::internal::gateway};

                     //
                     // Another ipc-domain wants to talk to us
                     //

                     if( state().runlevel != State::Runlevel::shutdown )
                     {

                        state::inbound::Connection connection;
                        connection.runlevel = state::inbound::Connection::Runlevel::booting;
                        connection.type = state::inbound::Connection::Type::ipc;

                        connection.process.pid = common::process::spawn(
                              common::environment::directory::casual() + "/bin/casual-gateway-inbound-ipc",
                              {
                                    "--remote-name", message.remote.name,
                                    "--remote-id", uuid::string( message.remote.id),
                                    "--remote-ipc-queue", std::to_string( message.process.queue),
                                    "--correlation", uuid::string( message.correlation),
                              });


                        state().connections.inbound.push_back( std::move( connection));
                     }
                  }


               } // ipc

               namespace tcp
               {

                  void Connect::operator () ( message_type& message)
                  {
                     Trace trace{ "gateway::manager::handle::inbound::tcp::Connect", log::internal::gateway};

                     log::internal::gateway << "message: " << message << '\n';

                     //
                     // We take ownership of the socket until we've spawned the inbound connection
                     //
                     auto socket = communication::tcp::adopt( message.descriptor);


                     if( state().runlevel != State::Runlevel::shutdown )
                     {
                        state::inbound::Connection connection;
                        connection.type = state::inbound::Connection::Type::tcp;

                        connection.process.pid = common::process::spawn(
                              common::environment::directory::casual() + "/bin/casual-gateway-inbound-tcp",
                              {
                                    "--descriptor", std::to_string( socket.descriptor()),
                              });


                        state().connections.inbound.push_back( std::move( connection));

                        socket.release();
                     }
                  }

               } // tcp

            } // inbound

         } // handle

         common::message::dispatch::Handler handler( State& state)
         {
            static common::server::handle::basic_admin_call admin{
               manager::admin::services( state),
               ipc::device().error_handler()};

            return {
               common::message::handle::ping(),
               common::message::handle::Shutdown{},
               manager::handle::process::Exit{ state},
               manager::handle::listener::Event{ state},
               manager::handle::inbound::Connect{ state},
               manager::handle::outbound::Connect{ state},
               manager::handle::inbound::ipc::Connect{ state},
               manager::handle::inbound::tcp::Connect{ state},
               std::ref( admin),

            };

         }

      } // manager

   } // gateway


} // casual

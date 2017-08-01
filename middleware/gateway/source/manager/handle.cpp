//!
//! casual
//!

#include "gateway/manager/handle.h"
#include "gateway/manager/admin/server.h"
#include "gateway/environment.h"
#include "gateway/common.h"



#include "common/server/handle/call.h"
#include "common/message/handle.h"


#include "common/environment.h"
#include "common/process.h"
#include "common/cast.h"

namespace casual
{
   using namespace common;

   namespace gateway
   {
      namespace manager
      {

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
                  namespace optional
                  {
                     template< typename D, typename M>
                     bool send( D&& device, M&& message)
                     {
                        try
                        {
                           ipc::device().blocking_send( device, message);
                           return true;
                        }
                        catch( const common::exception::system::communication::Unavailable&)
                        {
                           return false;
                        }
                     }

                  } // optional

                  namespace shutdown
                  {
                     struct Connection
                     {

                        template< typename C>
                        void operator () ( C& connection) const
                        {
                           Trace trace{ "gateway::manager::handle::local::shutdown::Connection"};

                           //
                           // We only want to handle terminate during this
                           //
                           common::signal::thread::scope::Mask mask{ signal::set::filled( signal::Type::terminate)};

                           if( connection.running())
                           {
                              if( connection.process)
                              {
                                 log << "send shutdown to connection: " << connection << std::endl;

                                 common::message::shutdown::Request request;
                                 request.process = common::process::handle();

                                 try
                                 {
                                    communication::ipc::outbound::Device ipc{ connection.process.queue};
                                    ipc.send( request, communication::ipc::policy::Blocking{});
                                 }
                                 catch( const exception::system::communication::Unavailable&)
                                 {
                                    connection.runlevel = state::base_connection::Runlevel::error;
                                    // no op, will be removed
                                 }
                              }
                              else if( connection.process.pid)
                              {
                                 log << "terminate connection: " << connection << std::endl;
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
                           throw exception::system::invalid::Argument{ string::compose( "invalid connection type: ", connection)};
                        }
                     }
                  }

                  struct Boot
                  {
                     void operator () ( manager::state::outbound::Connection& connection) const
                     {
                        Trace trace{ "gateway::manager::handle::local::Boot"};


                        if( connection.runlevel == manager::state::outbound::Connection::Runlevel::absent)
                        {
                           try
                           {
                              connection.process.pid = common::process::spawn(
                                    local::executable( connection),
                                    { "--address", common::string::join( connection.address, " "),
                                      "--order", std::to_string( connection.order)});

                              connection.runlevel = manager::state::outbound::Connection::Runlevel::connecting;
                           }
                           catch( ...)
                           {
                              exception::handle();
                              connection.runlevel = manager::state::outbound::Connection::Runlevel::error;
                           }
                        }
                        else
                        {
                           log::category::error << "boot connection: " << connection << " - wrong runlevel - action: ignore\n";
                        }
                     }

                  };

               } // <unnamed>
            } // local


            void shutdown( State& state)
            {
               Trace trace{ "gateway::manager::handle::shutdown"};

               //
               // We only want to handle child-signals during this stage
               //
               common::signal::thread::scope::Mask mask{ signal::set::filled( signal::Type::child)};

               state.runlevel = State::Runlevel::shutdown;

               log << "state: " << state << '\n';

               range::for_each( state.listeners, std::mem_fn( &Listener::shutdown));

               range::for_each( state.connections.inbound, local::shutdown::Connection{});
               range::for_each( state.connections.outbound, local::shutdown::Connection{});

               auto handler = manager::handler( state);

               while( state.running())
               {
                  log << "state: " << state << '\n';

                  handler( ipc::device().next( communication::ipc::policy::Blocking{}));
               }
            }


            void boot( State& state)
            {
               Trace trace{ "gateway::manager::handle::boot"};

               range::for_each( state.connections.outbound, local::Boot{});
               range::for_each( state.listeners, std::mem_fn( &Listener::start));
            }


            Base::Base( State& state) : m_state( state) {}

            State& Base::state() { return m_state;}


            namespace listener
            {

               void Event::operator () ( message_type& message)
               {
                  Trace trace{ "gateway::manager::handle::listener::Event::operator()"};
                  log << "message: " << message << '\n';

                  state().event( message);
               }

            } // listener

            namespace process
            {


               void exit( const common::process::lifetime::Exit& exit)
               {
                  Trace trace{ "gateway::manager::handle::process::exit"};

                  //
                  // We put a dead process event on our own ipc device, that
                  // will be handled later on.
                  //
                  common::message::event::process::Exit event{ exit};
                  communication::ipc::inbound::device().push( std::move( event));
               }

               void Exit::operator () ( message_type& message)
               {
                  Trace trace{ "gateway::manager::handle::process::Exit"};

                  //
                  // Send the exit notification to domain.
                  //
                  ipc::device().blocking_send( communication::ipc::domain::manager::device(), message);

                  auto inbound_found = range::find( state().connections.inbound, message.state.pid);
                  auto outbound_found = range::find( state().connections.outbound, message.state.pid);

                  if( inbound_found)
                  {
                     log::category::information << "inbound connection terminated - connection: " << *inbound_found << std::endl;

                     state().connections.inbound.erase( std::begin( inbound_found));
                  }
                  else if( outbound_found)
                  {
                     log::category::information << "outbound connection terminated - connection: " << *outbound_found << std::endl;

                     if( outbound_found->restart && state().runlevel == State::Runlevel::online)
                     {
                        outbound_found->reset();
                        local::Boot{}( *outbound_found);
                     }
                     else
                     {
                        state().connections.outbound.erase( std::begin( outbound_found));
                     }

                     //
                     // remove discover coordination, if any.
                     //
                     state().discover.remove( message.state.pid);

                  }
                  else
                  {
                     log::category::error << "failed to correlate child termination - state: " << message.state << " - action: discard\n";
                  }
               }

            } // process


            namespace domain
            {
               namespace discover
               {

                  void Request::operator () ( message_type& message)
                  {
                     Trace trace{ "gateway::manager::handle::domain::discover::Request"};

                     log << "message: " << message << '\n';

                     std::vector< platform::pid::type> requested;

                     auto destination = message.process.queue;

                     //
                     // Make sure we get the response
                     //
                     message.process = common::process::handle();

                     //
                     // Forward the request to all outbound connections
                     //

                     auto send_request = [&]( const state::outbound::Connection& outbound)
                           {

                              //
                              // We don't send to the same domain that is the requester.
                              //
                              if( outbound.remote != message.domain)
                              {
                                 if( local::optional::send( outbound.process.queue, message))
                                 {
                                    requested.push_back( outbound.process.pid);
                                 }
                              }
                           };

                     range::for_each( state().connections.outbound, send_request);

                     state().discover.outbound.add( message.correlation, destination, std::move( requested));
                  }

                  void Reply::operator () ( message_type& message)
                  {
                     Trace trace{ "gateway::manager::handle::domain::discover::Reply"};

                     log << "message: " << message << '\n';

                     //
                     // Accumulate the reply, might trigger a accumulated reply to the requester
                     //
                     state().discover.outbound.accumulate( message);
                  }

               } // discovery
            } // domain

            namespace outbound
            {
               namespace configuration
               {

                  void Request::operator () ( message_type& message)
                  {
                     Trace trace{ "gateway::manager::handle::outbound::configuration::Request"};

                     log << "message: " << message << '\n';

                     auto reply = common::message::reverse::type( message);

                     auto send_reply = common::scope::execute( [&](){
                        local::optional::send( message.process.queue, reply);
                     });


                     auto found = range::find( state().connections.outbound, message.process.pid);

                     if( found)
                     {
                        reply.process = common::process::handle();
                        reply.services = found->services;
                        reply.queues = found->queues;
                     }
                     else
                     {
                        common::log::category::error << "failed to find connection for outbound::configuration::Request" << message << '\n';
                     }
                  }


               } // configuration

               void Connect::operator () ( message_type& message)
               {
                  Trace trace{ "gateway::manager::handle::outbound::Connect"};

                  log << "message: " << message << '\n';

                  auto found = range::find( state().connections.outbound, message.process.pid);

                  if( found)
                  {
                     found->process = message.process;
                     found->remote = message.domain;
                     found->address = std::move( message.address);

                     if( found->runlevel == state::outbound::Connection::Runlevel::connecting)
                     {
                        found->runlevel = state::outbound::Connection::Runlevel::online;
                     }
                     else
                     {
                        log::category::error << "outbound connected is in wrong state: " << *found << " - action: discard\n";
                     }
                  }
                  else
                  {
                     log::category::error << "unknown outbound connected " << message << " - action: discard\n";
                  }
               }
            } // outbound

            namespace inbound
            {
               void Connect::operator () ( message_type& message)
               {
                  Trace trace{ "gateway::manager::handle::inbound::Connect"};

                  log << "message: " << message << '\n';

                  auto found = range::find( state().connections.inbound, message.process.pid);

                  if( found)
                  {
                     found->process = message.process;
                     found->remote = message.domain;
                     found->address = std::move( message.address);
                     found->runlevel = state::inbound::Connection::Runlevel::online;

                     //
                     // It will soon arrive a discovery message, where we can pick up domain-id and such.
                     //
                  }
                  else
                  {
                     log::category::error << "unknown inbound connected " << message << " - action: discard\n";
                  }
               }

               namespace ipc
               {
                  void Connect::operator () ( message_type& message)
                  {
                     Trace trace{ "gateway::manager::handle::inbound::ipc::Connect"};

                     //
                     // Another ipc-domain wants to talk to us
                     //

                     if( state().runlevel != State::Runlevel::shutdown )
                     {

                        state::inbound::Connection connection;
                        connection.runlevel = state::inbound::Connection::Runlevel::connecting;
                        connection.type = state::inbound::Connection::Type::ipc;

                        connection.process.pid = common::process::spawn(
                              common::environment::directory::casual() + "/bin/casual-gateway-inbound-ipc",
                              {
                                    "--remote-ipc-queue", common::string::compose( message.process.queue),
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
                     Trace trace{ "gateway::manager::handle::inbound::tcp::Connect"};

                     log << "message: " << message << '\n';

                     //
                     // The socket (file-handler) is duplicated to the child process, so we can close
                     // the socket that belongs to this process
                     //
                     auto socket = communication::tcp::adopt( message.descriptor);


                     if( state().runlevel != State::Runlevel::shutdown)
                     {
                        state::inbound::Connection connection;
                        connection.runlevel = state::inbound::Connection::Runlevel::connecting;
                        connection.type = state::inbound::Connection::Type::tcp;

                        connection.process.pid = common::process::spawn(
                              common::environment::directory::casual() + "/bin/casual-gateway-inbound-tcp",
                              {
                                    "--descriptor", std::to_string( socket.descriptor()),
                                    "--limit-messages", std::to_string( message.limit.messages),
                                    "--limit-size", std::to_string( message.limit.size),
                              });

                        state().connections.inbound.push_back( std::move( connection));
                     }
                  }

               } // tcp

            } // inbound

         } // handle

         common::communication::ipc::dispatch::Handler handler( State& state)
         {
            static common::server::handle::admin::Call admin{
               manager::admin::services( state),
               ipc::device().error_handler()};

            return {
               common::message::handle::ping(),
               common::message::handle::Shutdown{},
               manager::handle::process::Exit{ state},
               manager::handle::listener::Event{ state},
               manager::handle::outbound::configuration::Request{ state},
               manager::handle::inbound::Connect{ state},
               manager::handle::outbound::Connect{ state},
               manager::handle::inbound::ipc::Connect{ state},
               manager::handle::inbound::tcp::Connect{ state},
               handle::domain::discover::Request{ state},
               handle::domain::discover::Reply{ state},
               std::ref( admin),

            };

         }

      } // manager

   } // gateway


} // casual

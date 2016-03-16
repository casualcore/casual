

#include "gateway/manager/handle.h"
#include "gateway/manager/admin/server.h"
#include "gateway/environment.h"



#include "common/server/handle.h"
#include "common/message/handle.h"

#include "common/trace.h"
#include "common/environment.h"


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

                  struct Shutdown
                  {

                     template< typename C>
                     bool operator () ( const C& connection) const
                     {
                        Trace trace{ "gateway::manager::handle::local::Shutdown", log::internal::gateway};

                        if( connection.process)
                        {
                           log::internal::gateway << "send shutdown to connection: " << connection << std::endl;

                           common::message::shutdown::Request request;
                           request.process = common::process::handle();

                           try
                           {
                              ipc::device().blocking_send( connection.process.queue, request);
                           }
                           catch( const exception::queue::Unavailable&)
                           {
                              return true;
                           }
                        }
                        else if( connection.process.pid)
                        {
                           log::internal::gateway << "terminate signal to connection: " << connection << std::endl;
                           signal::send( connection.process.pid, signal::Type::terminate);
                           return true;
                        }
                        return false;
                     }

                  };

                  std::string executable( const manager::state::outbound::Connection& connection)
                  {
                     switch( connection.type)
                     {
                        case manager::state::outbound::Connection::Type::ipc:
                        {
                           return common::environment::directory::casual() + "/bin/casual-gateway-outbound-ipc";
                        }
                        default:
                        {
                           throw exception::invalid::Argument{ "TODO: not implemented yet..."};
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
                           connection.process.pid = common::process::spawn(
                                 local::executable( connection),
                                 { "--address", connection.address});

                           connection.runlevel = manager::state::outbound::Connection::Runlevel::booting;
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

               state.runlevel = State::Runlevel::shutdown;

               state.connections.inbound = range::to_vector( range::remove_if( state.connections.inbound, local::Shutdown{}));
               state.connections.outbound = range::to_vector( range::remove_if( state.connections.outbound, local::Shutdown{}));

               auto handler = manager::handler( state);

               while( ! state.connections.inbound.empty() || ! state.connections.outbound.empty())
               {
                  log::internal::gateway << "inbound.size: " << state.connections.inbound.size()
                        << " - outbound.size: " << state.connections.outbound.size() << std::endl;

                  handler( ipc::device().blocking_next());
               }
            }


            void boot( State& state)
            {
               Trace trace{ "gateway::manager::handle::boot", log::internal::gateway};


               range::for_each( state.connections.outbound, local::Boot{});

            }


            Base::Base( State& state) : m_state( state) {}

            State& Base::state() { return m_state;}



            namespace process
            {


               void exit( const common::process::lifetime::Exit& exit)
               {
                  Trace trace{ "gateway::manager::handle::process::exit", log::internal::gateway};

                  //
                  // We put a dead process event on our own ipc device, that
                  // will be handled later on.
                  //
                  common::message::process::termination::Event event{ exit};

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
                     found->process = message.process;
                     found->remote = message.remote;
                     found->runlevel = state::outbound::Connection::Runlevel::online;

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

               } // ipc
            } // inbound

         } // handle

         common::message::dispatch::Handler handler( State& state)
         {
            static common::server::handle::basic_admin_call admin{
               ipc::device().device(),
               manager::admin::services( state),
               gateway::environment::identification(),
               ipc::device().error_handler()};

            return {
               common::message::handle::ping(),
               common::message::handle::Shutdown{},
               manager::handle::process::Exit{ state},
               manager::handle::inbound::Connect{ state},
               manager::handle::outbound::Connect{ state},
               manager::handle::inbound::ipc::Connect{ state},
               std::ref( admin),

            };

         }

      } // manager

   } // gateway


} // casual

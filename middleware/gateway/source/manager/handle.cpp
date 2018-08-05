//! 
//! Copyright (c) 2015, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!


#include "gateway/manager/handle.h"
#include "gateway/manager/admin/server.h"
#include "gateway/environment.h"
#include "gateway/common.h"



#include "common/server/handle/call.h"
#include "common/message/handle.h"
#include "common/communication/instance.h"

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
                                 log::line( log, "send shutdown to connection: ", connection);

                                 common::message::shutdown::Request request;
                                 request.process = common::process::handle();

                                 try
                                 {
                                    communication::ipc::outbound::Device ipc{ connection.process.ipc};
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
                                 log::line( log, "terminate connection: ", connection);

                                 // try to fetch handle from domain manager
                                 {
                                    auto handle = common::communication::instance::fetch::handle( 
                                       connection.process.pid, common::communication::instance::fetch::Directive::direct);

                                    log::line( log, "fetched handle: ", handle);

                                    if( handle)
                                    {
                                       connection.process = handle;
                                       operator() ( connection);
                                    }
                                    else 
                                    {
                                       common::process::lifetime::terminate( { connection.process.pid});
                                       connection.runlevel = state::base_connection::Runlevel::offline;
                                    }
                                 }
                              }
                           }
                        }

                     };
                  } // shutdown

                  std::string executable( const manager::state::outbound::Connection& connection)
                  {
                     return common::environment::directory::casual() + "/bin/casual-gateway-outbound";
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
                                    { "--address", connection.address.peer,
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
                           log::line( log::category::error, "boot connection: ", connection, " - wrong runlevel - action: ignore");
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

               log::line( verbose::log, "state: ", state);

               algorithm::for_each( state.connections.inbound, local::shutdown::Connection{});
               algorithm::for_each( state.connections.outbound, local::shutdown::Connection{});

               auto handler = manager::handler( state);

               while( state.running())
               {
                  log::line( verbose::log, "state: ", state);

                  handler( ipc::device().next( communication::ipc::policy::Blocking{}));
               }
            }


            void boot( State& state)
            {
               Trace trace{ "gateway::manager::handle::boot"};

               algorithm::for_each( state.connections.outbound, local::Boot{});
            }



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
                  ipc::device().blocking_send( communication::instance::outbound::domain::manager::device(), message);

                  auto inbound_found = algorithm::find( state().connections.inbound, message.state.pid);
                  auto outbound_found = algorithm::find( state().connections.outbound, message.state.pid);

                  if( inbound_found)
                  {
                     log::line( log::category::information, "inbound connection terminated");
                     log::line( verbose::log, "connection: ", *inbound_found);

                     state().connections.inbound.erase( std::begin( inbound_found));
                  }
                  else if( outbound_found)
                  {
                     log::line( log::category::information, "outbound connection terminated");
                     log::line( verbose::log, "connection: ", *outbound_found);

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
                     log::line( log::category::error, "failed to correlate child termination - state: ",  message.state, " - action: discard");
                  }
               }

            } // process

            namespace select
            {

               void Error::operator () ()
               {
                  Trace trace{ "gateway::manager::handle::select::Error"};

                  try
                  {
                     throw;
                  }
                  catch( const exception::signal::child::Terminate& exception)
                  {
                     auto terminated = common::process::lifetime::ended();
                     for( auto& exit : terminated)
                     {
                        common::message::event::process::Exit event{ exit};
                        process::Exit::operator()( event);
                     }
                  }
               }

            } // select

            namespace domain
            {
               namespace discover
               {

                  void Request::operator () ( message_type& message)
                  {
                     Trace trace{ "gateway::manager::handle::domain::discover::Request"};

                     log::line( verbose::log, "message: ", message);

                     std::vector< strong::process::id> requested;

                     auto destination = message.process.ipc;

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
                                 if( local::optional::send( outbound.process.ipc, message))
                                 {
                                    requested.push_back( outbound.process.pid);
                                 }
                              }
                           };

                     algorithm::for_each( state().connections.outbound, send_request);

                     state().discover.outbound.add( message.correlation, destination, std::move( requested));
                  }

                  void Reply::operator () ( message_type& message)
                  {
                     Trace trace{ "gateway::manager::handle::domain::discover::Reply"};

                     log::line( verbose::log, "message: ", message);

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

                     log::line( verbose::log, "message: ", message);

                     auto reply = common::message::reverse::type( message);

                     auto send_reply = common::execute::scope( [&](){
                        local::optional::send( message.process.ipc, reply);
                     });


                     auto found = algorithm::find( state().connections.outbound, message.process.pid);

                     if( found)
                     {
                        reply.process = common::process::handle();
                        reply.services = found->services;
                        reply.queues = found->queues;
                     }
                     else
                     {
                        log::line( common::log::category::error, "failed to find connection for outbound::configuration::Request: ", message);
                     }
                  }


               } // configuration

               void Connect::operator () ( message_type& message)
               {
                  Trace trace{ "gateway::manager::handle::outbound::Connect"};

                  log::line( verbose::log, "message: ", message);

                  auto found = algorithm::find( state().connections.outbound, message.process.pid);

                  if( found)
                  {
                     found->process = message.process;
                     found->remote = message.domain;
                     found->address.local = message.address.local;
                     found->address.peer = message.address.peer;

                     if( found->runlevel == state::outbound::Connection::Runlevel::connecting)
                     {
                        found->runlevel = state::outbound::Connection::Runlevel::online;
                     }
                     else
                     {
                        log::line( log::category::error, "outbound connected is in wrong state: ", *found, " - action: discard");
                     }
                  }
                  else
                  {
                     log::line( log::category::error, "unknown outbound connected ", message, " - action: discard");
                  }
               }
            } // outbound

            namespace inbound
            {
               void Connect::operator () ( message_type& message)
               {
                  Trace trace{ "gateway::manager::handle::inbound::Connect"};

                  log::line( verbose::log, "message: ", message);

                  auto found = algorithm::find( state().connections.inbound, message.process.pid);

                  if( found)
                  {
                     found->process = message.process;
                     found->remote = message.domain;
                     found->address.local = message.address.local;
                     found->address.peer = message.address.peer;
                     found->runlevel = state::inbound::Connection::Runlevel::online;

                     //
                     // It will soon arrive a discovery message, where we can pick up domain-id and such.
                     //
                  }
                  else
                  {
                     log::line( log::category::error, "unknown inbound connected ", message, " - action: discard");
                  }
               }
            } // inbound

            namespace listen
            {
               void Accept::read( descriptor_type descriptor)
               {
                  auto connection = state().accept( descriptor);

                  if( state().runlevel != State::Runlevel::shutdown && connection.socket)
                  {
                     state::inbound::Connection inbound;
                     inbound.runlevel = state::inbound::Connection::Runlevel::connecting;

                     inbound.process.pid = common::process::spawn(
                           common::environment::directory::casual() + "/bin/casual-gateway-inbound",
                           {
                                 "--descriptor", std::to_string( connection.socket.descriptor().value()),
                                 "--limit-messages", std::to_string( connection.limit.messages),
                                 "--limit-size", std::to_string( connection.limit.size),
                           });

                     state().connections.inbound.push_back( std::move( inbound));
                  }
               }
               
               const std::vector< Accept::descriptor_type>& Accept::descriptors() const
               {
                  return state().descriptors();
               }
            } // listen

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
               manager::handle::outbound::configuration::Request{ state},
               manager::handle::inbound::Connect{ state},
               manager::handle::outbound::Connect{ state},
               handle::domain::discover::Request{ state},
               handle::domain::discover::Reply{ state},
               std::ref( admin),

            };

         }

      } // manager

   } // gateway


} // casual

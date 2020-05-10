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

#include "common/event/send.h"
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
            common::communication::ipc::inbound::Device& device()
            {
               return common::communication::ipc::inbound::device();
            }


         } // ipc

         namespace handle
         {
            namespace local
            {
               namespace
               {

                  namespace coordinate
                  {
                     auto send = []( auto& ipc, auto& message)
                     {
                        return common::communication::ipc::blocking::optional::send( ipc, message);
                     };
                  } // coordinate

                  namespace shutdown
                  {
                     struct Connection
                     {

                        template< typename C>
                        void operator () ( C& connection) const
                        {
                           Trace trace{ "gateway::manager::handle::local::shutdown::Connection"};

                           // We only want to handle terminate during this
                           common::signal::thread::scope::Mask mask{ signal::set::filled( code::signal::terminate)};

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
                           log::line( log::category::error, "boot connection: ", connection, " - wrong runlevel - action: ignore");
                     }

                  };

               } // <unnamed>
            } // local

            void shutdown( State& state)
            {
               Trace trace{ "gateway::manager::handle::shutdown"};

               // We only want to handle child-signals during this stage
               common::signal::thread::scope::Mask mask{ signal::set::filled( code::signal::child)};

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

                  // We put a dead process event on our own ipc device, that
                  // will be handled later on.
                  common::message::event::process::Exit event{ exit};
                  communication::ipc::inbound::device().push( std::move( event));
               }

            } // process


            std::vector< common::Uuid> rediscover( State& state)
            {
               Trace trace{ "gateway::manager::handle::rediscover"};

               auto result = state.rediscover.tasks.add( state, "rediscover outbound connections");
               log::line( verbose::log, "result: ", result);

               if( ! result.request)
                  return {};

               // send the first request
               auto& destination = result.request.value().destination;
               communication::ipc::blocking::send( destination.ipc, result.request.value().message);

               // main task
               {
                  common::message::event::Task event{ common::process::handle()};
                  event.correlation = result.correlation;
                  event.description = result.description;
                  event.state = decltype( event.state)::started;
                  common::event::send( std::move( event));
               }

               // sub task
               if( auto outbound = state.outbound( destination.pid))
               {
                  common::message::event::sub::Task event{ common::process::handle()};
                  event.correlation = result.correlation;
                  event.description = "redescover " + outbound->remote.name;
                  event.state = decltype( event.state)::started;
                  common::event::send( std::move( event));
               }

               return { result.correlation};

            }

            namespace listen
            {
               void Accept::read( descriptor_type descriptor)
               {
                  auto connection = m_state.get().accept( descriptor);

                  if( m_state.get().runlevel != State::Runlevel::shutdown && connection.socket)
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

                     m_state.get().connections.inbound.push_back( std::move( inbound));
                  }
               }
               
               const std::vector< Accept::descriptor_type>& Accept::descriptors() const
               {
                  return m_state.get().descriptors();
               }
            } // listen

            namespace local
            {
               namespace
               {
                  namespace process
                  {
                     auto exit( State& state)
                     {  
                        return [&state]( const common::message::event::process::Exit& message)
                        {
                           Trace trace{ "gateway::manager::handle::local::process::exit"};

                           // Send the exit notification to domain.
                           communication::ipc::blocking::send( communication::instance::outbound::domain::manager::device(), message);

                           if( auto inbound_found = algorithm::find( state.connections.inbound, message.state.pid))
                           {
                              log::line( log::category::information, "inbound connection terminated");
                              log::line( verbose::log, "connection: ", *inbound_found);

                              state.connections.inbound.erase( std::begin( inbound_found));
                           }
                           else if( auto outbound_found = algorithm::find( state.connections.outbound, message.state.pid))
                           {
                              log::line( log::category::information, "outbound connection terminated");
                              log::line( verbose::log, "connection: ", *outbound_found);

                              if( outbound_found->restart && state.runlevel == State::Runlevel::online)
                              {
                                 outbound_found->reset();
                                 local::Boot{}( *outbound_found);
                              }
                              else
                                 state.connections.outbound.erase( std::begin( outbound_found));

                              // remove discover coordination, if any.
                              state.discover.outbound.remove( message.state.pid, coordinate::send);

                           }
                           else
                              log::line( log::category::error, "failed to correlate child termination - state: ",  message.state, " - action: discard");


                           // take care of pending rediscover
                           algorithm::for_each( state.rediscover.tasks.remove( message.state.pid), []( auto&& task)
                           {
                              // we're done, and can send end event (if caller is listening)
                              common::message::event::Task event{ common::process::handle()};
                              event.correlation = task.correlation;
                              event.description = std::move( task.description);
                              event.state = decltype( event.state)::error;
                              common::event::send( std::move( event));
                           });
                        };
                     }

                  } // process

                  namespace domain
                  {
                     namespace discover
                     {
                        auto request( State& state)
                        {
                           return [&state]( common::message::gateway::domain::discover::Request& message)
                           {
                              Trace trace{ "gateway::manager::handle::local::domain::discover::request"};
                              log::line( verbose::log, "message: ", message);

                              std::vector< strong::process::id> requested;

                              auto destination = message.process.ipc;

                              // Make sure we get the response
                              message.process = common::process::handle();

                              // Forward the request to all outbound connections

                              auto send_request = [&]( const state::outbound::Connection& outbound)
                              {
                                 // We don't send to the same domain that is the requester.
                                 if( outbound.remote != message.domain && communication::ipc::blocking::optional::send( outbound.process.ipc, message))
                                    requested.push_back( outbound.process.pid);
                                 
                              };

                              algorithm::for_each( state.connections.outbound, send_request);

                              state.discover.outbound.add( message.correlation, destination, coordinate::send, std::move( requested));
                           };
                        }

                        auto reply( State& state)
                        {
                           return [&state]( common::message::gateway::domain::discover::Reply& message)
                           {
                              Trace trace{ "gateway::manager::handle::local::domain::discover::reply"};
                              log::line( verbose::log, "message: ", message);

                              // Accumulate the reply, might trigger a accumulated reply to the requester
                              state.discover.outbound.accumulate( message, coordinate::send);
                           };
                        }
                     } // discovery

                     namespace rediscover
                     {
                        auto reply( State& state)
                        {
                           return [&state]( const message::outbound::rediscover::Reply& message)
                           {
                              Trace trace{ "gateway::manager::handle::local::domain::rediscover::reply"};
                              log::line( verbose::log, "message: ", message);

                              auto result = state.rediscover.tasks.reply( state, message);
                              log::line( verbose::log, "result: ", result);

                              // sub event
                              if( auto outbound = state.outbound( message.process.pid))
                              {
                                 common::message::event::sub::Task event{ common::process::handle()};
                                 event.correlation = result.correlation;
                                 event.description = "redescover " + outbound->remote.name;
                                 event.state = decltype( event.state)::done;
                                 common::event::send( std::move( event));
                              }

                              if( result.request)
                              {
                                 auto& destination = result.request.value().destination;
                                 communication::ipc::blocking::send( destination.ipc, result.request.value().message);

                                 if( auto outbound = state.outbound( destination.pid))
                                 {
                                    common::message::event::sub::Task event{ common::process::handle()};
                                    event.correlation = result.correlation;
                                    event.description = "redescover " + outbound->remote.name;
                                    event.state = decltype( event.state)::started;
                                    common::event::send( std::move( event));
                                 }
                              }
                              else
                              {
                                 // we're done, and can send end event (if caller is listening)
                                 common::message::event::Task event{ common::process::handle()};
                                 event.correlation = result.correlation;
                                 event.description = std::move( result.description);
                                 common::event::send( std::move( event));
                              }

                           };
                        }

                     } // rediscover
                  } // domain

                  namespace outbound
                  {
                     namespace configuration
                     {
                        auto request( State& state)
                        {
                           return [&state]( message::outbound::configuration::Request& message)
                           {
                              Trace trace{ "gateway::manager::handle::local::outbound::configuration::request"};
                              log::line( verbose::log, "message: ", message);

                              auto reply = common::message::reverse::type( message);

                              auto send_reply = common::execute::scope( [&](){
                                 communication::ipc::blocking::optional::send( message.process.ipc, reply);
                              });

                              if( auto found = algorithm::find( state.connections.outbound, message.process.pid))
                              {
                                 reply.process = common::process::handle();
                                 reply.services = found->services;
                                 reply.queues = found->queues;
                              }
                              else
                                 log::line( common::log::category::error, "failed to find connection for outbound::configuration::Request: ", message);
                           };
                        } 
                     } // configuration

                     auto connect( State& state)
                     {
                        return [&state]( message::outbound::Connect& message)
                        {
                           Trace trace{ "gateway::manager::handle::local::outbound::connect"};
                           log::line( verbose::log, "message: ", message);

                           auto found = algorithm::find( state.connections.outbound, message.process.pid);

                           if( found)
                           {
                              found->process = message.process;
                              found->remote = message.domain;
                              found->address.local = message.address.local;
                              found->address.peer = message.address.peer;

                              if( found->runlevel == state::outbound::Connection::Runlevel::connecting)
                                 found->runlevel = state::outbound::Connection::Runlevel::online;
                              else
                                 log::line( log::category::error, "outbound connected is in wrong state: ", *found, " - action: discard");
                           }
                           else
                              log::line( log::category::error, "unknown outbound connected ", message, " - action: discard");
                        };
                     }
                  } // outbound

                  namespace inbound
                  {
                     auto connect( State& state)
                     {
                        return [&state]( message::inbound::Connect& message)
                        {
                           Trace trace{ "gateway::manager::handle::local::inbound::connect"};
                           log::line( verbose::log, "message: ", message);

                           if( auto found = algorithm::find( state.connections.inbound, message.process.pid))
                           {
                              found->process = message.process;
                              found->remote = message.domain;
                              found->address.local = message.address.local;
                              found->address.peer = message.address.peer;
                              found->runlevel = state::inbound::Connection::Runlevel::online;
                           }
                           else
                              log::line( log::category::error, "unknown inbound connected ", message, " - action: discard");
                        };
                     }
                  } // inbound
               } // <unnamed>
            } // local
         } // handle

         common::communication::ipc::dispatch::Handler handler( State& state)
         {
            return ipc::device().handler(
               common::message::handle::defaults( ipc::device()),
               handle::local::process::exit( state),
               handle::local::outbound::configuration::request( state),
               handle::local::outbound::connect( state),
               handle::local::inbound::connect( state),
               handle::local::domain::discover::request( state),
               handle::local::domain::discover::reply( state),
               handle::local::domain::rediscover::reply( state),
               common::server::handle::admin::Call{ manager::admin::services( state)});
         }

      } // manager
   } // gateway
} // casual

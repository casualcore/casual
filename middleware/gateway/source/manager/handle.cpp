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

   namespace gateway::manager
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
                     return common::communication::device::blocking::optional::send( ipc, message);
                  };
               } // coordinate

               namespace shutdown
               {
                  // @returns false if 'connection' is _absent_, otherwise true and the
                  // shutdown was delivered
                  auto connection()
                  {
                     return []( auto& connection)
                     {
                        Trace trace{ "gateway::manager::handle::local::shutdown::connection"};

                        // We only want to handle terminate during this
                        common::signal::thread::scope::Mask mask{ signal::set::filled( code::signal::terminate)};

                        if( connection.running())
                        {
                           if( connection.process)
                           {
                              log::line( log, "send shutdown to connection: ", connection);

                              common::message::shutdown::Request request;
                              request.process = common::process::handle();
                              
                              // if unavailable we remove the connection.
                              return ! communication::device::blocking::optional::send( connection.process.ipc, request).empty();
         
                           }
                           else if( connection.process.pid)
                           {
                              log::line( log, "terminate connection: ", connection);

                              // if unavailable we remove the connection.
                              return common::process::terminate( connection.process.pid);
                           }
                        }
                        return true;
                     };
                  }

                  auto reverse()
                  {
                     return []( auto& reverse)
                     {
                        Trace trace{ "gateway::manager::handle::local::shutdown::reverse"};

                        // We only want to handle terminate during this
                        common::signal::thread::scope::Mask mask{ signal::set::filled( code::signal::terminate)};

                        if( reverse.process)
                        {
                           log::line( log, "send shutdown to reverse: ", reverse);

                           common::message::shutdown::Request request;
                           request.process = common::process::handle();
                           
                           communication::device::blocking::optional::send( reverse.process.ipc, request).empty();
      
                        }
                        else if( reverse.process.pid)
                        {
                           log::line( log, "terminate reverse: ", reverse);

                           signal::send( reverse.process.pid, code::signal::terminate);
                        }
                     };
                  }

               } // shutdown

               std::string executable( std::string_view alias)
               {
                  return string::compose( directory::name::base( common::process::path()), '/', alias);
               }

               namespace spawn
               {
                  strong::process::id process( std::string_view alias, const std::vector< std::string>& arguments)
                  {
                        auto path = executable( alias);

                     try
                     {                          
                        auto pid = common::process::spawn(
                           path,
                           arguments);

                        // Send event
                        {
                           common::message::event::process::Spawn event{ common::process::handle()};
                           event.path = path;
                           event.alias = alias;
                           event.pids.push_back( pid);
                           common::event::send( event);
                        }

                        return pid;
                        
                     }
                     catch( ...)
                     {
                        exception::handle( log::category::error, "spawn process ", path);
                     }
                     return {};
                  }
               }

               auto boot()
               {
                  return []( auto& connection)
                  {
                     Trace trace{ "gateway::manager::handle::local::Boot"};

                     if( connection.runlevel == decltype( connection.runlevel)::absent)
                     {
                        connection.process.pid = spawn::process( "casual-gateway-outbound", 
                           { "--address", connection.address.peer,
                              "--order", std::to_string( connection.order)});

                        if( connection.process.pid)
                           connection.runlevel = decltype( connection.runlevel)::connecting;
                        else
                           connection.runlevel = decltype( connection.runlevel)::error;

                     }
                     else
                        log::line( log::category::error, "boot connection: ", connection, " - wrong runlevel - action: ignore");
                  };

               };

               namespace rediscover
               {
                  auto result( State& state, state::coordinate::outbound::rediscover::Tasks::Result result)
                  {
                     Trace trace{ "gateway::manager::handle::local::rediscover::result"};
                     log::line( verbose::log, "result: ", result);

                     auto send_end_event = []( auto& result)
                     {
                        // we're done, and can send end event (if caller is listening)
                        common::message::event::Task event{ common::process::handle()};
                        event.correlation = result.correlation;
                        event.description = std::move( result.description);
                        event.state = decltype( event.state)::done;
                        common::event::send( std::move( event));
                     };

                     if( ! result.request)
                     {
                        // nothing left to do, we send end-event.
                        send_end_event( result);
                        return result.correlation;
                     }

                     auto& destination = result.request.value().destination;

                     // outbound might not be available
                     if( communication::device::blocking::optional::send( destination.ipc, result.request.value().message))
                     {
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
                        // The outbound is not available for some reason. Remove the task...
                        state.rediscover.tasks.remove( destination.pid);

                        if( state.rediscover.tasks.empty())
                           send_end_event( result);
                     }

                     return result.correlation;
            
                  };
               } // rediscover

               namespace spawn
               {
                  namespace reverse
                  {
                     auto outbound()
                     {
                        return []( auto& outbound)
                        {
                           outbound.process.pid = spawn::process( outbound.alias, 
                              {});
                              //{ "--order", std::to_string( outbound.order)});
                        };
                     }

                     auto inbound()
                     {
                        return []( auto& inbound)
                        {
                           inbound.process.pid = spawn::process( inbound.alias, 
                              {});
                        };
                     }

                  } // reverse
               } // spawn

            } // <unnamed>
         } // local

         void shutdown( State& state)
         {
            Trace trace{ "gateway::manager::handle::shutdown"};

            // We only want to handle child-signals during this stage
            common::signal::thread::scope::Mask mask{ signal::set::filled( code::signal::child)};

            state.runlevel = State::Runlevel::shutdown;

            log::line( verbose::log, "state: ", state);

            algorithm::trim( state.connections.inbound, algorithm::filter( state.connections.inbound, local::shutdown::connection()));
            algorithm::trim( state.connections.outbound, algorithm::filter( state.connections.outbound, local::shutdown::connection()));


            algorithm::for_each( state.reverse.inbounds, local::shutdown::reverse());
            algorithm::for_each( state.reverse.outbounds, local::shutdown::reverse());

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

            algorithm::for_each( state.connections.outbound, local::boot());

            algorithm::for_each( state.reverse.outbounds, local::spawn::reverse::outbound());
            algorithm::for_each( state.reverse.inbounds, local::spawn::reverse::inbound());

            // make sure we got connected stuff before we continue
            common::message::dispatch::relaxed::pump(
               common::message::dispatch::condition::compose(
                  common::message::dispatch::condition::done( [&state]()
                  {
                     auto connected_reverse = []( auto& reverse)
                     {
                        return predicate::boolean( reverse.process);
                     };

                     return algorithm::all_of( state.reverse.outbounds, connected_reverse)
                        && algorithm::all_of( state.reverse.inbounds, connected_reverse);
                  })
               ),
               manager::handler( state),
               ipc::device());
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


         common::Uuid rediscover( State& state)
         {
            Trace trace{ "gateway::manager::handle::rediscover"};

            auto result = state.rediscover.tasks.add( state, "rediscover outbound connections");

            // main task
            {
               common::message::event::Task event{ common::process::handle()};
               event.correlation = result.correlation;
               event.description = result.description;
               event.state = decltype( event.state)::started;
               common::event::send( std::move( event));
            }

            return local::rediscover::result( state, std::move( result));
         }

         namespace listen
         {
            void Accept::read( descriptor_type descriptor)
            {
               Trace trace{ "gateway::manager::handle::listen::Accept::read"};
               log::line( verbose::log, "descriptor: ", descriptor);

               auto connection = m_state.get().accept( descriptor);

               if( m_state.get().runlevel != State::Runlevel::shutdown && connection.socket)
               {
                  state::inbound::Connection inbound;
                  inbound.runlevel = state::inbound::Connection::Runlevel::connecting;
                  auto path = common::environment::directory::casual() + "/bin/casual-gateway-inbound";

                  inbound.process.pid = common::process::spawn(
                     path,
                     {
                        "--descriptor", std::to_string( connection.socket.descriptor().value()),
                        "--limit-messages", std::to_string( connection.limit.messages),
                        "--limit-size", std::to_string( connection.limit.size),
                     });

                  // Send event to domain.
                  {
                     common::message::event::process::Spawn event{ common::process::handle()};
                     event.path = path;
                     event.alias = file::name::base( event.path);
                     event.pids.push_back( inbound.process.pid);
                     common::event::send( event);
                  }

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

                        const auto pid = message.state.pid;

                        // Send the exit notification to domain.
                        communication::device::blocking::send( communication::instance::outbound::domain::manager::device(), message);

                        if( auto found = algorithm::find( state.connections.inbound, pid))
                        {
                           log::line( log::category::information, "inbound connection terminated");
                           log::line( verbose::log, "connection: ", *found);

                           state.connections.inbound.erase( std::begin( found));
                        }
                        
                        if( auto found = algorithm::find( state.connections.outbound, pid))
                        {
                           log::line( log::category::information, "outbound connection terminated");
                           log::line( verbose::log, "connection: ", *found);

                           if( found->restart && state.runlevel == State::Runlevel::online)
                           {
                              found->reset();
                              local::boot()( *found);
                           }
                           else
                              state.connections.outbound.erase( std::begin( found));

                           // remove discover coordination, if any.
                           state.discover.outbound.remove( pid, coordinate::send);

                        }

                        algorithm::trim( state.reverse.outbounds, algorithm::remove(  state.reverse.outbounds, pid));
                        algorithm::trim( state.reverse.inbounds, algorithm::remove( state.reverse.inbounds, pid));


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
                              if( outbound.remote != message.domain && communication::device::blocking::optional::send( outbound.process.ipc, message))
                                 requested.push_back( outbound.process.pid);
                              
                           };

                           algorithm::for_each( state.connections.outbound, send_request);

                           // take care of reverse outbounds
                           algorithm::for_each( state.reverse.outbounds, [&]( auto& outbound)
                           {
                              if( outbound && communication::device::blocking::optional::send( outbound.process.ipc, message))
                                 requested.push_back( outbound.process.pid);
                           });

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

                           // sub event
                           if( auto outbound = state.outbound( message.process.pid))
                           {
                              common::message::event::sub::Task event{ common::process::handle()};
                              event.correlation = result.correlation;
                              event.description = "redescover " + outbound->remote.name;
                              event.state = decltype( event.state)::done;
                              common::event::send( std::move( event));
                           }

                           local::rediscover::result( state, std::move( result));
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
                              communication::device::blocking::optional::send( message.process.ipc, reply);
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

                        if( auto found = algorithm::find( state.connections.outbound, message.process.pid))
                        {
                           found->process = message.process;
                           found->remote = message.domain;
                           found->address.local = message.address.local;
                           found->address.peer = message.address.peer;

                           using Enum = decltype( found->runlevel);

                           if( found->runlevel == Enum::connecting)
                              found->runlevel = Enum::online;
                           else
                              log::line( verbose::log, "outbound connected is in wrong state: ", *found, " - action: discard");
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

               namespace reverse
               {
                  namespace outbound
                  {
                     auto connect( State& state)
                     {
                        return [&state]( message::reverse::outbound::Connect& message)
                        {
                           Trace trace{ "gateway::manager::handle::local::reverse::outbound::connect"};
                           log::line( verbose::log, "message: ", message);

                           if( auto found = algorithm::find( state.reverse.outbounds, message.process.pid))
                           {
                              found->process = message.process;

                              message::reverse::outbound::configuration::update::Request request{ common::process::handle()};
                              request.model = found->configuration;
                              request.order = found->order;

                              communication::device::blocking::optional::send( message.process.ipc, request);                                 
                           }
                           else
                              log::line( log::category::error, code::casual::internal_correlation, " failed to correlate reverse outbound pid ", message.process.pid, " - action: ignore");
                        };
                     }

                     namespace configuration::update
                     {
                        auto reply( State& state)
                        {
                           return []( message::reverse::outbound::configuration::update::Reply& message)
                           {
                              Trace trace{ "gateway::manager::handle::local::reverse::outbound::configuration::update"};
                              log::line( verbose::log, "message: ", message);

                              log::line( log::category::information, "reverse outbound configured");
                           };
                        }
                        
                     } // configuration::update
                  } // outbound

                  namespace inbound
                  {
                     auto connect( State& state)
                     {
                        return [&state]( message::reverse::inbound::Connect& message)
                        {
                           Trace trace{ "gateway::manager::handle::local::reverse::inbound::connect"};
                           log::line( verbose::log, "message: ", message);

                           if( auto found = algorithm::find( state.reverse.inbounds, message.process.pid))
                           {
                              found->process = message.process;

                              message::reverse::inbound::configuration::update::Request request{ common::process::handle()};
                              request.model = found->configuration;
                              communication::device::blocking::optional::send( message.process.ipc, request);                                 
                           }
                           else
                              log::line( log::category::error, code::casual::internal_correlation, " failed to correlate reverse inbound pid ", message.process.pid, " - action: ignore");
                        };
                     }


                     namespace configuration::update
                     {
                        auto reply( State& state)
                        {
                           return []( message::reverse::inbound::configuration::update::Reply& message)
                           {
                              Trace trace{ "gateway::manager::handle::local::reverse::inbound::configuration::update"};
                              log::line( verbose::log, "message: ", message);

                              log::line( log::category::information, "reverse inbound configured");
                           };
                        }
                        
                     } // configuration::update

                  } // inbound
               } // reverse
            } // <unnamed>
         } // local
      } // handle

      handle::dispatch_type handler( State& state)
      {
         static common::server::handle::admin::Call call{ manager::admin::services( state)};

         return common::message::dispatch::handler( ipc::device(),
            common::message::handle::defaults( ipc::device()),
            handle::local::process::exit( state),
            handle::local::outbound::configuration::request( state),
            handle::local::outbound::connect( state),
            handle::local::inbound::connect( state),
            handle::local::domain::discover::request( state),
            handle::local::domain::discover::reply( state),
            handle::local::domain::rediscover::reply( state),

            handle::local::reverse::outbound::connect( state),
            handle::local::reverse::outbound::configuration::update::reply( state),
            handle::local::reverse::inbound::connect( state),
            handle::local::reverse::inbound::configuration::update::reply( state),

            std::ref( call));
      }

   } // gateway::manager
} // casual

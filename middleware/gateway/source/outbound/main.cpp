//!
//! Copyright (c) 2020, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!

#include "gateway/outbound/state.h"
#include "gateway/outbound/handle.h"
#include "gateway/message.h"

#include "common/exception/guard.h"
#include "common/signal/timer.h"
#include "common/signal.h"
#include "common/argument.h"

#include "common/communication/instance.h"

namespace casual
{
   namespace gateway::outbound
   {
      using namespace common;
      
      namespace local
      {
         namespace
         {
            namespace ipc
            {
               auto& inbound() { return communication::ipc::inbound::device();}

               auto& gateway() { return communication::instance::outbound::gateway::manager::device();}
            } // ipc

            struct Arguments
            {
               // might have som arguments in the future
            };


            // local state to keep additional stuff for the connections...
            struct State : outbound::State
            {
               struct Connection
               {
                  Connection( configuration::model::gateway::outbound::Connection configuration)
                     : configuration{ std::move( configuration)} {}

                  configuration::model::gateway::outbound::Connection configuration;

                  struct
                  {
                     platform::size::type attempts{};
                     
                     CASUAL_LOG_SERIALIZE( CASUAL_SERIALIZE( attempts);)
                  } metric;

                  CASUAL_LOG_SERIALIZE( 
                     CASUAL_SERIALIZE( configuration);
                     CASUAL_SERIALIZE( metric);
                  )
               };

               struct
               {
                  std::vector< Connection> connections;

                  CASUAL_LOG_SERIALIZE( 
                     CASUAL_SERIALIZE( connections);
                  )
               } pending;

               CASUAL_LOG_SERIALIZE(
                  outbound::State::serialize( archive);
                  CASUAL_SERIALIZE( pending);
               )
            };

            State initialize( Arguments arguments)
            {
               Trace trace{ "casual::gateway::outbound::local::initialize"};

               // 'connect' to gateway-manager - will send configuration-update-request as soon as possible
               // that we'll handle in the main message pump
               communication::device::blocking::send( ipc::gateway(), gateway::message::outbound::Connect{ process::handle()});

               // 'connect' to our local domain
               common::communication::instance::connect();

               return {};
            }

            namespace external
            {
               void connect( State& state)
               {
                  Trace trace{ "casual::gateway::outbound::local::external::connect"};

                  auto connected = [&state]( auto& connection)
                  {
                     ++connection.metric.attempts;
                     if( auto socket = communication::tcp::non::blocking::connect( connection.configuration.address))
                     {
                        // start the connection phase against the other inbound
                        outbound::handle::connect( state, std::move( socket), std::move( connection.configuration));
                        return true;
                     }
                     return false;
                  };

                  auto& pending = state.pending.connections;
                  algorithm::trim( pending, algorithm::remove_if( pending, connected));

                  // check if we need to set a timeout to keep trying to connect

                  auto min_attempts = []( auto& l, auto& r){ return l.metric.attempts < r.metric.attempts;};

                  if( auto min = algorithm::min( pending, min_attempts))
                  {
                     if( min->metric.attempts < 200)
                        common::signal::timer::set( std::chrono::milliseconds{ 10});
                     else
                        common::signal::timer::set( std::chrono::seconds{ 3});
                  }

                  log::line( verbose::log, "state: ", state);
               }

               void reconnect( State& state, configuration::model::gateway::outbound::Connection configuration)
               {
                  Trace trace{ "casual::gateway::outbound::local::external::reconnect"};

                  if( state.runlevel == decltype( state.runlevel())::running)
                  {
                     log::line( log::category::information, code::casual::communication_unavailable, " lost connection ", configuration.address, " - action: try to reconnect");
                     state.pending.connections.emplace_back( std::move( configuration));
                     external::connect( state);
                  }
               }


               namespace dispatch
               {
                  auto create( State& state)
                  {
                     return [&state, handler = outbound::handle::external( state)]( strong::file::descriptor::id descriptor) mutable
                     {
                        auto is_connection = [descriptor]( auto& connection)
                        {
                           return connection.device.connector().descriptor() == descriptor;
                        };

                        if( auto found = algorithm::find_if( state.external.connections, is_connection))
                        {
                           try
                           {
                              state.external.last = descriptor;
                              handler( communication::device::blocking::next( found->device));
                           }
                           catch( ...)
                           {
                              if( exception::code() != code::casual::communication_unavailable)
                                 throw;

                              // Do we try to reconnect?
                              if( auto configuration = handle::connection::lost( state, descriptor))
                                 external::reconnect( state, std::move( configuration.value()));

                           }
                           return true;
                        }
                        return false;
                     };
                  }
               } // dispatch
            } // external

            namespace internal
            {
               // handles that are specific to the outbound
               namespace handle
               {
                  namespace configuration::update
                  {
                     auto request( State& state)
                     {
                        return [&state]( gateway::message::outbound::configuration::update::Request& message)
                        {
                           Trace trace{ "gateway::outbound::local::internal::handle::configuration::update::request"};
                           log::line( verbose::log, "message: ", message);

                           // TODO maintainece - make sure we can handle runtime updates...

                           state.alias = message.model.alias;
                           state.order = message.order;

                           state.pending.connections = algorithm::transform( message.model.connections, []( auto& configuration)
                           {
                              return local::State::Connection{ std::move( configuration)};
                           });


                           // we might got some addresses to try...
                           external::connect( state);
                        };
                     }
                  } // configuration::update

                  namespace state
                  {
                     auto request( State& state)
                     {
                        return [&state]( message::outbound::state::Request& message)
                        {
                           Trace trace{ "gateway::outbound::local::handle::internal::state::request"};
                           log::line( verbose::log, "message: ", message);
                           log::line( verbose::log, "state: ", state);

                           auto reply = state.reply( message);

                           // add pending connections
                           algorithm::transform( state.pending.connections, std::back_inserter( reply.state.connections), []( auto& pending)
                           {
                              message::outbound::state::Connection result;
                              result.configuration = pending.configuration;
                              return result;
                           });

                           log::line( verbose::log, "reply: ", reply);

                           communication::device::blocking::optional::send( message.process.ipc, reply);
                        };
                     }

                  } // state

                  namespace shutdown
                  {
                     auto request( State& state)
                     {
                        return [&state]( const common::message::shutdown::Request& message)
                        {
                           Trace trace{ "gateway::outbound::local::internal::handle::shutdown::request"};
                           log::line( verbose::log, "message: ", message);

                           // remove pending connections
                           state.pending.connections.clear();

                           outbound::handle::shutdown( state);
                        };
                     }
                  } // shutdown


               } // handle

               auto handler( State& state)
               {
                  return outbound::handle::internal( state) + common::message::dispatch::handler( ipc::inbound(),
                     handle::configuration::update::request( state),
                     handle::state::request( state),
                     handle::shutdown::request( state));
               }

               namespace dispatch
               {
                  auto create( State& state) 
                  { 
                     state.directive.read.add( ipc::inbound().connector().descriptor());
                     return [handler = internal::handler( state) ]() mutable
                     {
                        return predicate::boolean( handler( communication::device::non::blocking::next( ipc::inbound())));
                     };
                  }
               } // dispatch

            } // internal

            namespace signal::callback
            {
               auto timeout( State& state)
               {
                  return [&state]()
                  {
                     Trace trace{ "casual::gateway::outbound::local::signal::callback::timeout"};

                     external::connect( state);                     
                  };
               }
            } // signal::callback

            auto condition( State& state)
            {
               return communication::select::dispatch::condition::compose(
                  communication::select::dispatch::condition::done( [&state](){ return state.done();}),
                  communication::select::dispatch::condition::idle( [&state]()
                  {
                     if( !  state.disconnecting.empty())
                     {
                        // we might get some connection lost, and need to reconnect. 
                        for( auto& configuration : outbound::handle::idle( state))
                           external::reconnect( state, std::move( configuration));
                     }
                  })
               );
            }

            void run( State state)
            {
               Trace trace{ "casual::gateway::outbound::local::run"};
               log::line( verbose::log, "state: ", state);

               // register the alarm callback.
               common::signal::callback::registration< code::signal::alarm>( signal::callback::timeout( state));

               auto abort_guard = execute::scope( [&state]()
               {
                  gateway::outbound::handle::abort( state);
               });

               // start the message dispatch
               communication::select::dispatch::pump( 
                  local::condition( state),
                  state.directive, 
                  internal::dispatch::create( state),
                  external::dispatch::create( state)
               );

               abort_guard.release();

            }

            void main( int argc, char** argv)
            {
               Arguments arguments;
               
               argument::Parse{ "outbound",
               }( argc, argv);

               run( initialize( std::move( arguments)));
            } 

         } // <unnamed>
      } // local

   } // gateway::outbound

} // casual


int main( int argc, char** argv)
{
   return casual::common::exception::main::guard( [=]()
   {
      casual::gateway::outbound::local::main( argc, argv);
   });
} // main
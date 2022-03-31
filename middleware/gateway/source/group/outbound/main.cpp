//!
//! Copyright (c) 2020, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!

#include "gateway/group/outbound/state.h"
#include "gateway/group/outbound/handle.h"
#include "gateway/group/handle.h"
#include "gateway/group/tcp.h"
#include "gateway/group/tcp/logical/connect.h"
#include "gateway/group/tcp/connect.h"
#include "gateway/group/ipc.h"
#include "gateway/message.h"

#include "common/exception/guard.h"
#include "common/signal/timer.h"
#include "common/signal.h"
#include "common/argument.h"
#include "common/message/signal.h"
#include "common/message/internal.h"

#include "common/communication/instance.h"

namespace casual
{
   namespace gateway::group::outbound
   {
      using namespace common;
      
      namespace local
      {
         namespace
         {
            struct Arguments
            {
               // might have som arguments in the future
            };

            // local state to keep additional stuff for the connections...
            struct State : outbound::State
            {
               tcp::connect::state::Connect< configuration::model::gateway::outbound::Connection> connect;

               CASUAL_LOG_SERIALIZE(
                  outbound::State::serialize( archive);
                  CASUAL_SERIALIZE( connect);
               )
            };

            State initialize( Arguments arguments)
            {
               Trace trace{ "gateway::group::outbound::local::initialize"};

               // 'connect' to gateway-manager - will send configuration-update-request as soon as possible
               // that we'll handle in the main message pump
               communication::device::blocking::send( ipc::manager::gateway(), gateway::message::outbound::Connect{ process::handle()});

               // 'connect' to our local domain
               common::communication::instance::whitelist::connect();

               return {};
            }

            namespace external
            {
               void connect( State& state)
               {
                  Trace trace{ "gateway::group::outbound::local::external::connect"};

                  group::tcp::connect::attempt< group::tcp::logical::connect::Bound::out>( state);
                  log::line( verbose::log, "state: ", state);
               }

               void reconnect( State& state, configuration::model::gateway::outbound::Connection configuration)
               {
                  Trace trace{ "gateway::group::outbound::local::external::reconnect"};

                  if( state.runlevel == decltype( state.runlevel())::running)
                  {
                     log::line( log::category::information, code::casual::communication_unavailable, " lost connection ", configuration.address, " - action: try to reconnect");
                     state.connect.prospects.emplace_back( std::move( configuration));
                     external::connect( state);
                  }
               }

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
                           Trace trace{ "gateway::group::outbound::local::internal::handle::configuration::update::request"};
                           log::line( verbose::log, "message: ", message);

                           // TODO maintainece - make sure we can handle runtime updates...

                           state.alias = message.model.alias;
                           state.order = message.order;

                           for( auto& configuration : message.model.connections)
                              state.connect.prospects.emplace_back( std::move( configuration));


                           // we might got some addresses to try...
                           external::connect( state);
                           
                           // send reply
                           communication::device::blocking::optional::send(
                              message.process.ipc, common::message::reverse::type( message, common::process::handle()));
                           
                        };
                     }
                  } // configuration::update

                  namespace state
                  {
                     auto request( State& state)
                     {
                        return [&state]( message::outbound::state::Request& message)
                        {
                           Trace trace{ "gateway::group::outbound::local::handle::internal::state::request"};

                           communication::device::blocking::optional::send( message.process.ipc, tcp::connect::state::request( state, message));
                        };
                     }

                  } // state

                  namespace event
                  {
                     namespace process
                     {
                        auto exit( State& state)
                        {
                           return [&state]( common::message::event::process::Exit& message)
                           {
                              Trace trace{ "gateway::group::outbound::local::internal::handle::event::process::exit"};
                              common::log::line( verbose::log, "message: ", message);

                              // the process might be from our spawned connector
                              if( auto configuration = state.external.pending().exit( message.state))
                                 external::reconnect( state, std::move( configuration.value()));
                              
                           };
                        }
                     } // process
                  } // event

                  namespace shutdown
                  {
                     auto request( State& state)
                     {
                        return [&state]( const common::message::shutdown::Request& message)
                        {
                           Trace trace{ "gateway::group::outbound::local::internal::handle::shutdown::request"};
                           log::line( verbose::log, "message: ", message);

                           // remove pending connections
                           state.connect.prospects.clear();

                           outbound::handle::shutdown( state);
                        };
                     }
                  } // shutdown

                  auto timeout( State& state)
                  {
                     return [&state]( const common::message::signal::Timeout& message)
                     {
                        Trace trace{ "gateway::group::outbound::local::internal::handle::timeout"};

                        external::connect( state);
                     };
                  }

                  namespace connection
                  {
                     auto lost( State& state)
                     {
                        return [&state]( message::outbound::connection::Lost message)
                        {
                           Trace trace{ "gateway::group::outbound::local::internal::handle::connection::lost"};
                           log::line( verbose::log, "message: ", message);

                           external::reconnect( state, std::move( message.configuration));
                        };
                     }
                  } // connection


               } // handle

               auto handler( State& state)
               {
                  return outbound::handle::internal( state) + common::message::dispatch::handler( ipc::inbound(),
                     common::message::internal::dump::state::handle( state),
                     handle::configuration::update::request( state),
                     handle::state::request( state),
                     handle::event::process::exit( state),
                     handle::shutdown::request( state),
                     handle::timeout( state),
                     handle::connection::lost( state));
               }

            } // internal

            namespace signal::callback
            {
               auto timeout()
               {
                  return []()
                  {
                     Trace trace{ "gateway::group::outbound::local::signal::callback::timeout"};

                     // we push it to our own inbound ipc 'queue', and handle the timeout
                     // in our regular message pump.
                     ipc::inbound().push( common::message::signal::Timeout{});                 
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
               Trace trace{ "gateway::group::outbound::local::run"};
               log::line( verbose::log, "state: ", state);

               auto abort_guard = execute::scope( [&state]()
               {
                  gateway::group::outbound::handle::abort( state);
               });

               // register the alarm callback.
               common::signal::callback::registration< code::signal::alarm>( signal::callback::timeout());

               // make sure we listen to the death of our children
               common::signal::callback::registration< code::signal::child>( group::handle::signal::process::exit());

               // start the message dispatch
               communication::select::dispatch::pump( 
                  local::condition( state),
                  state.directive,
                  tcp::handle::dispatch::create( state, outbound::handle::external( state), &handle::connection::lost),
                  gateway::group::tcp::pending::send::dispatch( state),
                  ipc::dispatch::create( state, &internal::handler),
                  // takes care of multiplexing connects
                  tcp::connect::dispatch::create( state, tcp::logical::connect::Bound::out)
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

   } // gateway::group::outbound

} // casual


int main( int argc, char** argv)
{
   return casual::common::exception::main::log::guard( [=]()
   {
      casual::gateway::group::outbound::local::main( argc, argv);
   });
} // main
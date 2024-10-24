//!
//! Copyright (c) 2020, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!

#include "gateway/group/inbound/state.h"
#include "gateway/group/inbound/handle.h"
#include "gateway/group/handle.h"
#include "gateway/group/tcp.h"
#include "gateway/group/tcp/connect.h"
#include "gateway/group/ipc.h"
#include "gateway/message.h"

#include "common/exception/guard.h"
#include "common/signal/timer.h"
#include "common/signal.h"
#include "common/message/signal.h"
#include "common/message/dispatch/handle.h"
#include "common/algorithm.h"
#include "common/algorithm/container.h"

#include "common/communication/instance.h"
#include "common/communication/select/ipc.h"

#include "configuration/model/change.h"

namespace casual
{
   namespace gateway::group::inbound::reverse
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


            // local state to keep additional stuff for reverse connections...
            struct State : inbound::State
            {
               tcp::connect::state::Connect< configuration::model::gateway::inbound::Connection> connect;

               CASUAL_LOG_SERIALIZE(
                  inbound::State::serialize( archive);
                  CASUAL_SERIALIZE( connect);
               )
            };

            State initialize( Arguments arguments)
            {
               Trace trace{ "gateway::group::inbound::reverse::local::initialize"};

               State state;

               // 'connect' to gateway-manager - will send configuration-update-request as soon as possible
               // that we'll handle in the main message pump

               state.multiplex.send( ipc::manager::gateway(), gateway::message::inbound::Connect{ process::handle()});

               // 'connect' to our local domain
               common::communication::instance::whitelist::connect();

               return state;
            }

            namespace external
            {
               void connect( State& state)
               {
                  Trace trace{ "gateway::group::inbound::reverse::local::external::connect"};
                  
                  tcp::connect::attempt< tcp::logical::connect::Bound::in>( state);
                  log::line( verbose::log, "state: ", state);
               }

               void reconnect( State& state, configuration::model::gateway::inbound::Connection configuration)
               {
                  Trace trace{ "gateway::inbound::local::external::reconnect"};

                  if( state.runlevel == decltype( state.runlevel())::running)
                  {
                     log::line( log::category::information, "try to reconnect: '", configuration.address, "'");
                     state.connect.prospects.emplace_back( std::move( configuration));
                     external::connect( state);
                  }
               }

            } // external

            namespace management
            {
               // handles that are specific to the reverse-inbound
               namespace handle
               {
                  namespace configuration::update
                  {
                     auto request( State& state)
                     {
                        return [&state]( gateway::message::inbound::configuration::update::Request& message)
                        {
                           Trace trace{ "gateway::reverse::inbound::local::internal::handle::configuration::update::request"};
                           log::line( verbose::log, "message: ", message);

                           state.alias = message.model.alias;
                           state.limit = message.model.limit;

                           auto is_enabled = []( auto& connection){ return connection.enabled;};
                           auto [ enabled, disabled] = algorithm::stable::partition( message.model.connections, is_enabled);

                           state.disabled_connections = algorithm::container::vector::create( disabled);

                           auto equal_address = []( auto& lhs, auto& rhs){ return lhs.address == rhs.address;};

                           auto change = casual::configuration::model::change::concrete::calculate( 
                              state.connections.configuration(), 
                              algorithm::container::vector::create( enabled), 
                              equal_address);

                           log::line( verbose::log, "change: ", change);
                           
                           // add
                           {
                              for( auto& configuration : change.added)
                                 state.connect.prospects.emplace_back( std::move( configuration));
                           }

                           // modify
                           {
                              for( auto& configuration : change.modified)
                                 state.connections.replace_configuration( std::move( configuration));
                           }

                           // remove
                           {
                              for( auto& configuration : change.removed)
                                 if( auto descriptor = state.connections.external_descriptor( configuration.address))
                                    inbound::handle::connection::disconnect( state, descriptor);
                           }

                           // we might got some addresses to try...
                           external::connect( state);

                           // send reply
                           state.multiplex.send( message.process.ipc, common::message::reverse::type( message, common::process::handle()));
                        };
                     }
                  } // configuration::update

                  namespace state
                  {
                     auto request( State& state)
                     {
                        return [&state]( message::inbound::reverse::state::Request& message)
                        {
                           Trace trace{ "gateway::group::inbound::reverse::local::handle::internal::state::request"};

                           state.multiplex.send( message.process.ipc, tcp::connect::state::request( state, message));
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
                              Trace trace{ "gateway::group::inbound::reverse::local::handle::internal::event::process::exit"};
                              common::log::line( verbose::log, "message: ", message);

                              // the process might be from our spawned connector
                              if( auto configuration = state.connections.pending().exit( message.state))
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
                           Trace trace{ "gateway::group::inbound::reverse::local::handle::internal::shutdown::request"};
                           log::line( verbose::log, "message: ", message);

                           state.runlevel = decltype( state.runlevel())::shutdown;
                           inbound::handle::shutdown( state);
                        };
                     }
                  } // shutdown

                  auto timeout( State& state)
                  {
                     return [&state]( const common::message::signal::Timeout& message)
                     {
                        Trace trace{ "gateway::group::inbound::reverse::local::internal::handle::timeout"};

                        external::connect( state);
                     };
                  }

                  namespace connection
                  {
                     auto lost( State& state)
                     {
                        return [&state]( message::inbound::connection::Lost message)
                        {
                           Trace trace{ "gateway::group::inbound::reverse::local::internal::handle::connection::lost"};
                           log::line( verbose::log, "message: ", message);

                           if( state.runlevel > inbound::state::Runlevel::running)
                              return;

                           log::line( log::category::information, code::casual::communication_unavailable, " lost connection to domain: ", message.remote);
                           external::reconnect( state, std::move( message.configuration));
                        };
                     }
                  } // connection

               } // handle

               auto handler( State& state)
               {
                  // we add the common/general management handlers
                  return inbound::handle::management( state) + inbound::handle::management_handler{
                     common::message::dispatch::handle::defaults( state),
                     handle::configuration::update::request( state),
                     handle::state::request( state),
                     handle::event::process::exit( state),
                     handle::shutdown::request( state),
                     handle::timeout( state),
                     handle::connection::lost( state)
                  };
               }

            } // management

            namespace signal::callback
            {
               auto timeout()
               {
                  return []()
                  {
                     Trace trace{ "gateway::group::inbound::reverse::local::signal::callback::timeout"};

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
                  communication::select::dispatch::condition::idle( [&state](){ inbound::handle::idle( state);})
                  );
            }

            void run( State state)
            {
               Trace trace{ "gateway::group::inbound::reverse::local::run"};
               log::line( verbose::log, "state: ", state);

               auto abort_guard = execute::scope( [&state]()
               {
                  gateway::group::inbound::handle::abort( state);
               });

               // register the alarm callback.
               common::signal::callback::registration< code::signal::alarm>( signal::callback::timeout());

               // make sure we listen to the death of our children
               common::signal::callback::registration< code::signal::child>( group::handle::signal::process::exit());

               // start the message dispatch
               communication::select::dispatch::pump( 
                  local::condition( state),
                  state.directive,
                  tcp::pending::send::dispatch::create( state, &handle::connection::lost),
                  ipc::handle::dispatch::create< inbound::Policy>( state, inbound::handle::internal( state)),
                  tcp::handle::dispatch::create< inbound::Policy>( state, inbound::handle::external( state), &handle::connection::lost),
                  // takes care of multiplexing connects
                  tcp::connect::dispatch::create( state, tcp::logical::connect::Bound::in),
                  communication::select::ipc::dispatch::create< inbound::Policy>( state, &management::handler),
                  state.multiplex
               );

               abort_guard.release();
            }

            void main( int argc, const char** argv)
            {
               Arguments arguments;

               run( initialize( std::move( arguments)));
            } 

         } // <unnamed>
      } // local

   } // gateway::group::inbound::reverse

} // casual


int main( int argc, const char** argv)
{
   return casual::common::exception::main::log::guard( [=]()
   {
      casual::gateway::group::inbound::reverse::local::main( argc, argv);
   });
} // main
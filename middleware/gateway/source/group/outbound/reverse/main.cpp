//!
//! Copyright (c) 2020, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!

#include "gateway/group/outbound/handle.h"
#include "gateway/group//handle.h"
#include "gateway/group/outbound/state.h"
#include "gateway/group/tcp/listen.h"
#include "gateway/group/ipc.h"
#include "gateway/message.h"

#include "common/exception/guard.h"
#include "common/log/stream.h"
#include "common/communication/instance.h"
#include "common/communication/select/ipc.h"
#include "casual/argument.h"
#include "common/message/dispatch/handle.h"
#include "common/algorithm.h"
#include "common/algorithm/container.h"

#include "configuration/model/change.h"


namespace casual
{
   namespace gateway::group::outbound::reverse
   {
      using namespace common;

      namespace local
      {
         namespace
         {

            struct Arguments
            {
               CASUAL_LOG_SERIALIZE()
            };


            // local state to keep additional stuff for reverse connections...
            struct State : outbound::State
            {
               tcp::listen::state::Listen< configuration::model::gateway::outbound::Connection> listen;
   
               CASUAL_LOG_SERIALIZE(
                  outbound::State::serialize( archive);
                  CASUAL_SERIALIZE( listen);
               )
            };

 

            State initialize( Arguments arguments)
            {
               Trace trace{ "gateway::group::outbound::reverse::local::initialize"};
               log::line( verbose::log, "arguments: ", arguments);

               State state;

               // 'connect' to gateway-manager - will send configuration-update-request as soon as possible
               // that we'll handle in the main message pump
               state.multiplex.send( ipc::manager::gateway(), gateway::message::outbound::Connect{ process::handle()});

               // 'connect' to our local domain
               common::communication::instance::whitelist::connect();

               return state;
            }

            namespace management
            {
               // handles that are specific to the reverse-outbound
               namespace handle
               {
                  namespace configuration::update
                  {
                     auto request( State& state)
                     {
                        return [ &state]( gateway::message::outbound::configuration::update::Request&& message)
                        {
                           Trace trace{ "gateway::group::outbound::reverse::local::internal::handle::configuration::update::request"};
                           log::line( verbose::log, "message: ", message);

                           state.alias = message.model.alias;
                           
                           // if the order differ we need to re-advertise all current connections with
                           // the new order.
                           if( state.order != message.model.order)
                           {
                              state.order = message.model.order;
                              outbound::handle::advertise::connections( state);
                           }

                           auto is_enabled = []( auto& connection){ return connection.enabled;};
                           auto [ enabled, disabled] = algorithm::stable::partition( message.model.connections, is_enabled);

                           state.disabled_connections = algorithm::container::vector::create( disabled);

                           auto equal_address = []( auto& lhs, auto& rhs){ return lhs.address == rhs.address;};

                           auto change = casual::configuration::model::change::concrete::calculate( 
                              state.listen.configuration(), 
                              algorithm::container::vector::create( enabled), 
                              equal_address);

                           log::line( verbose::log, "change: ", change);
                           
                           // add
                           {
                              tcp::listen::attempt( state, std::move( change.added));
                           }

                           // modify
                           {
                              for( auto& configuration : change.modified)
                                 state.listen.replace_configuration( std::move( configuration));
                           }

                           // remove
                           {
                              auto remove = [ &state]( auto& configuration)
                              {
                                 if( auto listener = state.listen.extract( state.directive, configuration.address))
                                 {
                                    log::information( "stopped listening on: ", listener->configuration.address);
                                    
                                    for( auto descriptor : state.connections.host_connections( listener->socket.descriptor()))
                                       outbound::handle::connection::remove( state, descriptor);
                                 }
                              };

                              algorithm::for_each( change.removed, remove);
                           }

                           // send reply
                           state.multiplex.send( message.process.ipc, common::message::reverse::type( message, common::process::handle()));
                        };
                     }
                  } // configuration::update

                  namespace state
                  {
                     auto request( State& state)
                     {
                        return [&state]( message::outbound::reverse::state::Request& message)
                        {
                           Trace trace{ "gateway::group::outbound::reverse::local::handle::internal::state::request"};

                           state.multiplex.send( message.process.ipc, tcp::listen::state::request( state, message));
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
                              state.connections.pending().exit( message.state);
                              
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
                           Trace trace{ "gateway::group::outbound::reverse::local::internal::handle::shutdown::request"};
                           log::line( verbose::log, "message: ", message);

                           state.listen.clear( state.directive);

                           outbound::handle::shutdown( state);
                        };
                     }
                  } // shutdown

                  namespace connection
                  {
                     auto lost()
                     {
                        return []( message::outbound::connection::Lost message)
                        {
                           Trace trace{ "gateway::group::outbound::reverse::local::internal::handle::connection::lost"};
                           log::line( verbose::log, "message: ", message);

                           // we just log the 'event'
                           log::line( log::category::information, code::casual::communication_unavailable, " lost connection to domain: ", message.remote);
                        };
                     }
                  } // connection

               } // handle

               auto handler( State& state)
               {
                  return outbound::handle::management( state) + outbound::handle::management_handler{ 
                     common::message::dispatch::handle::defaults( state),
                     handle::configuration::update::request( state),
                     handle::state::request( state),
                     handle::event::process::exit( state),
                     handle::shutdown::request( state),
                     handle::connection::lost()
                  };
               }

            } // management

            auto condition( State& state)
            {
               return communication::select::dispatch::condition::compose(
                  communication::select::dispatch::condition::done( [&state](){ return state.done();}),
                  communication::select::dispatch::condition::idle( [&state](){ outbound::handle::idle( state);})
               );
            }

            void run( State state)
            {
               Trace trace{ "gateway::group::outbound::reverse::local::run"};
               log::line( verbose::log, "state: ", state);

               auto abort_guard = execute::scope( [&state]()
               {
                  gateway::group::outbound::handle::abort( state);
               });

               // make sure we listen to the death of our children
               common::signal::callback::registration< code::signal::child>( group::handle::signal::process::exit());

               // start the message dispatch
               communication::select::dispatch::pump( 
                  local::condition( state),
                  state.directive,
                  tcp::handle::dispatch::create< outbound::Policy>( state, outbound::handle::external( state), &handle::connection::lost),
                  tcp::pending::send::dispatch::create( state, &handle::connection::lost),
                  ipc::handle::dispatch::create< outbound::Policy>( state, outbound::handle::internal( state)),
                  communication::select::ipc::dispatch::create< outbound::Policy>( state, &management::handler),
                  tcp::listen::dispatch::create( state, tcp::logical::connect::Bound::out),
                  state.multiplex
               );
               
               abort_guard.release();
            }

            void main( int argc, const char** argv)
            {
               Trace trace{ "gateway::group::outbound::reverse::local::main"};

               Arguments arguments;

               argument::parse( "reverse outbound", {}, argc, argv);

               run( initialize( std::move( arguments)));
            } 

         } // <unnamed>
      } // local

   } // gateway::group::outbound::reverse

} // casual


int main( int argc, const char** argv)
{
   return casual::common::exception::main::log::guard( [=]()
   {
      casual::gateway::group::outbound::reverse::local::main( argc, argv);
   });
} // main
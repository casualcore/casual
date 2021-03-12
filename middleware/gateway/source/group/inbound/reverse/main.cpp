//!
//! Copyright (c) 2020, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!

#include "gateway/group/inbound/state.h"
#include "gateway/group/inbound/handle.h"
#include "gateway/group/connector.h"
#include "gateway/message.h"

#include "common/exception/guard.h"
#include "common/signal/timer.h"
#include "common/signal.h"
#include "common/message/signal.h"

#include "common/communication/instance.h"

namespace casual
{
   namespace gateway::group::inbound::reverse
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


            // local state to keep additional stuff for reverse connections...
            struct State : inbound::State
            {
               struct Connection
               {
                  Connection( configuration::model::gateway::inbound::Connection configuration)
                     : configuration{ std::move( configuration)} {}

                  configuration::model::gateway::inbound::Connection configuration;

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
               } reverse;

               CASUAL_LOG_SERIALIZE(
                  inbound::State::serialize( archive);
                  CASUAL_SERIALIZE( reverse);
               )
            };

            State initialize( Arguments arguments)
            {
               Trace trace{ "gateway::group::inbound::reverse::local::initialize"};

               // 'connect' to gateway-manager - will send configuration-update-request as soon as possible
               // that we'll handle in the main message pump
               communication::device::blocking::send( ipc::gateway(), gateway::message::inbound::Connect{ process::handle()});

               // 'connect' to our local domain
               common::communication::instance::whitelist::connect();

               return {};
            }

            namespace external
            {
               void connect( State& state)
               {
                  Trace trace{ "gateway::group::inbound::reverse::local::external::connect"};

                  group::connector::external::connect( state.reverse.connections, [&state]( auto&& socket, auto&& connection)
                  {
                     state.external.add( state.directive, std::move( socket), std::move( connection.configuration));
                  });

                  log::line( verbose::log, "state: ", state);
               }

               void reconnect( State& state, configuration::model::gateway::inbound::Connection configuration)
               {
                  Trace trace{ "gateway::inbound::local::external::reconnect"};

                  if( state.runlevel == decltype( state.runlevel())::running)
                  {
                     log::line( log::category::information, code::casual::communication_unavailable, " lost connection ", configuration.address, " - action: try to reconnect");
                     state.reverse.connections.emplace_back( std::move( configuration));
                     external::connect( state);
                  }
               }

               namespace dispatch
               {
                  auto create( State& state) 
                  { 
                     return [&state, handler = inbound::handle::external( state)]( common::strong::file::descriptor::id descriptor) mutable
                     {
                        if( auto found = algorithm::find( state.external.connections, descriptor))
                        {
                           try
                           {
                              if( auto correlation = handler( communication::device::blocking::next( found->device)))
                                 state.correlations.emplace_back( std::move( correlation), descriptor);
                              else
                                 log::line( log::category::error, code::casual::invalid_semantics, " failed to handle next message for device: ", found->device);
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

                           // TODO maintainece - make sure we can handle runtime updates...
                           state.alias = message.model.alias;
                           state.pending.requests.limit( message.model.limit);
                           
                           state.reverse.connections = algorithm::transform( message.model.connections, []( auto& configuration)
                           {
                              return local::State::Connection{ std::move( configuration)};
                           });

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
                        return [&state]( message::inbound::reverse::state::Request& message)
                        {
                           Trace trace{ "gateway::group::inbound::reverse::local::handle::internal::state::request"};
                           log::line( verbose::log, "message: ", message);
                           log::line( verbose::log, "state: ", state);
                           
                           auto reply = state.reply( message);

                           // add reverse connections
                           algorithm::transform( state.reverse.connections, reply.state.connections, []( auto& pending)
                           {
                              message::inbound::state::Connection result;
                              result.configuration = pending.configuration;
                              return result;
                           });
                           
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

               } // handle

               auto handler( State& state)
               {
                  // we add the common/general inbound handlers
                  return inbound::handle::internal( state) + common::message::dispatch::handler( ipc::inbound(),
                     handle::configuration::update::request( state),
                     handle::state::request( state),
                     handle::shutdown::request( state),
                     handle::timeout( state)
                  );
               }

               namespace dispatch
               {
                  auto create( State& state) 
                  { 
                     state.directive.read.add( ipc::inbound().connector().descriptor());
                     return [handler = internal::handler( state)]() mutable
                     {
                        return common::predicate::boolean( handler( common::communication::device::non::blocking::next( ipc::inbound())));

                     };
                  }
               } // dispatch

            } // internal

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

               run( initialize( std::move( arguments)));
            } 

         } // <unnamed>
      } // local

   } // gateway::group::inbound::reverse

} // casual


int main( int argc, char** argv)
{
   return casual::common::exception::main::guard( [=]()
   {
      casual::gateway::group::inbound::reverse::local::main( argc, argv);
   });
} // main
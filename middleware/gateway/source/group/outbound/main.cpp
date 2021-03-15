//!
//! Copyright (c) 2020, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!

#include "gateway/group/outbound/state.h"
#include "gateway/group/outbound/handle.h"
#include "gateway/group/connector.h"
#include "gateway/message.h"

#include "common/exception/guard.h"
#include "common/signal/timer.h"
#include "common/signal.h"
#include "common/argument.h"
#include "common/message/signal.h"

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
               Trace trace{ "gateway::group::outbound::local::initialize"};

               // 'connect' to gateway-manager - will send configuration-update-request as soon as possible
               // that we'll handle in the main message pump
               communication::device::blocking::send( ipc::gateway(), gateway::message::outbound::Connect{ process::handle()});

               // 'connect' to our local domain
               common::communication::instance::whitelist::connect();

               return {};
            }

            namespace external
            {
               void connect( State& state)
               {
                  Trace trace{ "gateway::group::outbound::local::external::connect"};

                  group::connector::external::connect( state.pending.connections, [&state]( auto&& socket, auto&& connection)
                  {
                     // start the connection phase against the other inbound
                     outbound::handle::connect( state, std::move( socket), std::move( connection.configuration));
                  });

                  log::line( verbose::log, "state: ", state);
               }

               void reconnect( State& state, configuration::model::gateway::outbound::Connection configuration)
               {
                  Trace trace{ "gateway::group::outbound::local::external::reconnect"};

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
                     return [&state, handler = outbound::handle::external( state)]
                        ( strong::file::descriptor::id descriptor, communication::select::tag::read) mutable
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
                           Trace trace{ "gateway::group::outbound::local::internal::handle::configuration::update::request"};
                           log::line( verbose::log, "message: ", message);

                           // TODO maintainece - make sure we can handle runtime updates...

                           state.alias = message.model.alias;
                           state.order = message.model.order;

                           state.pending.connections = algorithm::transform( message.model.connections, []( auto& configuration)
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
                        return [&state]( message::outbound::state::Request& message)
                        {
                           Trace trace{ "gateway::group::outbound::local::handle::internal::state::request"};
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
                           Trace trace{ "gateway::group::outbound::local::internal::handle::shutdown::request"};
                           log::line( verbose::log, "message: ", message);

                           // remove pending connections
                           state.pending.connections.clear();

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


               } // handle

               auto handler( State& state)
               {
                  return outbound::handle::internal( state) + common::message::dispatch::handler( ipc::inbound(),
                     handle::configuration::update::request( state),
                     handle::state::request( state),
                     handle::shutdown::request( state),
                     handle::timeout( state));
               }

               namespace dispatch
               {
                  auto create( State& state) 
                  { 
                     state.directive.read.add( ipc::inbound().connector().descriptor());
                     return [handler = internal::handler( state) ]() mutable -> strong::file::descriptor::id
                     {
                        if( handler( communication::device::non::blocking::next( ipc::inbound())))
                           return ipc::inbound().connector().descriptor();

                        return {};
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

               // register the alarm callback.
               common::signal::callback::registration< code::signal::alarm>( signal::callback::timeout());

               auto abort_guard = execute::scope( [&state]()
               {
                  gateway::group::outbound::handle::abort( state);
               });

               // start the message dispatch
               communication::select::dispatch::pump( 
                  local::condition( state),
                  state.directive,
                  external::dispatch::create( state),
                  internal::dispatch::create( state)
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
   return casual::common::exception::main::guard( [=]()
   {
      casual::gateway::group::outbound::local::main( argc, argv);
   });
} // main
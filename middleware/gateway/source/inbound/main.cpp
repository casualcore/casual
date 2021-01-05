//!
//! Copyright (c) 2020, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!

#include "gateway/inbound/handle.h"
#include "gateway/inbound/state.h"
#include "gateway/message.h"

#include "common/exception/guard.h"
#include "common/log/stream.h"
#include "common/communication/instance.h"
#include "common/argument.h"


namespace casual
{
   namespace gateway::inbound
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
               CASUAL_LOG_SERIALIZE()
            };


            // local state to keep additional stuff for reverse connections...
            struct State : inbound::State
            {
               struct Listener
               {
                  communication::tcp::Socket socket;
                  configuration::model::gateway::inbound::Connection configuration;

                  inline friend bool operator == ( const Listener& lhs, common::strong::file::descriptor::id rhs) { return lhs.socket.descriptor() == rhs;}

                  CASUAL_LOG_SERIALIZE( 
                     CASUAL_SERIALIZE( socket);
                     CASUAL_SERIALIZE( configuration);
                  )
               };

               std::vector< Listener> listeners;
               std::vector< configuration::model::gateway::inbound::Connection> offline;
               
               CASUAL_LOG_SERIALIZE(
                  inbound::State::serialize( archive);
                  CASUAL_SERIALIZE( listeners);
                  CASUAL_SERIALIZE( offline);
               )
            };

 

            State initialize( Arguments arguments)
            {
               Trace trace{ "gateway::inbound::local::initialize"};
               log::line( verbose::log, "arguments: ", arguments);

               // 'connect' to gateway-manager - will send configuration-update-request as soon as possible
               // that we'll handle in the main message pump
               communication::device::blocking::send( ipc::gateway(), gateway::message::inbound::Connect{ process::handle()});

               // 'connect' to our local domain
               common::communication::instance::connect();

               return {};
            }

            namespace internal
            {
               // handles that are specific to the inbound
               namespace handle
               {
                  namespace configuration::update
                  {
                     auto request( State& state)
                     {
                        return [&state]( gateway::message::inbound::configuration::update::Request& message)
                        {
                           Trace trace{ "gateway::inbound::local::internal::handle::configuration::update::request"};
                           log::line( verbose::log, "message: ", message);

                           // TODO maintainece - make sure we can handle runtime updates...

                           state.alias = message.model.alias;

                           state.listeners = algorithm::transform( message.model.connections, []( auto& information)
                           {
                              auto result = State::Listener{
                                  communication::tcp::socket::listen( information.address),
                                  information};

                              // we need the socket to not block in 'accept'
                              result.socket.set( communication::socket::option::File::no_block);
                              return result;
                           });

                           state.directive.read.add( 
                              algorithm::transform( state.listeners, []( auto& listener){ return listener.socket.descriptor();}));

                           log::line( verbose::log, "state: ", state);
                        };
                     }
                  } // configuration::update

                  namespace state
                  {
                     auto request( State& state)
                     {
                        return [&state]( message::inbound::state::Request& message)
                        {
                           Trace trace{ "gateway::inbound::local::handle::internal::state::request"};
                           log::line( verbose::log, "message: ", message);
                           log::line( verbose::log, "state: ", state);

                           auto reply = state.reply( message);
                           
                           reply.state.listeners = algorithm::transform( state.listeners, []( auto& listener)
                           {
                              message::state::Listener result;
                              result.address = listener.configuration.address;
                              result.descriptor = listener.socket.descriptor();

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
                           Trace trace{ "gateway::inbound::local::handle::internal::shutdown::request"};
                           log::line( verbose::log, "message: ", message);

                           state.runlevel = decltype( state.runlevel())::shutdown;

                           // remove listeners
                           state.directive.read.remove( 
                              algorithm::transform( state.listeners, []( auto& listener){ return listener.socket.descriptor();}));

                           state.offline = algorithm::transform( state.listeners, []( auto& listener){ return listener.configuration;});
                           state.listeners.clear();

                           inbound::handle::shutdown( state);
                        };
                     }
                  } // shutdown

               } // handle

               auto handler( State& state)
               {
                  // we add the common/general inbound logic
                  return inbound::handle::internal( state) + common::message::dispatch::handler( ipc::inbound(),
                     handle::configuration::update::request( state),
                     handle::state::request( state),
                     handle::shutdown::request( state)
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


            namespace external
            {  
               namespace listener
               {
                  namespace dispatch
                  {
                     auto create( State& state)
                     {
                        return [&state]( strong::file::descriptor::id descriptor)
                        {
                           Trace trace{ "gateway::inbound::local::external::listener::dispatch"};

                           if( auto found = algorithm::find( state.listeners, descriptor))
                           {
                              log::line( verbose::log, "found: ", *found);

                              if( auto socket = communication::tcp::socket::accept( found->socket))
                              {
                                 // the socket needs to be 'blocking'
                                 socket.unset( communication::socket::option::File::no_block);

                                 state.external.add( 
                                    state.directive, 
                                    std::move( socket),
                                    found->configuration);
                              }
                              else 
                                 log::line( log::category::error, code::casual::communication_protocol, " failed to accept connection: ", *found);

                              return true;
                           }
                           return false;
                        };
                     }
                  } // dispatch
               } // listener

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
                              
                              handle::connection::lost( state, descriptor);
                           }
                           return true;
                        }
                        return false;
                     };
                  }
               } // dispatch
               
            } // external

            auto condition( State& state)
            {
               return communication::select::dispatch::condition::compose(
                  communication::select::dispatch::condition::done( [&state](){ return state.done();}),
                  communication::select::dispatch::condition::idle( [&state](){ inbound::handle::idle( state);})
                  );
            }

            void run( State state)
            {
               Trace trace{ "gateway::inbound::local::run"};
               log::line( verbose::log, "state: ", state);

               auto abort_guard = execute::scope( [&state]()
               {
                  gateway::inbound::handle::abort( state);
               });

               {
                  Trace trace{ "gateway::inbound::local::run dispatch pump"};

                  // start the message dispatch
                  communication::select::dispatch::pump( 
                     local::condition( state),
                     state.directive, 
                     internal::dispatch::create( state),
                     external::dispatch::create( state),
                     external::listener::dispatch::create( state)
                  );
               }

               abort_guard.release();
            }

            void main( int argc, char** argv)
            {
               Trace trace{ "gateway::inbound::local::main"};

               Arguments arguments;

               argument::Parse{ "inbound",
               }( argc, argv);

               run( initialize( std::move( arguments)));
            } 

         } // <unnamed>
      } // local

   } // gateway::inbound

} // casual


int main( int argc, char** argv)
{
   return casual::common::exception::main::guard( [=]()
   {
      casual::gateway::inbound::local::main( argc, argv);
   });
} // main
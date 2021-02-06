//!
//! Copyright (c) 2020, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!

#include "gateway/outbound/handle.h"
#include "gateway/outbound/state.h"
#include "gateway/outbound/error/reply.h"
#include "gateway/message.h"

#include "common/exception/guard.h"
#include "common/log/stream.h"
#include "common/communication/instance.h"
#include  "common/argument.h"


namespace casual
{
   namespace gateway::outbound::reverse
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
            struct State : outbound::State
            {
               struct Listener
               {
                  communication::Socket socket;
                  configuration::model::gateway::outbound::Connection configuration;

                  inline friend bool operator == ( const Listener& lhs, common::strong::file::descriptor::id rhs) { return lhs.socket.descriptor() == rhs;}

                  CASUAL_LOG_SERIALIZE( 
                     CASUAL_SERIALIZE( socket);
                     CASUAL_SERIALIZE( configuration);
                  )
               };

               std::vector< Listener> listeners;
               
               CASUAL_LOG_SERIALIZE(
                  outbound::State::serialize( archive);
                  CASUAL_SERIALIZE( listeners);
               )
            };

 

            State initialize( Arguments arguments)
            {
               Trace trace{ "casual::gateway::outbound::reverse::local::initialize"};
               log::line( verbose::log, "arguments: ", arguments);

               // 'connect' to gateway-manager - will send configuration-update-request as soon as possible
               // that we'll handle in the main message pump
               communication::device::blocking::send( ipc::gateway(), gateway::message::outbound::Connect{ process::handle()});

               // 'connect' to our local domain
               common::communication::instance::whitelist::connect();

               return {};
            }

            namespace internal
            {
               // handles that are specific to the reverse-outbound
               namespace handle
               {
                  namespace configuration::update
                  {
                     auto request( State& state)
                     {
                        return [&state]( gateway::message::outbound::configuration::update::Request& message)
                        {
                           Trace trace{ "gateway::outbound::reverse::local::internal::handle::configuration::update::request"};
                           log::line( verbose::log, "message: ", message);

                           // TODO maintainece - make sure we can handle runtime updates...

                           state.alias = message.model.alias;
                           state.order = message.order;

                           state.listeners = algorithm::transform( message.model.connections, []( auto& information)
                           {
                              auto result = State::Listener{
                                  communication::tcp::socket::listen( information.address),
                                  information};

                              // we need the socket to not block in accept. 
                              result.socket.set( communication::socket::option::File::no_block);

                              return result;
                           });

                           state.directive.read.add( 
                              algorithm::transform( state.listeners, []( auto& listener){ return listener.socket.descriptor();}));

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
                        return [&state]( message::outbound::reverse::state::Request& message)
                        {
                           Trace trace{ "gateway::outbound::reverse::local::handle::internal::state::request"};
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
                           Trace trace{ "gateway::outbound::reverse::local::internal::handle::shutdown::request"};
                           log::line( verbose::log, "message: ", message);

                           // remove all listeners
                           {
                              state.directive.read.remove( algorithm::transform( state.listeners, []( auto& listener){ return listener.socket.descriptor();}));
                              state.listeners.clear();
                           }
                           

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
                        return predicate::boolean( handler( communication::device::non::blocking::next( ipc::inbound())));
                     };
                  }
               } // dispatch
            } // internal



            namespace external
            {
               namespace dispatch
               {
                  auto create( State& state) 
                  {
                     return [&state, handler = gateway::outbound::handle::external( state)]( strong::file::descriptor::id descriptor) mutable
                     {
                        Trace trace{ "gateway::outbound::reverse::local::external::dispatch"};

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
                              
                              outbound::handle::connection::lost( state, descriptor);
                           }
                           return true;
                        }
                        return false;
                     };
                  }
               } // dispatch

               namespace listener
               {
                  namespace dispatch
                  {
                     auto create( State& state)
                     {
                        return [&state]( strong::file::descriptor::id descriptor)
                        {
                           Trace trace{ "gateway::outbound::reverse::local::external::listener::dispatch"};

                           if( auto found = algorithm::find( state.listeners, descriptor))
                           {
                              log::line( verbose::log, "found: ", *found);

                              if( auto socket = communication::tcp::socket::accept( found->socket))
                              {
                                 // the socket needs to be 'blocking'
                                 socket.unset( communication::socket::option::File::no_block);

                                 // start the connection phase against the other inbound
                                 outbound::handle::connect( state, std::move( socket), found->configuration);
                              }
                              return true;
                           }
                           return false;
                        };
                     }
                  } // dispatch
               } // listener
               
            } // external

            auto condition( State& state)
            {
               return communication::select::dispatch::condition::compose(
                  communication::select::dispatch::condition::done( [&state](){ return state.done();}),
                  communication::select::dispatch::condition::idle( [&state](){ outbound::handle::idle( state);})
               );
            }

            void run( State state)
            {
               Trace trace{ "casual::gateway::outbound::reverse::local::run"};
               log::line( verbose::log, "state: ", state);

               auto abort_guard = execute::scope( [&state]()
               {
                  gateway::outbound::handle::abort( state);
               });

               // start the message dispatch
               communication::select::dispatch::pump( 
                  local::condition( state),
                  state.directive, 
                  internal::dispatch::create( state),
                  external::dispatch::create( state),
                  external::listener::dispatch::create( state)
               );
               
               abort_guard.release();
            }

            void main( int argc, char** argv)
            {
               Trace trace{ "casual::gateway::outbound::reverse::local::main"};

               Arguments arguments;

               argument::Parse{ "reverse outbound",
               }( argc, argv);

               run( initialize( std::move( arguments)));
            } 

         } // <unnamed>
      } // local

   } // gateway::outbound::reverse

} // casual


int main( int argc, char** argv)
{
   return casual::common::exception::main::guard( [=]()
   {
      casual::gateway::outbound::reverse::local::main( argc, argv);
   });
} // main
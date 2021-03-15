//!
//! Copyright (c) 2020, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!

#include "gateway/group/inbound/handle.h"
#include "gateway/group/inbound/state.h"
#include "gateway/message.h"

#include "common/exception/guard.h"
#include "common/log/stream.h"
#include "common/communication/instance.h"
#include "common/argument.h"


namespace casual
{
   namespace gateway::group::inbound
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
                  communication::Socket socket;
                  configuration::model::gateway::inbound::Connection configuration;
                  platform::time::point::type created = platform::time::clock::type::now();

                  inline friend bool operator == ( const Listener& lhs, common::strong::file::descriptor::id rhs) { return lhs.socket.descriptor() == rhs;}

                  CASUAL_LOG_SERIALIZE( 
                     CASUAL_SERIALIZE( socket);
                     CASUAL_SERIALIZE( configuration);
                     CASUAL_SERIALIZE( created);
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
               Trace trace{ "gateway::group::inbound::local::initialize"};
               log::line( verbose::log, "arguments: ", arguments);

               // 'connect' to gateway-manager - will send configuration-update-request as soon as possible
               // that we'll handle in the main message pump
               communication::device::blocking::send( ipc::gateway(), gateway::message::inbound::Connect{ process::handle()});

               // 'connect' to our local domain
               common::communication::instance::whitelist::connect();

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
                           Trace trace{ "gateway::group::inbound::local::internal::handle::configuration::update::request"};
                           log::line( verbose::log, "message: ", message);

                           // TODO maintainece - make sure we can handle runtime updates...

                           state.alias = message.model.alias;
                           state.pending.requests.limit( message.model.limit);

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
                        return [&state]( message::inbound::state::Request& message)
                        {
                           Trace trace{ "gateway::group::inbound::local::handle::internal::state::request"};
                           log::line( verbose::log, "message: ", message);
                           log::line( verbose::log, "state: ", state);

                           auto reply = state.reply( message);
                           
                           reply.state.listeners = algorithm::transform( state.listeners, []( auto& listener)
                           {
                              message::state::Listener result;
                              result.address = listener.configuration.address;
                              result.descriptor = listener.socket.descriptor();
                              result.created = listener.created;

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
                           Trace trace{ "gateway::group::inbound::local::handle::internal::shutdown::request"};
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
                     return [handler = internal::handler( state)]() mutable -> strong::file::descriptor::id
                     {
                        if( handler( communication::device::non::blocking::next( ipc::inbound())))
                           return ipc::inbound().connector().descriptor();

                        return {};
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
                        return [&state]( strong::file::descriptor::id descriptor, communication::select::tag::read)
                        {
                           Trace trace{ "gateway::group::inbound::local::external::listener::dispatch"};

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
                     return [&state, handler = inbound::handle::external( state)]
                        ( common::strong::file::descriptor::id descriptor, communication::select::tag::read) mutable
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
               Trace trace{ "gateway::group::inbound::local::run"};
               log::line( verbose::log, "state: ", state);

               auto abort_guard = execute::scope( [&state]()
               {
                  gateway::group::inbound::handle::abort( state);
               });

               {
                  Trace trace{ "gateway::group::inbound::local::run dispatch pump"};

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
               Trace trace{ "gateway::group::inbound::local::main"};

               Arguments arguments;

               argument::Parse{ "inbound",
               }( argc, argv);

               run( initialize( std::move( arguments)));
            } 

         } // <unnamed>
      } // local

   } // gateway::group::inbound

} // casual


int main( int argc, char** argv)
{
   return casual::common::exception::main::guard( [=]()
   {
      casual::gateway::group::inbound::local::main( argc, argv);
   });
} // main
//!
//! Copyright (c) 2020, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!

#include "gateway/group/inbound/handle.h"
#include "gateway/group/inbound/state.h"
#include "gateway/group/handle.h"
#include "gateway/group/ipc.h"
#include "gateway/message.h"

#include "common/exception/guard.h"
#include "common/log/stream.h"
#include "common/communication/instance.h"
#include "common/argument.h"
#include "common/message/internal.h"


namespace casual
{
   namespace gateway::group::inbound
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
               ipc::flush::send( ipc::manager::gateway(), gateway::message::inbound::Connect{ process::handle()});

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
                           ipc::flush::optional::send(
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

                           ipc::flush::optional::send( message.process.ipc, reply);
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
                     common::message::internal::dump::state::handle( state),
                     handle::configuration::update::request( state),
                     handle::state::request( state),
                     handle::shutdown::request( state)
                  );
               }

            } // internal

            namespace external
            {  
  
               namespace dispatch
               {
                  auto create( State& state) 
                  { 
                     return [&state, handler = inbound::handle::external( state)]
                        ( common::strong::file::descriptor::id descriptor, communication::select::tag::read) mutable
                     {
                        Trace trace{ "gateway::group::inbound::local::external::dispatch"};

                        if( auto connection = state.external.connection( descriptor))
                        {
                           try
                           {
                              if( auto correlation = handler( connection->next()))
                                 state.correlations.emplace_back( std::move( correlation), descriptor);
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

               // make sure we listen to the death of our children
               common::signal::callback::registration< code::signal::child>( group::handle::signal::process::exit());

               {
                  Trace trace{ "gateway::group::inbound::local::run dispatch pump"};

                  // start the message dispatch
                  communication::select::dispatch::pump( 
                     local::condition( state),
                     state.directive,
                     group::tcp::pending::send::dispatch( state),
                     ipc::dispatch::create( state, &internal::handler),
                     external::dispatch::create( state),
                     tcp::listener::dispatch::create( state, tcp::connector::Bound::in)
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
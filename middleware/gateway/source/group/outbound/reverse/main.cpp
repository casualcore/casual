//!
//! Copyright (c) 2020, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!

#include "gateway/group/outbound/handle.h"
#include "gateway/group//handle.h"
#include "gateway/group/outbound/state.h"
#include "gateway/group/outbound/error/reply.h"
#include "gateway/group/ipc.h"
#include "gateway/message.h"

#include "common/exception/guard.h"
#include "common/log/stream.h"
#include "common/communication/instance.h"
#include "common/argument.h"
#include "common/message/internal.h"


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
               struct Listener
               {
                  communication::Socket socket;
                  configuration::model::gateway::outbound::Connection configuration;
                  platform::time::point::type created = platform::time::clock::type::now();

                  inline friend bool operator == ( const Listener& lhs, common::strong::file::descriptor::id rhs) { return lhs.socket.descriptor() == rhs;}

                  CASUAL_LOG_SERIALIZE( 
                     CASUAL_SERIALIZE( socket);
                     CASUAL_SERIALIZE( configuration);
                     CASUAL_SERIALIZE( created);
                  )
               };

               std::vector< Listener> listeners;
               std::vector< configuration::model::gateway::outbound::Connection> failed;
               
               CASUAL_LOG_SERIALIZE(
                  outbound::State::serialize( archive);
                  CASUAL_SERIALIZE( listeners);
                  CASUAL_SERIALIZE( failed);
               )
            };

 

            State initialize( Arguments arguments)
            {
               Trace trace{ "gateway::group::outbound::reverse::local::initialize"};
               log::line( verbose::log, "arguments: ", arguments);

               // 'connect' to gateway-manager - will send configuration-update-request as soon as possible
               // that we'll handle in the main message pump
               communication::device::blocking::send( ipc::manager::gateway(), gateway::message::outbound::Connect{ process::handle()});

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
                           Trace trace{ "gateway::group::outbound::reverse::local::internal::handle::configuration::update::request"};
                           log::line( verbose::log, "message: ", message);

                           // TODO maintainece - make sure we can handle runtime updates...

                           state.alias = message.model.alias;
                           state.order = message.order;

                           auto try_connect_listener = [&state]( auto& configuration)
                           {
                              try
                              {
                                 auto result = State::Listener{
                                    communication::tcp::socket::listen( configuration.address),
                                    configuration};

                                 // we need the socket to not block in 'accept'
                                 result.socket.set( communication::socket::option::File::no_block);

                                 state.listeners.push_back( std::move( result));
                              }
                              catch( ...)
                              {
                                 state.failed.push_back( configuration);
                              } 
                           };

                           algorithm::for_each( message.model.connections, try_connect_listener);

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
                           Trace trace{ "gateway::group::outbound::reverse::local::handle::internal::state::request"};
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

                           algorithm::copy( state.failed, reply.state.failed);

                           communication::device::blocking::optional::send( message.process.ipc, reply);
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
                              state.external.pending().exit( message.state);
                              
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
                     common::message::internal::dump::state::handle( state),
                     handle::configuration::update::request( state),
                     handle::state::request( state),
                     handle::event::process::exit( state),
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
                     return [&state, handler = gateway::group::outbound::handle::external( state)]
                        ( strong::file::descriptor::id descriptor, communication::select::tag::read) mutable
                     {
                        Trace trace{ "gateway::group::outbound::reverse::local::external::dispatch"};

                        if( auto connection = state.external.connection( descriptor))
                        {
                           try
                           {
                              state.external.last( descriptor);
                              handler( connection->next());  
                           }
                           catch( ...)
                           {
                              if( exception::capture().code() != code::casual::communication_unavailable)
                                 throw;

                               outbound::handle::connection::lost( state, descriptor);
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
                  gateway::group::tcp::pending::send::dispatch( state),
                  external::dispatch::create( state),
                  ipc::dispatch::create( state, &internal::handler),
                  tcp::listener::dispatch::create( state, tcp::connector::Bound::out)
               );
               
               abort_guard.release();
            }

            void main( int argc, char** argv)
            {
               Trace trace{ "gateway::group::outbound::reverse::local::main"};

               Arguments arguments;

               argument::Parse{ "reverse outbound",
               }( argc, argv);

               run( initialize( std::move( arguments)));
            } 

         } // <unnamed>
      } // local

   } // gateway::group::outbound::reverse

} // casual


int main( int argc, char** argv)
{
   return casual::common::exception::main::log::guard( [=]()
   {
      casual::gateway::group::outbound::reverse::local::main( argc, argv);
   });
} // main
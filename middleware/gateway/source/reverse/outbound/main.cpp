//!
//! Copyright (c) 2020, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!

#include "gateway/reverse/outbound/handle.h"
#include "gateway/reverse/outbound/state.h"
#include "gateway/message.h"

#include "common/exception/guard.h"
#include "common/log/stream.h"
#include "common/communication/instance.h"
#include  "common/argument.h"


namespace casual
{
   namespace gateway::reverse::outbound
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
                  communication::tcp::Socket socket;
                  configuration::model::gateway::reverse::outbound::Connection configuration;

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
               Trace trace{ "casual::gateway::reverse::outbound::local::initialize"};
               log::line( verbose::log, "arguments: ", arguments);

               // 'connect' to gateway-manager - will send configuration-update-request as soon as possible
               // that we'll handle in the main message pump
               communication::device::blocking::send( ipc::gateway(), gateway::message::reverse::outbound::Connect{ process::handle()});

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
                        return [&state]( gateway::message::reverse::outbound::configuration::update::Request& message)
                        {
                           Trace trace{ "gateway::reverse::outbound::local::internal::handle::configuration::update::request"};
                           log::line( verbose::log, "message: ", message);

                           // TODO maintainece - make sure we can handle runtime updates...

                           state.alias = message.model.alias;
                           state.order = message.order;

                           state.listeners = algorithm::transform( message.model.connections, []( auto& information)
                           {
                              return State::Listener{
                                 communication::tcp::socket::listen( information.address),
                                 information};
                           });

                           state.directive.read.add( 
                              algorithm::transform( state.listeners, []( auto& listener){ return listener.socket.descriptor();}));
                        };
                     }
                  } // configuration::update

                  namespace state
                  {
                     auto request( State& state)
                     {
                        return [&state]( message::reverse::outbound::state::Request& message)
                        {
                           Trace trace{ "gateway::reverse::outbound::local::handle::internal::state::request"};
                           log::line( verbose::log, "message: ", message);

                           auto reply = common::message::reverse::type( message, process::handle());
                           reply.state.alias = state.alias;
                           reply.state.order = state.order;
                           reply.state.listeners = algorithm::transform( state.listeners, []( auto& listener)
                           {
                              message::reverse::outbound::state::Listener result;
                              result.address = listener.configuration.address;
                              result.descriptor = listener.socket.descriptor();

                              return result;
                           });

                           reply.state.connections = algorithm::transform( state.external.connections, [&state]( auto& connection)
                           {
                              auto descriptor = connection.device.connector().descriptor();
                              message::reverse::outbound::state::Connection result;
                              result.descriptor = descriptor;
                              result.address.local = communication::tcp::socket::address::host( descriptor);
                              result.address.peer = communication::tcp::socket::address::peer( descriptor);

                              if( auto found = algorithm::find( state.external.information, descriptor))
                              {
                                 result.domain = found->domain;
                              }

                              return result;
                           });


                           log::line( verbose::log, "state: ", state);

                           communication::device::blocking::optional::send( message.process.ipc, reply);
                        };
                     }

                  } // state

                  auto specific( State& state)
                  {
                     return common::message::dispatch::handler( ipc::inbound(),
                        configuration::update::request( state),
                        state::request( state)
                     );
                  }

               } // handle

               using handler_type = decltype( outbound::handle::internal( std::declval< State&>()));
               struct Dispatch
               {
                  Dispatch( State& state) : m_handler( outbound::handle::internal( state) + handle::specific( state)) 
                  {
                     state.directive.read.add( ipc::inbound().connector().descriptor());
                  }

                  auto descriptor() const { return ipc::inbound().connector().descriptor();}

                  auto consume()
                  {
                     return m_handler( communication::device::non::blocking::next( ipc::inbound()));
                  } 
                  handler_type m_handler;
               };

               namespace dispatch
               {
                  auto create( State& state) { return Dispatch{ state};}   
               } // dispatch
            } // internal



            namespace external
            {
               using handler_type = decltype( outbound::handle::external( std::declval< State&>()));
               struct Dispatch
               {
                  Dispatch( State& state) : m_state{ &state}, m_handler( outbound::handle::external( *m_state)) 
                  {
                  }
                  
                  auto& descriptors() const { return m_state->external.descriptors;}

                  void read( strong::file::descriptor::id descriptor)
                  {
                     auto is_connection = [=]( auto& connection)
                     {
                        return connection.device.connector().descriptor() == descriptor;
                     };

                     if( auto found = algorithm::find_if( m_state->external.connections, is_connection))
                     {
                        m_handler( communication::device::blocking::next( found->device));
                     }
                        
                  }

               private:
                  State* m_state;
                  handler_type m_handler;
               };

               namespace dispatch
               {
                  auto create( State& state) { return Dispatch{ state};}   
               } // dispatch

               namespace listener
               {
                  struct Dispatch
                  {
                     Dispatch( State& state) : m_state{ &state}
                     {
                     }
                     
                     std::vector< strong::file::descriptor::id> descriptors() const 
                     { 
                        std::vector< strong::file::descriptor::id> result;
                        algorithm::transform( m_state->listeners, result, []( auto& listener){ return listener.socket.descriptor();});
                        return result;
                     }

                     void read( strong::file::descriptor::id descriptor)
                     {
                        Trace trace{ "gateway::reverse::outbound::local::external::listener::Dispatch::read"};

                        if( auto found = algorithm::find( m_state->listeners, descriptor))
                        {
                           log::line( verbose::log, "found: ", *found);

                           communication::tcp::Duplex device{ communication::tcp::socket::accept( found->socket)};

                           // start the connection phase against the other inbound
                           {
                              Trace trace{ "gateway::reverse::outbound::local::external::listener::Dispatch::read send domain connect request"};

                              common::message::gateway::domain::connect::Request request;
                              request.domain = domain::identity();
                              request.correlation = uuid::make();
                              request.versions = { common::message::gateway::domain::protocol::Version::version_1};
                              
                              log::line( verbose::log, "request: ", request);

                              m_state->route.message.emplace( 
                                 communication::device::blocking::send( device, request),
                                 process::handle(), 
                                 common::message::type( request), 
                                 device.connector().descriptor());
                           }

                           m_state->external.add( 
                              m_state->directive, 
                              std::move( device),
                              found->configuration);
                        }
                     }

                  private:
                     State* m_state;
                  };

                  namespace dispatch
                  {
                     auto create( State& state) { return Dispatch{ state};}   
                  } // dispatch
               } // listener
               
            } // external

            void run( State state)
            {
               Trace trace{ "casual::gateway::reverse::outbound::local::run"};
               log::line( verbose::log, "state: ", state);

               {
                  Trace trace{ "casual::gateway::reverse::outbound::local::run dispatch pump"};

                  // start the message dispatch
                  communication::select::dispatch::pump( 
                     state.directive, 
                     internal::dispatch::create( state),
                     external::dispatch::create( state),
                     external::listener::dispatch::create( state)
                  );
               }

            }

            void main( int argc, char** argv)
            {
               Trace trace{ "casual::gateway::reverse::outbound::local::main"};

               Arguments arguments;

               argument::Parse{ "reverse outbound",
               }( argc, argv);

               run( initialize( std::move( arguments)));
            } 

         } // <unnamed>
      } // local

   } // gateway::reverse::outbound

} // casual


int main( int argc, char** argv)
{
   return casual::common::exception::main::guard( [=]()
   {
      casual::gateway::reverse::outbound::local::main( argc, argv);
   });
} // main
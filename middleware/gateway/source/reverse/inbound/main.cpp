//!
//! Copyright (c) 2020, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!

#include "gateway/reverse/inbound/state.h"
#include "gateway/reverse/inbound/handle.h"
#include "gateway/message.h"

#include "common/exception/guard.h"
#include "common/signal/timer.h"
#include "common/signal.h"

#include "common/communication/instance.h"

namespace casual
{
   namespace gateway::reverse::inbound
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
               struct
               {
                  std::vector< configuration::model::gateway::reverse::inbound::Connection> connections;

                  CASUAL_LOG_SERIALIZE( 
                     CASUAL_SERIALIZE( connections);
                  )
               } pending;

               CASUAL_LOG_SERIALIZE(
                  inbound::State::serialize( archive);
                  CASUAL_SERIALIZE( pending);
               )
            };

            State initialize( Arguments arguments)
            {
               Trace trace{ "casual::gateway::reverse::inbound::local::initialize"};

               // 'connect' to gateway-manager - will send configuration-update-request as soon as possible
               // that we'll handle in the main message pump
               communication::device::blocking::send( ipc::gateway(), gateway::message::reverse::inbound::Connect{ process::handle()});

               return {};
            }

            namespace external
            {
               using handler_type = decltype( handle::external( std::declval< State&>()));
               struct Dispatch
               {
                  Dispatch( State& state) : m_state{ &state}, m_handler( handle::external( *m_state)) 
                  {
                     state.directive.read.add( m_state->external.descriptors);
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
                        if( auto correlation = m_handler( communication::device::blocking::next( found->device)))
                           m_state->correlations.emplace_back( std::move( correlation), descriptor);
                        else
                           log::line( log::category::error, code::casual::invalid_semantics, " failed to handle next message for device: ", found->device);
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

               void connect( State& state)
               {
                  Trace trace{ "casual::gateway::reverse::inbound::local::external::connect"};

                  auto connected = [&state]( auto& connection)
                  {
                     if( auto socket = communication::tcp::non::blocking::connect( connection.address))
                     {
                        state.external.add( state.directive, std::move( socket), std::move( connection.note));
                        return true;
                     }
                     return false;
                  };

                  auto& pending = state.pending.connections;
                  algorithm::trim( pending, algorithm::remove_if( pending, connected));

                  // check if we need to set a timeout to keep trying to connect
                  if( ! pending.empty())
                     common::signal::timer::set( std::chrono::seconds{ 2});

                  log::line( verbose::log, "state: ", state);
               }

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
                        return [&state]( gateway::message::reverse::inbound::configuration::update::Request& message)
                        {
                           Trace trace{ "gateway::reverse::inbound::local::internal::handle::configuration::update::request"};
                           log::line( verbose::log, "message: ", message);

                           // TODO maintainece - make sure we can handle runtime updates...

                           state.messages.limit( message.model.limit);
                           state.pending.connections = std::move( message.model.connections);

                           // we might got some addresses to try...
                           external::connect( state);
                        };
                     }
                  } // configuration::update

                  auto specific( State& state)
                  {
                     return common::message::dispatch::handler( ipc::inbound(),
                        configuration::update::request( state)
                     );
                  }

               } // handle

               using handler_type = decltype( inbound::handle::internal( std::declval< State&>()));
               struct Dispatch
               {
                  Dispatch( State& state) : m_handler( inbound::handle::internal( state) + local::internal::handle::specific( state)) 
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

            namespace signal::callback
            {
               auto timeout( State& state)
               {
                  return [&state]()
                  {
                     Trace trace{ "casual::gateway::reverse::inbound::local::signal::callback::timeout"};

                     external::connect( state);                     
                  };
               }
            } // signal::callback

            void run( State state)
            {
               Trace trace{ "casual::gateway::reverse::inbound::local::run"};
               log::line( verbose::log, "state: ", state);

               // register the alarm callback.
               common::signal::callback::registration< code::signal::alarm>( signal::callback::timeout( state));

               // we try to connect before we enter message pump
               external::connect( state);

               {
                  Trace trace{ "casual::gateway::reverse::inbound::local::run dispatch pump"};

                  // start the message dispatch
                  communication::select::dispatch::pump( 
                     state.directive, 
                     internal::dispatch::create( state),
                     external::dispatch::create( state)
                  );
               }
            }

            void main( int argc, char** argv)
            {
               Arguments arguments;

               run( initialize( std::move( arguments)));
            } 

         } // <unnamed>
      } // local

   } // gateway::reverse::inbound

} // casual


int main( int argc, char** argv)
{
   return casual::common::exception::main::guard( [=]()
   {
      casual::gateway::reverse::inbound::local::main( argc, argv);
   });
} // main
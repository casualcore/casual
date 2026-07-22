//!
//! Copyright (c) 2026, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!

#include "common/exception/guard.h"

#include "common/log.h"
#include "common/communication/tcp.h"
#include "common/communication/select.h"



#include "casual/argument.h"

namespace casual
{
   namespace common::unittest::tcp::server
   {
      namespace local
      {
         namespace
         {
            struct Settings
            {
               struct
               {
                  std::string address;

                  CASUAL_LOG_SERIALIZE(
                     CASUAL_SERIALIZE( address);
                  )

               } listen;

               CASUAL_LOG_SERIALIZE(
                  CASUAL_SERIALIZE( listen);
               )
            };

            auto validate( Settings settings)
            {
               if( settings.listen.address.empty())
                  code::raise::error( code::casual::invalid_argument, "no listen address provided");

               return settings;
            }

            struct State
            {
               common::communication::select::Directive directive;
               common::communication::Socket listener;

               std::vector< communication::tcp::Duplex> connections;

               CASUAL_LOG_SERIALIZE(
                  CASUAL_SERIALIZE( listener);
                  CASUAL_SERIALIZE( connections);
               )
            };

            State transform( Settings settings)
            {
               auto state = State{ .listener =  communication::tcp::socket::listen( std::move( settings.listen.address))};
               state.directive.read_add( state.listener.descriptor());

               // we need the socket to not block in 'accept'
               state.listener.set( communication::socket::option::File::no_block);

               log::debug( "state: ", state);

               return state;
            }

            namespace dispatch
            {
               auto listen( State& state)
               {
                  return [ &state]( strong::file::descriptor::id descriptor, communication::select::tag::read)
                  {
                     Trace trace{ "common::unittest::tcp::server::local::dispatch::listen"};

                     if( descriptor == state.listener.descriptor())
                     {
                        auto socket = common::communication::tcp::socket::accept( state.listener);

                        log::information( "connected - host: ", communication::tcp::socket::address::host( socket), " peer: ", communication::tcp::socket::address::peer( socket));

                        state.directive.read_add( socket.descriptor());

                        state.connections.emplace_back( std::move( socket));

                        return true;
                     }
                     return false;
                  };
               }

               auto echo( State& state)
               {
                  return [ &state]( common::strong::file::descriptor::id descriptor, common::communication::select::tag::read)
                  {
                     Trace trace{ "common::unittest::tcp::server::local::dispatch::echo"};

                     if( auto connection = common::algorithm::find( state.connections, descriptor))
                     {
                        if( auto complete = common::communication::device::non::blocking::next( *connection))
                        {
                           log::debug( "received: ", complete);

                           // we need to set the offset to 0. Offset is set to the end of the 
                           // buffer when reading is done and we have a complete message. Offset is used 
                           // when sending to keep track of what has been sent, hence we need to reset it before sending.
                           complete.offset = 0;
                           common::communication::device::blocking::send( *connection, std::move( complete));
                        }
                        
                        return true;
                     }
                     return false;
                  };
               }
            } // dispatch

            void run( State state)
            {
               Trace trace{ "common::unittest::tcp::server::local::run"};
               log::debug( "state: ", state);

               // start the message dispatch
               communication::select::dispatch::pump( 
                  state.directive,
                  dispatch::listen( state),
                  dispatch::echo( state)
               );

            }


            void main( int argc, const char **argv)
            {
               Trace trace{ "common::unittest::tcp::server::local::main"};

               Settings settings;

               auto outcome = argument::parse( "common::unittest::tcp::server", {
                  argument::Option( std::tie( settings.listen.address), { { "--listen"}}, "address to listen on")( argument::cardinality::one())
               }, argc, argv);

               if( outcome == argument::Outcome::parsed)
                  local::run( local::transform( local::validate( std::move( settings))));
            }
            
         } // <unnamed>
      } // local
      
   } // common::unittest::tcp::server
   
} // casual

int main( int argc, const char **argv)
{
   return casual::common::exception::main::cli::guard( [=]()
   {
      casual::common::unittest::tcp::server::local::main( argc, argv);
   });
}


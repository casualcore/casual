//!
//! Copyright (c) 2021, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!

#include "gateway/message.h"
#include "gateway/common.h"

#include "common/exception/guard.h"

#include "common/argument.h"
#include "common/strong/id.h"
#include "common/communication/tcp.h"
#include "common/communication/ipc.h"
#include "common/signal/timer.h"

namespace casual
{
   namespace gateway::group::tcp::logical::connect
   {
      using namespace common;

      namespace local
      {
         namespace
         {
            struct Settings
            {
               strong::socket::id::value_type descriptor;
               std::string ipc;
               std::string bound;

            };

            auto create_socket( strong::socket::id::value_type descriptor)
            {
               communication::Socket socket{ strong::socket::id{ descriptor}};
               
               // socket needs to be blocking
               socket.unset( common::communication::socket::option::File::no_block);
               return socket;
            }

            struct State
            {
               State( Settings settings)
                  : device{ create_socket( settings.descriptor)},
                     destination{ Uuid{ settings.ipc}}
               {}

               communication::tcp::Duplex device;
               strong::ipc::id destination;

               // Even if we block to send to destination, we won't be "signaled" if the destination
               // is removed. We need to retry the blocking send, from time to tome.
               // This could only happen if parent in/out-bound cores, or something similar
               common::signal::timer::Deadline timer{ platform::tcp::connect::attempts::delay};
            };

            namespace signal::timer
            {
               common::signal::timer::Deadline deadline{ platform::tcp::connect::attempts::delay};

               auto callback()
               {
                  return []() 
                  { 
                     // just reset the signal.
                     signal::timer::deadline = common::signal::timer::Deadline{ platform::tcp::connect::attempts::delay};
                  };
               }
            } // signal::timer


            auto initialize( Settings settings)
            {
               Trace trace{ "gateway::group::tcp::connector::local::initialize"};

               State state{ std::move( settings)};

               // register the timer callback.
               common::signal::callback::registration< code::signal::alarm>( signal::timer::callback());

               return state;
            }


            namespace run
            {
               void in( State state)
               {
                  Trace trace{ "gateway::group::tcp::connector::local::run::in"};

                  gateway::message::domain::connect::Request request;

                  communication::device::blocking::receive( state.device, request);
                  log::line( verbose::log, "request: ", request);

                  auto reply = common::message::reverse::type( request);

                  if( auto found = algorithm::find_first_of( message::protocol::versions, request.versions))
                     reply.version = *found;

                  reply.domain = common::domain::identity();

                  communication::device::blocking::send( state.device, reply);

                  if( algorithm::none_of( gateway::message::protocol::versions, predicate::value::equal( reply.version)))
                     code::raise::error( code::casual::invalid_version, "invalid protocol version: ", reply.version);

                  gateway::message::domain::Connected connected;
                  connected.connector = process::id();
                  connected.domain = request.domain;
                  connected.version = reply.version;

                  communication::device::blocking::send( state.destination, connected);
               }

               void out( State state)
               {
                  Trace trace{ "gateway::group::tcp::connector::local::run::out"};

                  gateway::message::domain::connect::Request request;
                  request.domain = common::domain::identity();
                  request.versions = algorithm::container::vector::create( gateway::message::protocol::versions);

                  auto reply = common::message::reverse::type( request);
                  
                  communication::device::blocking::receive( state.device, reply, communication::device::blocking::send( state.device, request));

                  if( algorithm::none_of( gateway::message::protocol::versions, predicate::value::equal( reply.version)))
                     code::raise::error( code::casual::invalid_version, "invalid protocol version: ", reply.version);

                  gateway::message::domain::Connected connected;
                  connected.connector = process::id();
                  connected.domain = reply.domain;
                  connected.version = reply.version;

                  communication::device::blocking::send( state.destination, connected);
               }

            } // run

            
            void main( int argc, char** argv)
            {
               Trace trace{ "gateway::group::tcp::connector::local::main"};

               Settings settings;

               argument::Parse{ "responsible for the logical connection phase to another domain",
                  argument::Option{ std::tie( settings.descriptor), { "--descriptor"}, "tcp descriptor"}( argument::cardinality::one{}),
                  argument::Option{ std::tie( settings.ipc), { "--ipc"}, "where to send the completed reply"}( argument::cardinality::one{}),
                  argument::Option{ std::tie( settings.bound), { "--bound"}, "[inbound|outbound] outbound instigate the connections phase, inbound the opposite"}( argument::cardinality::one{})
               }( argc, argv);

               if( settings.bound == "in")
                  run::in( initialize( std::move( settings)));
               else if( settings.bound == "out")
                  run::out( initialize( std::move( settings)));
               else 
                  code::raise::error( code::casual::invalid_argument, "--bound has to be [in|out]");

               
            } // main
         } // <unnamed>
      } // local
      
   } // gateway::group::tcp::logical::connect
   
} // casual

int main( int argc, char** argv)
{
   return casual::common::exception::main::log::guard( [=]()
   {
      casual::gateway::group::tcp::logical::connect::local::main( argc, argv);
   });
} // main
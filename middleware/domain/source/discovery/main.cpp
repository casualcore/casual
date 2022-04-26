//!
//! Copyright (c) 2021, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!

#include "domain/discovery/state.h"
#include "domain/discovery/instance.h"
#include "domain/discovery/handle.h"
#include "domain/discovery/common.h"

#include "common/communication/select.h"
#include "common/exception/guard.h"
#include "common/argument.h"
#include "common/communication/instance.h"

namespace casual
{
   using namespace common;

   namespace domain::discovery
   {
      namespace local
      {
         namespace
         {
            struct Settings
            {
            };

            auto initialize( Settings settings)
            {
               Trace trace{ "domain::discovery::local::initialize"};

               communication::instance::whitelist::connect( discovery::instance::identity);


               return State{};
            }

            auto condition( State& state)
            {
               return common::message::dispatch::condition::compose(
                  common::message::dispatch::condition::done( [&state]() { return state.done();})
               );
            }

            namespace dispatch
            {
               auto handler( State& state)
               {
                  return [ handler = handle::create( state)]( common::strong::file::descriptor::id descriptor, common::communication::select::tag::read) mutable
                  {
                     auto& inbound = common::communication::ipc::inbound::device();
                     if( inbound.descriptor() != descriptor)
                        return false;
                     
                     handler( common::communication::device::non::blocking::next( inbound));
                     return true;
                  };
               }
            } // dispatch

            void start( State state)
            {
               Trace trace{ "domain::discovery::local::start"};
               log::line( verbose::log, "state: ", state);

               auto handler = handle::create( state);

               // start the message dispatch
               communication::select::dispatch::pump( 
                  local::condition( state),
                  state.directive,
                  dispatch::handler( state),
                  state.multiplex
               );
            }

            void main( int argc, char** argv)
            {
               Trace trace{ "domain::discovery::local::main"};
               
               Settings settings;
               {
                  using namespace casual::common::argument;
                  Parse{ R"(responsible for orchestrating discoveries to and from other domains.)",
                  }( argc, argv);
               }

               start( initialize( std::move( settings)));
            }
            
         } // <unnamed>
      } // local

   } // domain::discovery
} // casual

int main( int argc, char** argv)
{
   casual::common::exception::main::log::guard( [=]()
   {
      casual::domain::discovery::local::main( argc, argv);
   });
} 
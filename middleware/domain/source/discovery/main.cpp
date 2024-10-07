//!
//! Copyright (c) 2021, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!

#include "domain/discovery/state.h"
#include "domain/discovery/instance.h"
#include "domain/discovery/handle.h"
#include "domain/discovery/common.h"
#include "domain/configuration.h"

#include "common/communication/select.h"
#include "common/communication/select/ipc.h"
#include "common/exception/guard.h"
#include "casual/argument.h"
#include "common/communication/instance.h"
#include "common/message/signal.h"

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

               State state;

               // rig the configuration setup
               {
                  using Ability = casual::domain::configuration::registration::Ability;
                  casual::domain::configuration::registration::apply( Ability::runtime_update);

                  handle::configuration_update( state, casual::domain::configuration::fetch());
               }

               return state;
            }

            namespace signal::callback
            {
               auto timeout()
               {
                  return []()
                  {
                     Trace trace{ "domain::discovery::local::signal::callback::timeout"};

                     // we push it to our own inbound ipc 'queue', and handle the timeout
                     // in our regular message pump.
                     communication::ipc::inbound::device().push( common::message::signal::Timeout{});                 
                  };
               }
            } // signal::callback

            auto condition( State& state)
            {
               return common::message::dispatch::condition::compose(
                  common::message::dispatch::condition::done( [&state]() { return state.done();})
               );
            }

            void start( State state)
            {
               Trace trace{ "domain::discovery::local::start"};
               log::line( verbose::log, "state: ", state);

               // register the alarm callback.
               common::signal::callback::registration< code::signal::alarm>( signal::callback::timeout());

               // start the message dispatch
               communication::select::dispatch::pump( 
                  local::condition( state),
                  state.directive,
                  communication::select::ipc::dispatch::create( state, &handle::create),
                  state.multiplex
               );

               log::line( verbose::log, "state: ", state);
            }

            void main( int argc, const char** argv)
            {
               Trace trace{ "domain::discovery::local::main"};
               
               Settings settings;
               {
                  argument::parse( R"(responsible for orchestrating discoveries to and from other domains.)", {
                  }, argc, argv);
               }

               start( initialize( std::move( settings)));
            }
            
         } // <unnamed>
      } // local

   } // domain::discovery
} // casual

int main( int argc, const char** argv)
{
   casual::common::exception::main::log::guard( [=]()
   {
      casual::domain::discovery::local::main( argc, argv);
   });
} 
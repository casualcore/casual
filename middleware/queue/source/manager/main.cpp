//! 
//! Copyright (c) 2015, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!

#include "queue/manager/state.h"
#include "queue/manager/handle.h"
#include "queue/common/log.h"


#include "casual/argument.h"
#include "common/exception/capture.h"
#include "common/message/dispatch.h"
#include "common/environment.h"
#include "common/environment/normalize.h"
#include "common/communication/select/ipc.h"

#include "domain/configuration.h"
#include "domain/discovery/api.h"

namespace casual
{
   using namespace common;
   namespace queue::manager
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
               Trace trace( "queue::manager::local::initialize");

               // Set environment variable so children can find us easy
               common::environment::variable::set(
                  common::environment::variable::name::ipc::queue::manager,
                  common::process::handle());

               State state;

               // We ask the domain manager for configuration, and 'comply' to it...
               handle::comply::configuration( state, casual::domain::configuration::fetch());

               return state;
            }

            auto condition( State& state)
            {
               return message::dispatch::condition::compose(
                  message::dispatch::condition::done( [ &state]() { return state.done();}),
                  message::dispatch::condition::idle( [ &state](){ handle::idle( state);})
               );
            }

            void start( State state)
            {
               Trace trace( "queue::manager::local::start");

               auto abort = execute::scope( [&state](){ manager::handle::abort( state);});

               // make sure we handle death of our children
               signal::callback::registration< common::code::signal::child>( [ &state]()
               {
                  for( auto& exit : process::lifetime::ended())
                     manager::handle::process::exit( state, exit);
               });

               // register that we can answer discovery questions.
               {
                  using Ability = casual::domain::discovery::provider::Ability;
                  casual::domain::discovery::provider::registration( Ability::lookup | Ability::fetch_known);
               }

               // we can supply configuration
               {
                  using Ability = casual::domain::configuration::registration::Ability;
                  casual::domain::configuration::registration::apply( Ability::supply | Ability::runtime_update);
               }
               

               common::log::line( common::log::category::information, "casual-queue-manager is on-line");
               common::log::line( verbose::log, "state: ", state);
               
               communication::select::dispatch::pump(
                  local::condition( state),
                  state.directive,
                  state.multiplex,
                  communication::select::ipc::dispatch::create( state, &handle::create));

               // cancel the abort
               abort.release();
            }

            void main( int argc, const char **argv)
            {
               Settings settings;
               {
                  argument::parse( R"(Manages casual queue, the provided queue functionality)", {}, argc, argv);
               }

               start( initialize( std::move( settings)));
            }

         } // <unnamed>
      } // local
         
   } // queue::manager
} // casual

int main( int argc, const char** argv)
{
   return casual::common::exception::main::log::guard( [=]()
   {
      casual::queue::manager::local::main( argc, argv);
   });
}



//! 
//! Copyright (c) 2015, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!

#include "queue/manager/state.h"
#include "queue/manager/handle.h"
#include "queue/common/log.h"


#include "common/argument.h"
#include "common/exception/handle.h"
#include "common/message/dispatch.h"
#include "common/environment.h"
#include "common/environment/normalize.h"

#include "domain/configuration/fetch.h"
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

               // make sure we handle death of our children
               signal::callback::registration< code::signal::child>( []()
               {
                  algorithm::for_each( process::lifetime::ended(), []( auto& exit)
                  {
                     manager::handle::process::exit( exit);
                  });
               });

               // Set environment variable so children can find us easy
               common::environment::variable::process::set(
                  common::environment::variable::name::ipc::queue::manager,
                  common::process::handle());

               State state;

               // We ask the domain manager for configuration, and 'comply' to it...
               handle::comply::configuration( state, casual::domain::configuration::fetch().queue);

               return state;
            }

            namespace wait
            {
               //! waits for the 'entities' to be running (or worse)
               template< typename H>
               void running( State& state, H& handler)
               {
                  Trace trace( "queue::manager::local::wait::running");
                  
                  auto condition = message::dispatch::condition::compose(
                     message::dispatch::condition::done( [&state]() { return state.ready();})
                  );

                  message::dispatch::pump( 
                     condition,
                     handler, 
                     ipc::device());

                  // Connect to domain
                  common::communication::instance::whitelist::connect( common::communication::instance::identity::queue::manager);

                  // register that we can answer discovery questions.
                  using Ability = casual::domain::discovery::provider::Ability;
                  casual::domain::discovery::provider::registration( flags::compose( Ability::discover_internal, Ability::needs));
               }  
            } // wait

            auto condition( State& state)
            {
               return message::dispatch::condition::compose(
                  message::dispatch::condition::done( [&state]() { return state.done();})
               );
            }

            void start( State state)
            {
               Trace trace( "queue::manager::local::start");

               auto abort = execute::scope( [&state](){ manager::handle::abort( state);});

               auto handler = manager::handlers( state);

               wait::running( state, handler);

               // we can supply configuration
               casual::domain::configuration::supplier::registration();

               common::log::line( common::log::category::information, "casual-queue-manager is on-line");
               common::log::line( verbose::log, "state: ", state);
               
               message::dispatch::pump( 
                  local::condition( state),
                  handler, 
                  ipc::device());

               // cancel the abort
               abort.release();
            }

            void main( int argc, char **argv)
            {
               Settings settings;
               {
                  using namespace casual::common::argument;
                  Parse{ R"(
Manages casual queue, the provided queue functionality.
)",
                  }( argc, argv);
               }

               start( initialize( std::move( settings)));
            }

         } // <unnamed>
      } // local
         
   } // queue::manager
} // casual

int main( int argc, char **argv)
{
   return casual::common::exception::main::log::guard( [=]()
   {
      casual::queue::manager::local::main( argc, argv);
   });
}



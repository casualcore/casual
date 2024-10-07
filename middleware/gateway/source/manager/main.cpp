//! 
//! Copyright (c) 2015, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!



#include "gateway/manager/state.h"
#include "gateway/manager/handle.h"
#include "gateway/manager/transform.h"
#include "gateway/manager/configuration.h"

#include "gateway/environment.h"
#include "gateway/common.h"

#include "domain/configuration.h"

#include "casual/argument.h"
#include "common/environment.h"
#include "common/exception/capture.h"
#include "common/communication/instance.h"


namespace casual
{
   using namespace common;

   namespace gateway::manager
   {
      namespace local
      {
         namespace
         {
            State initialize()
            {
               Trace trace{ "gateway::manager::local::connect"};

               // make sure we handle death of our children
               signal::callback::registration< code::signal::child>( []()
               {
                  algorithm::for_each( process::lifetime::ended(), []( auto& exit)
                  {
                     manager::handle::process::exit( exit);
                  });
               }); 

               // Set environment variable to make it easier for connections to get in
               // touch with us
               common::environment::variable::set(
                  common::environment::variable::name::ipc::gateway::manager,
                  process::handle());

               
               return {};
            }

            auto condition( State& state)
            {
               return common::message::dispatch::condition::compose(
                  common::message::dispatch::condition::done( [&state](){ return state.done();})
               );
            }

            void start( State state)
            {
               Trace trace{ "gateway::Manager::start"};

               auto abort_guard = execute::scope( [&state]()
               {
                  handle::shutdown( state);
               });

               // boot procedure
               {
                  configuration::conform( state, casual::domain::configuration::fetch());

                  // will execute after all configuration tasks are done
                  auto configuration_done = [ &state]( task::unit::id)
                  {
                     // Connect to domain
                     communication::instance::whitelist::connect( communication::instance::identity::gateway::manager);

                     using Ability = casual::domain::configuration::registration::Ability;
                     casual::domain::configuration::registration::apply( Ability::supply | Ability::runtime_update);

                     state.runlevel = manager::state::Runlevel::running;

                     log::line( log::category::information, "casual-gateway-manager is online");

                     return task::unit::action::Outcome::abort;
                  };

                  state.tasks.then( task::create::unit( 
                     task::create::action( "configuration_done", std::move( configuration_done))
                  ));
                    
               }

               // start message pump
               common::message::dispatch::pump(
                  manager::local::condition( state),
                  manager::handler( state),
                  manager::ipc::inbound());

               abort_guard.release();
            }

            void main( int argc, const char** argv)
            {
               // to provide --help, only for consistency
               argument::parse( "Responsible for interdomain communications", {
                  }, argc, argv);

               start( initialize());
            }

         } // <unnamed>
      } // local



   } // gateway::manager
} // casual


int main( int argc, const char** argv)
{
   return casual::common::exception::main::log::guard( [=]()
   {
       casual::gateway::manager::local::main( argc, argv);
   });
}


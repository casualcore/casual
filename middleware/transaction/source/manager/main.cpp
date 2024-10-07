//! 
//! Copyright (c) 2015, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!

#include "transaction/manager/state.h"
#include "transaction/common.h"
#include "transaction/manager/handle.h"
#include "transaction/manager/transform.h"
#include "transaction/manager/configuration.h"

#include "domain/configuration.h"


#include "casual/argument.h"
#include "common/process.h"
#include "common/exception/guard.h"
#include "common/environment.h"
#include "common/communication/instance.h"
#include "common/communication/select/ipc.h"



namespace casual
{
   using namespace common;

   namespace transaction::manager
   {
      namespace local
      {
         namespace
         {
            struct Settings
            {
               CASUAL_CONST_CORRECT_SERIALIZE()
            };

            State initialize( Settings settings)
            {
               Trace trace( "transaction::manager:local::initialize");
               log::line( verbose::log, "settings: ", settings);

               // Set the process variables so children can find us easier.
               environment::variable::set(
                  environment::variable::name::ipc::transaction::manager,
                  process::handle());

               signal::callback::registration< code::signal::child>( []()
               {
                  algorithm::for_each( process::lifetime::ended(), []( auto& exit)
                  {
                     manager::handle::process::exit( exit);
                  });
               }); 

               return {};
            }

            void configure( State& state)
            {
               configuration::conform( state, casual::domain::configuration::fetch());

               // when all those configuration tasks, if any, are completed, we connect

               auto connect_task = [ &state]( task::unit::id)
               {
                  log::line( common::log::category::information, "casual-transaction-manager is on-line");
                  log::line( verbose::log, "state: ", state);

                  // Connect to domain
                  communication::instance::whitelist::connect( communication::instance::identity::transaction::manager);
                  return task::unit::action::Outcome::abort;
               };

               state.task.coordinator.then( task::create::action( std::move( connect_task)));
            }


            auto condition( State& state)
            {
               return message::dispatch::condition::compose(
                  message::dispatch::condition::done( [ &state]() { return state.done();}),
                  // if input device is 'idle', we persist and send persistent replies, if any.
                  message::dispatch::condition::idle( [&state]() { manager::handle::persist::send( state);})
               );
            }

            void start( State state)
            {
               Trace trace( "transaction::manager:local::start");

               auto abort_guard = execute::scope( [&state](){
                  handle::abort( state);
               });

               configure( state);

               // we can supply configuration and runtime update
               {
                  using Ability = casual::domain::configuration::registration::Ability;
                  casual::domain::configuration::registration::apply( Ability::supply | Ability::runtime_update);
               }

               communication::select::dispatch::pump(
                  local::condition( state),
                  state.directive,
                  communication::select::ipc::dispatch::create( state, &handle::handlers),
                  state.multiplex
               );

               abort_guard.release();
            }

            void main( int argc, const char** argv)
            {
               Trace trace( "transaction::manager:local::main");

               Settings settings;
               {
                  argument::parse( "transaction manager", {}, argc, argv);
               }

               start( initialize( std::move( settings)));
            }

         } // <unnamed>
      } // local
   } // transaction::manager
} // casual


int main( int argc, const char** argv)
{
   return casual::common::exception::main::fatal::guard( [=]()
   {
      casual::transaction::manager::local::main( argc, argv);
   });
}


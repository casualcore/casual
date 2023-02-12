//! 
//! Copyright (c) 2015, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!

#include "transaction/manager/state.h"
#include "transaction/common.h"
#include "transaction/manager/handle.h"
#include "transaction/manager/action.h"
#include "transaction/manager/transform.h"

#include "domain/configuration.h"


#include "common/argument.h"
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

            auto initialize( Settings settings)
            {
               Trace trace( "transaction::manager:local::initialize");
               log::line( verbose::log, "settings: ", settings);

               // Set the process variables so children can find us easier.
               environment::variable::process::set(
                  environment::variable::name::ipc::transaction::manager,
                  process::handle());

               return transform::state( casual::domain::configuration::fetch());
            }

            void setup( State& state)
            {
               Trace trace( "transaction::manager:local::setup");

               // make sure we handle death of our children
               signal::callback::registration< code::signal::child>( []()
               {
                  algorithm::for_each( process::lifetime::ended(), []( auto& exit)
                  {
                     manager::handle::process::exit( exit);
                  });
               }); 

               // Start resource-proxies
               {
                  Trace trace{ "transaction::manager:local::setup start rm-proxy-servers"};

                  common::algorithm::for_each(
                     state.resources,
                     action::resource::scale::instances( state));

                  // Make sure we wait for the resources to get ready
                  namespace dispatch = common::message::dispatch;
                  dispatch::pump( 
                     dispatch::condition::compose( dispatch::condition::done( [&](){ return state.booted();})),
                     manager::handle::startup::handlers( state),
                     manager::ipc::device());
               }
            }


            auto condition( State& state)
            {
               return message::dispatch::condition::compose(
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

               setup( state);

               // we can supply configuration
               {
                  using Ability = casual::domain::configuration::registration::Ability;
                  casual::domain::configuration::registration::apply( Ability::supply);
               }

               // Connect to domain
               communication::instance::whitelist::connect( communication::instance::identity::transaction::manager);

               log::line( common::log::category::information, "casual-transaction-manager is on-line");
               log::line( verbose::log, "state: ", state);

               communication::select::dispatch::pump(
                  local::condition( state),
                  state.directive,
                  communication::select::ipc::dispatch::create( state, &handle::handlers),
                  state.multiplex
               );

               abort_guard.release();
            }

            void main( int argc, char** argv)
            {
               Trace trace( "transaction::manager:local::main");

               Settings settings;
               {
                  argument::Parse{ "transaction manager",
                  }( argc, argv);
               }

               start( initialize( std::move( settings)));
            }

         } // <unnamed>
      } // local
   } // transaction::manager
} // casual


int main( int argc, char** argv)
{
   return casual::common::exception::main::fatal::guard( [=]()
   {
      casual::transaction::manager::local::main( argc, argv);
   });
}


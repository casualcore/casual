//! 
//! Copyright (c) 2015, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!


#include "service/manager/state.h"
#include "service/manager/handle.h"
#include "service/forward/instance.h"
#include "service/common.h"

#include "domain/configuration/fetch.h"
#include "domain/discovery/api.h"

#include "common/communication/select.h"
#include "common/exception/capture.h"
#include "common/argument.h"
#include "common/environment.h"
#include "common/communication/instance.h"
#include "common/communication/select/ipc.h"
#include "common/message/dispatch.h"

#include <iostream>

namespace casual
{
   using namespace common;

   namespace service::manager
   {
      namespace local
      {
         namespace
         {
            struct Settings
            {
               CASUAL_LOG_SERIALIZE()
            };


            auto initialize( Settings settings)
            {
               Trace trace( "service::manager:local::initialize");


               // Set the process variables so children can find us easier.
               environment::variable::process::set(
                  environment::variable::name::ipc::service::manager,
                  process::handle());

               State state;

               // We ask the domain manager for configuration, and 'comply' to it...
               handle::comply::configuration( state, casual::domain::configuration::fetch());

               // register so domain-manager can fetch configuration from us.
               casual::domain::configuration::supplier::registration();

               return state;
            }

            void setup( State& state)
            {
               Trace trace( "service::manager:local::setup");

               // make sure we handle death of our children
               signal::callback::registration< code::signal::child>( []()
               {
                  algorithm::for_each( process::lifetime::ended(), []( auto& exit)
                  {
                     manager::handle::process::exit( exit);
                  });
               }); 

               signal::callback::registration< code::signal::alarm>( [&state]()
               {
                  handle::timeout( state);
               });

               // Start forward
               {
                  Trace trace{ "service::manager:local::setup spawn forward"};

                  state.forward = common::Process( process::path().parent_path() / "casual-service-forward");
                  state.forward.ipc = common::communication::instance::fetch::handle( forward::instance::identity.id).ipc;

                  log::line( log, "forward: ", state.forward);
               }
            }


            auto condition( State& state)
            {
               return message::dispatch::condition::compose(
                  message::dispatch::condition::done( [&state]() { return state.done();}),
                  message::dispatch::condition::idle( [&state]()
                  {
                     // the input socket is empty, we can't know if there ever gonna be any more 
                     // messages to read, we need to send metric, if any...
                     if( state.metric && state.events.active< common::message::event::service::Calls>())
                        manager::handle::metric::send( state);
                  })
               );
            }

            void start( State state)
            {
               Trace trace( "service::manager:local::start");

               setup( state);

               // register that we can answer discovery questions.
               using Ability = casual::domain::discovery::provider::Ability;
               casual::domain::discovery::provider::registration( flags::compose( Ability::discover_internal, Ability::known));

               // Connect to domain
               communication::instance::whitelist::connect( communication::instance::identity::service::manager);

               log::line( common::log::category::information, "casual-service-manager is on-line");
               log::line( verbose::log, "state: ", state);

               communication::select::dispatch::pump(
                  local::condition( state),
                  state.directive,
                  communication::select::ipc::dispatch::create( state, &handle::create),
                  state.multiplex
               );
            }

            void main( int argc, char** argv)
            {
               Trace trace( "service::manager:local::main");

               Settings settings;

               argument::Parse{ "Responsible for service related informaiton, such as service lookups",
               }( argc, argv);

               start( initialize( std::move( settings)));

            }
         } // <unnamed>
      } // local

   } // service::manager

} // casual


int main( int argc, char** argv)
{
   return casual::common::exception::main::log::guard( [=]()
   {  
      casual::service::manager::local::main( argc, argv);
   });
}

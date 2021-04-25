//! 
//! Copyright (c) 2015, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!



#include "gateway/manager/state.h"
#include "gateway/manager/handle.h"

#include "gateway/environment.h"
#include "gateway/transform.h"
#include "gateway/common.h"

#include "domain/configuration/fetch.h"

#include "common/argument.h"
#include "common/environment.h"
#include "common/exception/handle.h"
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
               common::environment::variable::process::set(
                  common::environment::variable::name::ipc::gateway::manager,
                  process::handle());

               // Ask domain manager for configuration
               return gateway::transform::state( casual::domain::configuration::fetch().gateway);
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

               // boot outbounds
               manager::handle::boot( state);

               // Connect to domain
               communication::instance::whitelist::connect( communication::instance::identity::gateway::manager);

               state.runlevel = manager::state::Runlevel::running;

               log::line( log::category::information, "casual-gateway-manager is online");

               // start message pump
               common::message::dispatch::pump(
                  manager::local::condition( state),
                  manager::handler( state),
                  manager::ipc::inbound());

               abort_guard.release();
            }

            void main( int argc, char **argv)
            {
               // to provide --help, only for consistency
               common::argument::Parse{
                  R"(Responsible for interdomain communications.
)"
               }( argc, argv);

               start( initialize());
            }

         } // <unnamed>
      } // local



   } // gateway::manager
} // casual


int main( int argc, char **argv)
{
   return casual::common::exception::main::log::guard( [=]()
   {
       casual::gateway::manager::local::main( argc, argv);
   });
}


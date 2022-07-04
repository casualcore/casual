//! 
//! Copyright (c) 2015, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!


#include "domain/manager/state.h"
#include "domain/manager/handle.h"
#include "domain/manager/transform.h"
#include "domain/manager/task/event.h"
#include "domain/common.h"

#include "configuration/model/load.h"

#include "common/communication/ipc.h"
#include "common/exception/capture.h"
#include "common/argument.h"
#include "common/string/compose.h"

namespace casual
{
   using namespace common;
   namespace domain::manager
   {
      namespace local
      {
         namespace
         {
            struct Settings
            {
               std::vector< std::string> configuration;

               bool bare = false;

               struct
               {
                  common::strong::ipc::id ipc;
                  common::strong::correlation::id correlation;

                  CASUAL_LOG_SERIALIZE(
                     CASUAL_SERIALIZE( ipc);
                     CASUAL_SERIALIZE( correlation);
                  )
               } parent;

               CASUAL_LOG_SERIALIZE(
                  CASUAL_SERIALIZE( configuration);
                  CASUAL_SERIALIZE( bare);
                  CASUAL_SERIALIZE( parent);
               )
            };


            auto initialize( Settings settings)
            {
               Trace trace{ "domain::manager::local::initialize"};
               log::line( verbose::log, "settings: ", settings);

               const auto files = common::file::find( settings.configuration);
               log::line( log::category::information, "configuration files used: ", files);

               auto state = transform::model( casual::configuration::model::load( files));


               //! if we've got a parent, we make sure it gets events.
               if( settings.parent.ipc)
               {
                  common::message::event::subscription::Begin request;
                  request.process.ipc = settings.parent.ipc;
                  state.event.subscription( request);
               }

               state.bare = settings.bare;
               state.singleton_file = common::domain::singleton::create();

               manager::task::event::dispatch( state, [&]()
               {
                  common::message::event::Notification event{ process::handle()};
                  event.correlation = settings.parent.correlation;
                  event.message = string::compose( "configuration files used: ", files);
                  return event;
               });

         
               // make sure we handle death of our children
               signal::callback::registration< code::signal::child>( []()
               {
                  for( auto& exit : process::lifetime::ended())
                     manager::handle::process::exit( exit);
               });

               // make sure we're whitelisted from assassinations
               state.whitelisted.push_back( process::id());

               // Set the process variables so children can communicate with us.
               common::environment::variable::process::set(
                     common::environment::variable::name::ipc::domain::manager,
                     common::process::handle());

               handle::mandatory::boot::core::prepare( state);

               if( ! state.bare)
                  handle::mandatory::boot::prepare( state);

               
               // begin the boot procedure...
               handle::boot( state, settings.parent.correlation);

               return state;
            }

            namespace condition
            {
               namespace detail
               {
                  auto error( State& state)
                  {
                     return [&state]()
                     {
                        auto error = exception::capture();

                        if( error.code() == code::casual::shutdown)
                        {
                           state.runlevel = state::Runlevel::shutdown;
                           handle::shutdown( state);
                        }
                        else if( state.runlevel == state::Runlevel::error)
                        {
                           log::line( log::category::error, error, " already in error state - fatal abort");
                           log::line( log::category::verbose::error, "state: ", state);
                           throw;
                        }
                        else 
                        {
                           log::line( log::category::error, error, " enter error state");
                           state.runlevel = state::Runlevel::error;
                           handle::shutdown( state);
                           log::line( log::category::verbose::error, "state: ", state);
                        }
                     };
                  }
               }
               auto create( State& state)
               {
                  namespace condition = message::dispatch::condition;
                  return condition::compose(
                     condition::idle( [&state]() { state.tasks.idle( state);}),
                     condition::done( [&state]() { return ! state.execute();}),
                     condition::error( detail::error( state))
                  );
               }
            }


            void start( State state)
            {
               Trace trace{ "domain::manager::local::start"};
               log::line( verbose::log, "state: ", state);

               auto handler = handle::create( state);

               message::dispatch::pump(
                  local::condition::create( state),
                  handler,
                  manager::ipc::device());
            }


            void main( int argc, char** argv)
            {
               common::strong::ipc::id ipc;

               try
               {
                  Settings settings;
                  { 
                     bool dummy{};
                     argument::Parse{ "domain manager",
                        argument::Option( std::tie( settings.configuration), argument::option::keys( { "-c", "--configuration"}, {"--configuration-files"}), "domain configuration 'glob' patterns"),
                        argument::Option( std::tie( settings.bare), { "--bare"}, "if use 'bare' mode or not, ie, do not boot mandatory (broker, TM), mostly for unittest"),
                        argument::Option( std::tie( settings.parent.ipc.underlying()), { "--event-ipc"}, "ipc to send events to"),
                        argument::Option( std::tie( settings.parent.correlation), { "--event-id"}, "id of the events to correlate"),
                        
                        argument::Option( std::tie( dummy), argument::option::keys( {}, { "--persist"}), "not used"),
                     }( argc, argv);
                  }

                  ipc = settings.parent.ipc;

                  local::start( local::initialize( std::move( settings)));

               }
               catch( ...)
               {
                  if( ipc)
                  {
                     auto error = common::exception::capture();
                     common::message::event::Error event;
                     event.message = error.what();
                     event.severity = common::message::event::Error::Severity::fatal;
                     event.code = error.code();

                     common::communication::device::non::blocking::optional::send( ipc, event);
                  }

                  throw;
               }

            }
         } // <unnamed>
      } // local
      

   } // domain::manager
} // casual


int main( int argc, char** argv)
{
   casual::common::exception::main::log::guard( [=]()
   {
      casual::domain::manager::local::main( argc, argv);
   });
}

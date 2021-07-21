//! 
//! Copyright (c) 2015, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!


#include "domain/manager/state.h"
#include "domain/manager/handle.h"
#include "domain/manager/transform.h"
#include "domain/common.h"

#include "configuration/model/load.h"

#include "common/communication/ipc.h"
#include "common/exception/handle.h"
#include "common/argument.h"

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

            namespace configuration
            {
               auto load( const std::vector< std::string>& patterns)
               {
                  const auto files = common::file::find( patterns);

                  auto state = transform::model( casual::configuration::model::load( files));

                  //state.bare = settings.bare;

                  log::line( log::category::information, "used configuration: ", files, " from patterns: ", patterns);

                  return state;
               }
            } // configuration

            auto initialize( Settings settings)
            {
               Trace trace{ "domain::manager::local::initialize"};
               log::line( verbose::log, "settings: ", settings);

               auto state =  configuration::load( settings.configuration);
               state.bare = settings.bare;
               state.singleton_file = common::domain::singleton::create( common::process::handle(), common::domain::identity());

               state.parent.ipc = settings.parent.ipc;
               state.parent.correlation = settings.parent.correlation;

               //! if we've got a parent, we make sure it gets events.
               if( state.parent)
               {
                  common::message::event::subscription::Begin request;
                  request.process.ipc = state.parent.ipc;
                  state.event.subscription( request);
               }
         
               // make sure we handle death of our children
               signal::callback::registration< code::signal::child>( []()
               {
                  algorithm::for_each( process::lifetime::ended(), []( auto& exit)
                  {
                     manager::handle::event::process::exit( exit);
                  });
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

               handle::boot( state, state.parent.correlation);

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
                        argument::Option( std::tie( settings.parent.ipc.underlaying()), { "--event-ipc"}, "ipc to send events to"),
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

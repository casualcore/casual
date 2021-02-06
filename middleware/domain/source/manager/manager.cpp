//! 
//! Copyright (c) 2015, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!


#include "domain/manager/manager.h"
#include "domain/manager/handle.h"
#include "domain/manager/configuration.h"
#include "domain/common.h"

#include "common/file.h"
#include "common/environment.h"
#include "common/uuid.h"
#include "common/exception/handle.h"


#include <fstream>

namespace casual
{
   using namespace common;

   namespace domain
   {
      namespace manager
      {

         Manager::Manager( Settings&& settings)
           : m_state{ configuration::state( settings)},
             m_singelton{ common::domain::singleton::create( common::process::handle(), common::domain::identity())}
         {
            Trace trace{ "domain::Manager ctor"};

            // make sure we handle death of our children
            signal::callback::registration< code::signal::child>( []()
            {
               algorithm::for_each( process::lifetime::ended(), []( auto& exit)
               {
                  manager::handle::event::process::exit( exit);
               });
            });

            // make sure we're whitelisted from assassinations
            m_state.whitelisted.push_back( process::id());

            // Set the process variables so children can communicate with us.
            common::environment::variable::process::set(
                  common::environment::variable::name::ipc::domain::manager,
                  common::process::handle());

            // start casual-domain-pending-message
            handle::start::pending::message( m_state);

            if( ! m_state.bare)
               handle::mandatory::boot::prepare( m_state);

            handle::boot( m_state, settings.event.id);
         }

         Manager::~Manager()
         {
            Trace trace{ "domain::Manager dtor"};
         }


         namespace local
         {
            namespace
            {
               namespace callback
               {
                  auto error( State& state)
                  {
                     return [&state]()
                     {
                        auto condition = exception::code();

                        if( condition == code::casual::shutdown)
                        {
                           state.runlevel( State::Runlevel::shutdown);
                           handle::shutdown( state);
                        }
                        else if( state.runlevel() == State::Runlevel::error)
                        {
                           log::line( log::category::error, condition, " already in error state - fatal abort");
                           log::line( log::category::verbose::error, "state: ", state);
                           throw;
                        }
                        else 
                        {
                           log::line( log::category::error, condition, " enter error state");
                           state.runlevel( State::Runlevel::error);
                           handle::shutdown( state);
                           log::line( log::category::verbose::error, "state: ", state);
                        }
                     };
                  }

               } // callback

               auto condition( State& state)
               {
                  namespace condition = message::dispatch::condition;
                  return condition::compose(
                     condition::idle( [&state]() { state.tasks.idle( state);}),
                     condition::done( [&state]() { return ! state.execute();}),
                     condition::error( callback::error( state))
                  );
               }

            } // <unnamed>
         } // local

         void Manager::start()
         {
            Trace trace{ "domain::Manager::start"};

            auto handler = manager::handler( m_state);

            message::dispatch::pump(
               local::condition( m_state),
               handler,
               manager::ipc::device());
         }

      } // manager
   } // domain

} // casual

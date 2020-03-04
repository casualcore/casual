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
#include "common/exception/casual.h"


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

            // Set the process variables so children can communicate with us.
            common::environment::variable::process::set(
                  common::environment::variable::name::ipc::domain::manager,
                  common::process::handle());

            // start casual-domain-pending-message
            m_state.process.pending = handle::start::pending::message();

            if( m_state.mandatory_prepare)
               handle::mandatory::boot::prepare( m_state);

            handle::boot( m_state);
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
                  auto idle( State& state)
                  {
                     return [&state]()
                     {
                        state.tasks.idle( state);
                     };
                  }

                  auto done( State& state)
                  {
                     return [&state]()
                     {
                        return ! state.execute();
                     };
                  }

                  auto error( State& state)
                  {
                     return [&state]()
                     {
                        try
                        {
                           throw;
                        }
                        catch( const exception::casual::Shutdown&)
                        {
                           state.runlevel( State::Runlevel::shutdown);
                           handle::shutdown( state);
                        }
                        catch( const exception::system::communication::Unavailable&)
                        {
                           exception::handle();
                           state.runlevel( State::Runlevel::error);
                           handle::shutdown( state);
                           throw;
                        }
                        catch( ...)
                        {
                           exception::handle();
                           state.runlevel( State::Runlevel::error);
                           handle::shutdown( state);
                        }
                     };
                  }

               } // callback

            } // <unnamed>
         } // local

         void Manager::start()
         {
            Trace trace{ "domain::Manager::start"};

            auto handler = manager::handler( m_state);

            message::dispatch::empty::conditional::pump(
               handler,
               manager::ipc::device(),
               local::callback::idle( m_state),
               local::callback::done( m_state),
               local::callback::error( m_state));
         }

      } // manager
   } // domain

} // casual

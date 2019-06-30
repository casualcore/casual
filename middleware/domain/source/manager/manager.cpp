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

            // Set the process variables so children can communicate with us.
            common::environment::variable::process::set(
                  common::environment::variable::name::ipc::domain::manager(),
                  common::process::handle());

            // start casual-domain-pending-message
            m_state.process.pending = handle::start::pending::message();

            if( m_state.mandatory_prepare)
               handle::mandatory::boot::prepare( m_state);

            handle::boot( m_state);
         }

         Manager::~Manager()
         {
         }


         namespace local
         {
            namespace
            {
               namespace message
               {
                  void pump( State& state)
                  {
                     Trace trace{ "domain message pump"};

                     auto handler = manager::handler( state);


                     while( state.execute())
                     {
                        try
                        {
                           // We always block
                           handler( manager::ipc::device().blocking_next());
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
                     }

                  }
               } // message

            } // <unnamed>
         } // local

         void Manager::start()
         {
            try
            {
               local::message::pump( m_state);
            }
            catch( const exception::casual::Shutdown&)
            {
               m_state.runlevel( State::Runlevel::shutdown);
            }
            catch( ...)
            {
               exception::handle();
               m_state.runlevel( State::Runlevel::error);
            }
         }

      } // manager
   } // domain

} // casual

//!
//! casual 
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

            //
            // Set the process variables so children can communicate with us.
            //
            common::environment::variable::process::set(
                  common::environment::variable::name::ipc::domain::manager(),
                  common::process::handle());


            if( m_state.mandatory_prepare)
            {
               handle::mandatory::boot::prepare( m_state);
            }

            handle::boot( m_state);
         }

         Manager::~Manager()
         {
            Trace trace{ "domain::Manager dtor"};

            try
            {


            }
            catch( ...)
            {
               exception::handle();
            }
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
                           if( state.pending.replies.empty())
                           {
                              //
                              // We can block
                              //
                              handler( manager::ipc::device().blocking_next());
                           }
                           else
                           {

                              //
                              // Take care of pending replies
                              //
                              {
                                 signal::thread::scope::Block block;

                                 auto& pending = state.pending.replies;

                                 log << "pending replies: " << range::make( pending) << '\n';

                                 algorithm::trim( pending, algorithm::remove_if( pending,
                                       common::message::pending::sender(
                                             communication::ipc::policy::non::Blocking{})));
                              }


                              {
                                 //
                                 // If we've got pending that is 'never' sent, we still want to
                                 // do a lot of domain stuff. Hence, if we got into an 'error state'
                                 // we'll still function...
                                 //
                                 // TODO: Should we have some sort of TTL for the pending?
                                 //
                                 auto count = common::platform::batch::domain::pending;

                                 while( handler( ipc::device().non_blocking_next()) && count-- > 0)
                                    ;
                              }
                           }
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

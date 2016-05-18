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


#include <fstream>

namespace casual
{
   using namespace common;

   namespace domain
   {
      namespace manager
      {

         namespace local
         {
            namespace
            {

               file::scoped::Path singleton()
               {
                  auto path = environment::domain::singleton::file();
                  std::ifstream file( path);

                  if( file)
                  {
                     //
                     // There is potentially a running casual-domain already - abort
                     //
                     throw common::exception::invalid::Process( "can only be one casual-domain running in a domain");
                  }

                  file::scoped::Path result( std::move( path));


                  std::ofstream output( result);

                  if( output)
                  {
                     output << communication::ipc::inbound::id() << std::endl;
                     output << process::uuid() << std::endl;
                  }
                  else
                  {
                     throw common::exception::invalid::File( "failed to write process singleton queue file: " + path);
                  }

                  return result;
               }

            } // <unnamed>
         } // local


         Manager::Manager( Settings&& settings)
           : m_singelton{ local::singleton()}, m_state{ configuration::state( settings)}
         {
            Trace trace{ "domain::Manager ctor"};

            //
            // Set our ipc-queue so children easy can send messages to us.
            //
            environment::variable::set( environment::variable::name::ipc::domain::manager(), communication::ipc::inbound::id());

            if( ! settings.bare)
            {
               handle::mandatory::boot( m_state);
            }

            handle::boot( m_state);
         }

         Manager::~Manager()
         {
            Trace trace{ "domain::Manager dtor"};

            try
            {
               //handle::shutdown( m_state);


            }
            catch( ...)
            {
               error::handler();
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

                                 range::trim( pending, range::remove_if( pending,
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
                                 auto count = common::platform::batch::transaction;

                                 while( handler( ipc::device().non_blocking_next()) && count-- > 0)
                                    ;
                              }
                           }

                           log << "state: " << state << '\n';

                        }
                        catch( const exception::Shutdown&)
                        {
                           state.runlevel( State::Runlevel::shutdown);
                           handle::shutdown( state);
                        }
                        catch( const exception::queue::Unavailable&)
                        {
                           error::handler();
                           state.runlevel( State::Runlevel::error);
                           handle::shutdown( state);
                           throw;
                        }
                        catch( ...)
                        {
                           error::handler();
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
            catch( const exception::Shutdown&)
            {
               m_state.runlevel( State::Runlevel::shutdown);
            }
            catch( ...)
            {
               error::handler();
               m_state.runlevel( State::Runlevel::error);
            }
         }

      } // manager
   } // domain

} // casual

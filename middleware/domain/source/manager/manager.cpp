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
                  Trace trace{ "domain::manager::local::singleton"};

                  auto path = environment::domain::singleton::file();

                  auto temp_file = file::scoped::Path{ file::name::unique( "/tmp/", ".tmp")};

                  std::ofstream output( temp_file);

                  if( output)
                  {
                     output << communication::ipc::inbound::id() << '\n';
                     output << common::domain::identity().name << '\n';
                     output << common::domain::identity().id << std::endl;
                  }
                  else
                  {
                     throw common::exception::invalid::File( "failed to write temporary domain singleton file: " + temp_file.path());
                  }


                  if( common::file::exists( path))
                  {
                     //
                     // There is potentially a running casual-domain already - abort
                     //
                     throw common::exception::invalid::Process( "can only be one casual-domain running in a domain");
                  }

                  common::file::move( temp_file, path);

                  temp_file.release();

                  return { std::move( path)};
               }

            } // <unnamed>
         } // local


         Manager::Manager( Settings&& settings)
           : m_state{ configuration::state( settings)}
         {
            Trace trace{ "domain::Manager ctor"};

            m_singelton = local::singleton();

            //
            // Set our ipc-queue so children easy can send messages to us.
            //
            environment::variable::set( environment::variable::name::ipc::domain::manager(), communication::ipc::inbound::id());

            if( ! settings.bare)
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
                                 auto count = common::platform::batch::transaction();

                                 while( handler( ipc::device().non_blocking_next()) && count-- > 0)
                                    ;
                              }
                           }

                           //log << "state: " << state << '\n';

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

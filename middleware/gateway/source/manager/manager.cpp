//!
//! casual
//!

#include "gateway/manager/manager.h"
#include "gateway/manager/handle.h"
#include "gateway/environment.h"
#include "gateway/transform.h"
#include "gateway/common.h"

#include "config/domain.h"


#include "common/trace.h"


namespace casual
{
   using namespace common;

   namespace gateway
   {

      namespace manager
      {
         namespace local
         {
            namespace
            {
               manager::State connect( manager::Settings settings)
               {
                  Trace trace{ "gateway::manager::local::connect"};

                  //
                  // Connect to domain
                  //
                  process::instance::connect( process::instance::identity::gateway::manager());


                  if( ! settings.configuration.empty())
                  {
                     return gateway::transform::state(
                           config::gateway::transform::gateway(
                                 config::domain::get( settings.configuration).gateway));
                  }


                  //
                  // Ask domain manager for configuration
                  //
                  common::message::domain::configuration::gateway::Request request;
                  request.process = process::handle();

                  return gateway::transform::state(
                        manager::ipc::device().call(
                              communication::ipc::domain::manager::device(),
                              request));

               }

            } // <unnamed>
         } // local

      } // manager






      Manager::Manager( manager::Settings settings)
        : m_state{ manager::local::connect( std::move( settings))}
      {
         Trace trace{ "gateway::Manager::Manager"};


      }

      Manager::~Manager()
      {
         Trace trace{ "gateway::Manager::~Manager"};

         try
         {
            //
            // Shutdown
            //
            manager::handle::shutdown( m_state);
         }
         catch( ...)
         {
            error::handler();
         }

      }

      void Manager::start()
      {
         Trace trace{ "gateway::Manager::start"};

         //
         // boot outbounds
         //
         {

            manager::handle::boot( m_state);
            m_state.runlevel = manager::State::Runlevel::online;
         }


         //
         // start message pump
         //
         {

            auto handler = manager::handler( m_state);


            while( true)
            {
               handler( manager::ipc::device().blocking_next());
            }
         }
      }


   } // gateway


} // casual

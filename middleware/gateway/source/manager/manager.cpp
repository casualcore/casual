//!
//! manager.cpp
//!
//! Created on: Nov 8, 2015
//!     Author: Lazan
//!

#include "gateway/manager/manager.h"
#include "gateway/manager/handle.h"
#include "gateway/environment.h"
#include "gateway/transform.h"


#include "common/trace.h"


namespace casual
{
   using namespace common;

   namespace gateway
   {

      namespace manager
      {
         namespace transform
         {

            State settings( Settings settings)
            {
               Trace trace{ "gateway::manager::transform::settings", log::internal::gateway};

               if( settings.configuration.empty())
               {
                  return gateway::transform::state( config::gateway::get());
               }
               return gateway::transform::state( config::gateway::get( settings.configuration));
            }

         } // transform

      } // manager



      Manager::Manager( manager::Settings settings)
        : m_state{ manager::transform::settings( std::move( settings))}
      {
         Trace trace{ "gateway::Manager::Manager", log::internal::gateway};

         //
         // Set environment variables for children
         //
         gateway::environment::manager::set( manager::ipc::device().id());
      }

      Manager::~Manager()
      {
         Trace trace{ "gateway::Manager::~Manager", log::internal::gateway};

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
         Trace trace{ "gateway::Manager::start", log::internal::gateway};

         //
         // boot outbounds
         //
         {
            manager::handle::boot( m_state);


         }


         //
         // start message pump
         //
         {

            auto handler = manager::handler( m_state);


            while( handler( manager::ipc::device().blocking_next()))
            {
               ;
            }
         }
      }


   } // gateway


} // casual

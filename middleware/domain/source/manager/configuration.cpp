//!
//! casual 
//!

#include "domain/manager/configuration.h"
#include "domain/manager/manager.h"
#include "domain/transform.h"

#include "config/domain.h"

namespace casual
{
   using namespace common;
   namespace domain
   {
      namespace manager
      {
         namespace configuration
         {

            State state( const Settings& settings)
            {
               return transform::state( config::domain::get( settings.configurationfiles));
            }

            void persist( const State& state)
            {
               Trace trace{ "domain::manager::configuration::persist"};


            }

         } // configuration
      } // manager
   } // domain



} // casual

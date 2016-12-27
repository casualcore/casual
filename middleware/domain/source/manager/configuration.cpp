//!
//! casual 
//!

#include "domain/manager/configuration.h"

#include "configuration/domain.h"
#include "domain/manager/manager.h"
#include "domain/transform.h"


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
               return transform::state( casual::configuration::domain::get( settings.configurationfiles));
            }


         } // configuration
      } // manager
   } // domain



} // casual

//!
//! casual 
//!

#ifndef CASUAL_MIDDLEWARE_DOMAIN_INCLUDE_DOMAIN_MANAGER_CONFIGURATION_H_
#define CASUAL_MIDDLEWARE_DOMAIN_INCLUDE_DOMAIN_MANAGER_CONFIGURATION_H_

#include "domain/manager/state.h"

namespace casual
{
   namespace domain
   {
      namespace manager
      {
         struct Settings;
         namespace configuration
         {

            State state( const Settings& settings);


            void persist( const State& state);


         } // configuration
      } // manager

   } // domain

} // casual

#endif // CASUAL_MIDDLEWARE_DOMAIN_INCLUDE_DOMAIN_MANAGER_CONFIGURATION_H_

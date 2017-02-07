//!
//! casual 
//!

#ifndef CASUAL_MIDDLEWARE_DOMAIN_INCLUDE_DOMAIN_MANAGER_PERSISTENT_H_
#define CASUAL_MIDDLEWARE_DOMAIN_INCLUDE_DOMAIN_MANAGER_PERSISTENT_H_

#include "domain/manager/state.h"



namespace casual
{
   namespace domain
   {
      namespace manager
      {
         namespace persistent
         {
            namespace state
            {

               void save( const State&);
               void save( const State&, const std::string& file);

               State load( const std::string& file);
               State load();

            } // state


         } // persistent

      } // manager
   } // domain


} // casual

#endif // CASUAL_MIDDLEWARE_DOMAIN_INCLUDE_DOMAIN_MANAGER_PERSISTENT_H_

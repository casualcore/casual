//!
//! casual 
//!

#ifndef CASUAL_MIDDLEWARE_DOMAIN_INCLUDE_DOMAIN_TRANSFORM_H_
#define CASUAL_MIDDLEWARE_DOMAIN_INCLUDE_DOMAIN_TRANSFORM_H_

#include "domain/manager/admin/vo.h"
#include "domain/manager/state.h"

namespace casual
{
   namespace configuration
   {
      namespace domain
      {
         struct Manager;
      } // domain
   } // config
   namespace domain
   {
      namespace transform
      {

         manager::admin::vo::State state( const manager::State& state);


         manager::State state( const configuration::domain::Manager& domain);


      } // transform

   } // domain


} // casual

#endif // CASUAL_MIDDLEWARE_DOMAIN_INCLUDE_DOMAIN_TRANSFORM_H_

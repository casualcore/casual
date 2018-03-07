//! 
//! Copyright (c) 2015, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!


#ifndef CASUAL_MIDDLEWARE_DOMAIN_INCLUDE_DOMAIN_TRANSFORM_H_
#define CASUAL_MIDDLEWARE_DOMAIN_INCLUDE_DOMAIN_TRANSFORM_H_

#include "domain/manager/admin/vo.h"
#include "domain/manager/state.h"

#include "configuration/domain.h"

namespace casual
{
   namespace domain
   {
      namespace transform
      {

         manager::admin::vo::State state( const manager::State& state);


         manager::State state( casual::configuration::domain::Manager domain);

      } // transform

   } // domain


} // casual

#endif // CASUAL_MIDDLEWARE_DOMAIN_INCLUDE_DOMAIN_TRANSFORM_H_

//! 
//! Copyright (c) 2015, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!


#pragma once

#include "configuration/user.h"
#include "configuration/model.h"

namespace casual
{
   namespace configuration::model
   {
      configuration::Model transform( user::Domain domain);
      user::Domain transform( const configuration::Model& domain);

      model::domain::Environment transform( user::Environment environment);

   } // configuration::model

} // casual



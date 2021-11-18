//! 
//! Copyright (c) 2015, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!


#pragma once

#include "configuration/model.h"

#include <vector>
#include <string>

namespace casual
{
   namespace configuration::system
   {
      model::system::Model get( const std::string& glob);

      //! @returns the system model deduced from environment variables
      model::system::Model get();

   } // configuration::system
} // casual



//!
//! Copyright (c) 2020, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!

#pragma once

#include "configuration/user.h"

#include <string>
#include <vector>

namespace casual
{
   namespace configuration
   {
      namespace user
      {
         //! for each file: load the user configuration and accumulate. 
         //! Then normalize the whole model
         //! @return an accumulated model of all user configuration files
         Domain load( const std::vector< std::string>& files);

      } // user
   } // configuration

} // casual
//!
//! Copyright (c) 2020, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!

#pragma once

#include "configuration/model.h"

#include <string>
#include <vector>

namespace casual
{
   namespace configuration
   {
      namespace model
      {
         //! for each file: load the user configuration and transform it to the model
         //! @return an accumulated model of all user configuration files
         configuration::Model load( const std::vector< std::string>& files);

      } // model
   } // configuration

} // casual
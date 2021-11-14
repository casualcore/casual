//!
//! Copyright (c) 2020, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!

#pragma once

#include "configuration/model.h"

#include <vector>
#include <filesystem>

namespace casual
{
   namespace configuration::model
   {
      //! for each file: load the user configuration and transform it to the model
      //! @return an accumulated model of all user configuration files
      configuration::Model load( const std::vector< std::filesystem::path>& files);

   } // configuration

} // casual
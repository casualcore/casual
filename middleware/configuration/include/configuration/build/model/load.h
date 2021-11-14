//!
//! Copyright (c) 2021, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!

#pragma once

#include "configuration/build/model.h"

#include <filesystem>

namespace casual
{
   namespace configuration::build::model::load
   {
      server::Model server( const std::filesystem::path& path);
      executable::Model executable( const std::filesystem::path& path);
      
   } // configuration::build::model::load
} // casual
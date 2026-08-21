//!
//! Copyright (c) 2018, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!

#pragma once

#include "tools/build/settings.h"
#include <filesystem>

namespace casual
{
   namespace tools::build
   {
      void task( const std::filesystem::path& input, const Settings& directive);

   } // tools::build
} // casual

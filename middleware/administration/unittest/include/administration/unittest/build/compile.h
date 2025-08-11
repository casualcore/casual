//!
//! Copyright (c) 2025, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!

#pragma once

#include "common/file.h"

#include <string_view>

namespace casual
{
   namespace administration::unittest::build
   {
      // compiles content and returns the object file
      casual::common::file::scoped::Path compile( std::string_view source_content);
   }
} // casual

//!
//! Copyright (c) 2018, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!

#pragma once

#include "casual/argument.h"

namespace casual
{
   namespace file::manager::admin::cli
   {
      argument::Option options();
      std::vector< std::tuple< std::string, std::string>> information();

   } // file::manager::admin::cli
} // casual

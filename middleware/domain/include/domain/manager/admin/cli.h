//!
//! Copyright (c) 2018, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!

#pragma once


#include "casual/argument.h"
#include "common/pimpl.h"

namespace casual
{
   namespace domain::manager::admin::cli
   {
      argument::Option options();

      std::vector< std::tuple< std::string, std::string>> information();

   } // domain::manager::admin::cli
} // casual

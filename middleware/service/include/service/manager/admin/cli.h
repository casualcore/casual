//! 
//! Copyright (c) 2015, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!

#pragma once


#include "casual/argument.h"
#include "common/pimpl.h"

namespace casual
{
   namespace service::manager::admin::cli
   {
      std::vector< std::tuple< std::string, std::string>> information();

      argument::Option options();

   } // service::manager::admin::cli
} // casual

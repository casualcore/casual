//!
//! Copyright (c) 2021, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!

#pragma once

#include "configuration/build/model.h"

namespace casual
{
   namespace configuration::example::build::model
   {
      configuration::build::server::Model server();
      configuration::build::executable::Model executable();

   } // configuration::example::build::model
} // casual
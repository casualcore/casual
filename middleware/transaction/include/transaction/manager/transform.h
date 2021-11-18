//!
//! Copyright (c) 2021, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!

#pragma once

#include "transaction/manager/state.h"

#include "configuration/model.h"

namespace casual
{
   namespace transaction::manager::transform
   {
      State state( casual::configuration::Model model);
      
   } // transaction::manager::transform
} // casual
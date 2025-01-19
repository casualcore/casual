//! 
//! Copyright (c) 2015, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!

#pragma once

#include "common/server/argument.h"

namespace casual
{
   namespace file::manager
   {
      struct State;

      namespace admin
      {
         common::server::Arguments services( manager::State& state);
      } // admin
   } // manager

} // casual



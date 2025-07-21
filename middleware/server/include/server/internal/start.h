//!
//! Copyright (c) 2025, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!

#pragma once

#include "server/argument.h"

namespace casual
{
   namespace server::internal
   {
      void start( server::Arguments arguments, common::function<void()const> initialize);

   } // server::internal
   
} // casual

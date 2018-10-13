//!
//! Copyright (c) 2018, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!

#pragma once

#include "http/outbound/state.h"
#include "http/outbound/configuration.h"

namespace casual
{
   namespace http
   {
      namespace outbound
      {
         namespace transform
         {
            State configuration( configuration::Model model);
         } // transform
      } // outbound
   } // http
} // casual
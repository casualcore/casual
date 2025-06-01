//!
//! Copyright (c) 2025, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!

#include "domain/discovery/instance.h"

namespace casual
{
   namespace domain::discovery::instance
   {
      common::communication::instance::outbound::detail::optional::Device& device()
      {
         static common::communication::instance::outbound::detail::optional::Device device{ identity};
         return device;
      }

   } // domain::discovery::instance
} // casual
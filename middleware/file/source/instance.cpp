//! 
//! Copyright (c) 2025, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!

#include "file/instance.h"

namespace casual
{
   namespace file::instance
   {
      common::communication::instance::outbound::detail::optional::Device& device()
      {
         static common::communication::instance::outbound::detail::optional::Device device{ instance::identity};
         return device;
      }

   } // file::instance
} // casual



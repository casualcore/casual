//!
//! Copyright (c) 2023, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!


#include "common/algorithm/random.h"

namespace casual
{
   namespace common::algorithm::random
   {
      std::mt19937& generator() noexcept
      {
         static std::mt19937 singleton{ std::random_device{}()};
         return singleton;
      }
      
   } // common::algorithm::random
} // casual
//!
//! Copyright (c) 2020, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!

#pragma once

#include <vector>
#include <string>

namespace casual
{
   namespace domain::unittest
   {
      namespace service
      {
         std::vector< std::string> discover( std::vector< std::string> services);
      } // service

      void discover( std::vector< std::string> services, std::vector< std::string> queues);
      
   } // domain::unittest
} // casual
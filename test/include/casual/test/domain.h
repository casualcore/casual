//!
//! Copyright (c) 2019, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!

#pragma once

#include "domain/manager/unittest/process.h"

#include <string>
#include <vector>

namespace casual
{
   namespace test
   {
      namespace domain
      {
         struct Manager : casual::domain::manager::unittest::Process
         {
            Manager( const std::vector< std::string>& configuration);
         };
      } // domain
   } // test
} // casual
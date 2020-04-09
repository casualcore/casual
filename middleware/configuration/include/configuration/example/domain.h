//! 
//! Copyright (c) 2015, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!

#pragma once

#include "configuration/user.h"

namespace casual
{
   namespace configuration
   {
      namespace example
      {
         configuration::user::Domain domain();
         configuration::user::queue::Manager queue();

      } // example

   } // configuration


} // casual



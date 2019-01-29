//! 
//! Copyright (c) 2015, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!


#include "configuration/executable.h"

#include "common/algorithm.h"

namespace casual
{
   namespace configuration
   {
      Executable& Executable::operator += ( const executable::Default& value)
      {
         instances = common::coalesce( instances, value.instances);
         restart = common::coalesce( restart, value.restart);
         memberships = common::coalesce( memberships, value.memberships);
         environment = common::coalesce( environment, value.environment);

         return *this;
      }

   } // configuration
} // casual

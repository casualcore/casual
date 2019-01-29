//! 
//! Copyright (c) 2015, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!


#include "configuration/group.h"

#include "common/algorithm.h"

namespace casual
{
   namespace configuration
   {
      bool operator == ( const Group& lhs, const Group& rhs)
      {
         return lhs.name == rhs.name;
      }

      bool operator < ( const Group& lhs, const Group& rhs)
      {
         return lhs.name < rhs.name;
      }

   } // configuration
} // casual

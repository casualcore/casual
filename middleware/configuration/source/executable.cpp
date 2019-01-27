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

      namespace local
      {
         namespace
         {
            auto tie( const Executable& value)
            {
               return std::tie( value.path, value.alias);
            }
         } // <unnamed>
      } // local

      Executable& Executable::operator += ( const executable::Default& value)
      {
         instances = common::coalesce( instances, value.instances);
         restart = common::coalesce( restart, value.restart);
         memberships = common::coalesce( memberships, value.memberships);
         environment = common::coalesce( environment, value.environment);

         return *this;
      }


      bool operator == ( const Executable& lhs, const Executable& rhs)
      {
         return local::tie( lhs) == local::tie( rhs);
      }

      bool operator < ( const Executable& lhs, const Executable& rhs)
      {
         return local::tie( lhs) < local::tie( rhs);
      }

   } // configuration
} // casual

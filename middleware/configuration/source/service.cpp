//! 
//! Copyright (c) 2015, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!


#include "configuration/service.h"


namespace casual
{
   namespace configuration
   {
      Service& Service::operator += ( const service::Default& rhs)
      {
         timeout = common::coalesce( timeout, rhs.timeout);
         return *this;
      }

      bool operator == ( const Service& lhs, const Service& rhs)
      {
         return lhs.name == rhs.name;
      }
   } // configuration
} // casual

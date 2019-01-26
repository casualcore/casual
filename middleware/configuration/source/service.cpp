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
      namespace service
      {
         bool operator == ( const Service& lhs, const Service& rhs)
         {
            return lhs.name == rhs.name;
         }

         Service& operator += ( Service& lhs, const service::Default& rhs)
         {
            lhs.timeout = common::coalesce( lhs.timeout, rhs.timeout);
            return lhs;
         }

      } // service
   } // configuration
} // casual

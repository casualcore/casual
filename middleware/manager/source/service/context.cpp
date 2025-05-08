//!
//! Copyright (c) 2025, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!

#include "casual/manager/service/context.h"

namespace casual
{
   namespace manager::service
   {
      namespace context
      {
         State::State( std::vector< manager::Service> services)
         {
            for( auto& service : services)
               this->services[ service.name] = std::move( service);
         }
      } // context
      
      
   } // manager::service
} // casual
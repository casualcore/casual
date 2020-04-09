//! 
//! Copyright (c) 2015, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!


#pragma once


#include "common/server/service.h"
#include "common/transaction/context.h"
#include "common/serialize/macro.h"

#include <vector>


namespace casual
{
   namespace common
   {
      namespace server
      {

         struct Arguments
         {

            Arguments();
            Arguments( std::vector< Service> services);
            Arguments( std::vector< Service> services, std::vector< transaction::resource::Link> resources);

            Arguments( Arguments&&);
            Arguments& operator = (Arguments&&);

            std::vector< Service> services;
            std::vector< transaction::resource::Link> resources;

            CASUAL_LOG_SERIALIZE(
               CASUAL_SERIALIZE( services);
               CASUAL_SERIALIZE( resources);
            )
         };

      } // server
   } // common
} // casual



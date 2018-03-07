//! 
//! Copyright (c) 2015, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!


#ifndef CASUAL_COMMON_SERVER_ARGUMENTS_H_
#define CASUAL_COMMON_SERVER_ARGUMENTS_H_

#include "common/server/service.h"

#include "common/transaction/context.h"


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
         };

      } // server
   } // common
} // casual

#endif // ARGUMENTS_H_

//! 
//! Copyright (c) 2015, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!


#pragma once


#include "server/service.h"

#include "transaction/resource/link.h"

#include "common/serialize/macro.h"

#include <vector>


namespace casual
{
   namespace server
   {
      struct Arguments
      {
         std::vector< Service> services;
         std::vector< transaction::resource::Link> resources;

         CASUAL_LOG_SERIALIZE(
            CASUAL_SERIALIZE( services);
            CASUAL_SERIALIZE( resources);
         )
      };

   } // server

} // casual



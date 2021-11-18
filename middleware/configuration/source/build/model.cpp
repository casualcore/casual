//!
//! Copyright (c) 2021, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!

#include "configuration/build/model.h"

namespace casual
{
   namespace configuration::build
   {
      namespace server
      {
         Model normalize( Model model)
         {
            for( auto& service : model.server.services)
            {
               if( ! service.transaction)
                  service.transaction = model.server.server_default.service.transaction;
               if( ! service.function)
                  service.function = service.name;
               if( ! service.category)
                  service.category = model.server.server_default.service.category;
            }
            return model;
         }
         
      } // server
   } // configuration::build
   
} // casual
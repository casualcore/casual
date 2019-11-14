//! 
//! Copyright (c) 2019, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!

#pragma once

#include "casual/domain/manager/api/model.h"
#include "domain/manager/admin/model.h"

namespace casual
{
   namespace domain
   {
      namespace manager
      {
         namespace api
         {
            namespace internal
            {
               namespace transform
               {
                  api::Model state( admin::model::State state);
                  
               } // transform

            } // internal
         } // api
      } // manager
   } // domain
} // casual

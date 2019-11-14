//! 
//! Copyright (c) 2019, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!

#pragma once

#include "casual/domain/manager/api/model.h"

namespace casual
{
   namespace domain
   {
      namespace manager
      {
         namespace api
         {
            inline namespace v1 
            {
               //! @return the current state of casual-domain-manager
               Model state();

            } // v1
         } // api
      } // manager
   } // domain
} // casual

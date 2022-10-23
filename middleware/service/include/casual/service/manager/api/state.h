//!
//! Copyright (c) 2019, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!

#pragma once

#include "casual/service/manager/api/model.h"

namespace casual
{
   namespace service::manager::api
   {
      inline namespace v1 
      {
         //! fetches the current state of casual-service-manager 
         //! @throws ...
         Model state();

      } // v1

   } // service::manager::api
} // casual
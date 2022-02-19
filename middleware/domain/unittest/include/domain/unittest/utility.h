//!
//! Copyright (c) 2022, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!

#pragma once

#include "common/unittest.h"

#include "domain/manager/admin/model.h"

namespace casual
{
   namespace domain::unittest
   {
      manager::admin::model::State state();

      namespace fetch
      {
         constexpr auto until = common::unittest::fetch::until( &unittest::state);
      }

   } // domain::unittest
} // casual
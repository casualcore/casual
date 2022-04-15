//!
//! Copyright (c) 2021, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!

#pragma once

#include "common/unittest.h"

#include "transaction/manager/admin/model.h"

namespace casual
{
   namespace transaction::unittest
   {
      manager::admin::model::State state();

      namespace fetch
      {
         constexpr auto until = common::unittest::fetch::until( &unittest::state);
      }
      
   } // transaction::unittest
   
} // casual
//!
//! Copyright (c) 2025, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!

#pragma once

#include "common/transaction/id.h"

namespace casual
{
   namespace transaction
   {
      using ID = common::transaction::ID;

      namespace id = common::transaction::id;

      namespace resource
      {
         using id = common::strong::resource::id;
      } // resource


   } // transaction
   
} // casual
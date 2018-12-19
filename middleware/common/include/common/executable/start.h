//!
//! Copyright (c) 2018, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!

#pragma once

#include "common/transaction/resource/link.h"

#include <functional>
#include <string>
#include <vector>

#include <xa.h>

namespace casual
{
   namespace common
   {
      namespace executable
      {
         inline namespace v1
         {
            namespace argument
            {
               namespace transaction
               {
                  using Resource = common::transaction::resource::Link;
               } // transaction
            } // argument

            int start( std::vector< argument::transaction::Resource> resources, std::function< int()> user_main);

         } // v1
      } // executable
   } // common
} // casual
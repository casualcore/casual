//!
//! Copyright (c) 2018, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!

#pragma once

#include "transaction/resource/link.h"

#include "common/functional.h"

#include <string>
#include <vector>

#include <xa.h>

namespace casual
{
   namespace server::executable
   {
         inline namespace v1
         {
            namespace argument
            {
               namespace transaction
               {
                  using Resource = casual::transaction::resource::Link;
               } // transaction
            } // argument

            int start( std::vector< argument::transaction::Resource> resources, common::unique_function< int()> user_main);

         } // v1

   } // server::executable
} // casual

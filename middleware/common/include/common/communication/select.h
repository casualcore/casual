//!
//! Copyright (c) 2018, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!

#pragma once

#include "common/strong/id.h"

#include <functional>
#include <vector>

namespace casual
{
   namespace common
   {
      namespace communication
      {
         namespace select
         {
            struct Action
            {
               enum class Direction : short
               {
                  read,
                  write
               };
               
               Direction direction;
               strong::file::descriptor::id descriptor;
               std::function< void( strong::file::descriptor::id descriptor)> callback;
            };


            void block( const std::vector< Action>& actions);

         } // select
      } // communication
   } // common
} // casual